//g++ src/*.cpp -std=c++20 -O3 -o map_generator_mac

#include <iostream>
#include <algorithm>
#include <iterator>
#include <cmath>
#include <random>
#include <limits>
#include <tuple>
#include <queue>
#include <format>


#include "functions.hpp"
#include "data_types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

constexpr Float64 PI = 3.14159265358979323846;

// ======================================================================
//  Tunable settings - everything that controls the output is here
// ======================================================================

// Average province size in pixels. Each separate land area of a state (split off by rivers, sea or
// the state's own shape) gets max(1, round(area / PIXELS_PER_PROVINCE)) provinces.
constexpr SizeT PIXELS_PER_PROVINCE = 500;
// Land areas smaller than this get no province of their own: if a river cuts them off, the river is
// opened up next to them so they merge with the province across it (avoids tiny sliver provinces)
constexpr SizeT MIN_REGION_SIZE = 100;

// Main run: total flip attempts per state = ITERATIONS_PER_PIXEL * state pixel count
constexpr SizeT ITERATIONS_PER_PIXEL = 10;
// A flip that makes the map worse is still kept with probability exp(-scoreIncrease / temperature)
// (simulated annealing). Temperature is measured in "pixels of extra border": at temperature 1, a
// flip that adds one pixel's worth of border is kept ~37% of the time, one that adds 3 pixels ~5%.
// It falls geometrically from START to END over the main run. Ending slightly warm matters: at zero
// temperature, single-pixel flips lock borders into straight horizontal/vertical runs.
constexpr Float64 TEMPERATURE_START = 3.0;
constexpr Float64 TEMPERATURE_END = 0.12;
// How strongly pixels far from their province centre are favoured for flipping:
// weight = borderingSides * (1 + DISTANCE_WEIGHT * min(distance / radius, MAX_DISTANCE_RATIO))
// where radius = sqrt(area / pi), i.e. the radius of a circle of the same area
constexpr Float64 DISTANCE_WEIGHT = 1.0;
constexpr Float64 MAX_DISTANCE_RATIO = 3.0;

// Each province gets its own random target size: mean size * a log-normal factor with this spread
// (0.3 = roughly +-30%, close to how much vanilla provinces vary within a state). 0 = all equal.
constexpr Float64 TARGET_SIZE_SPREAD = 0.3;

// Score (lower is better):
//   SIZE_WEIGHT          * average over provinces of ((size - target) / target)^2
// + NEIGHBOURHOOD_WEIGHT * border score
constexpr Float64 SIZE_WEIGHT = 100.0;
// Border score: for every pixel, count the pixels within NEIGHBOURHOOD_RADIUS (same state only) that
// belong to a different province, weighted by the terrain cost below. It acts like border length, but
// also sees how *thin* part of a province is, so retracting a panhandle improves the score at every
// step. (A plain pixel-side perimeter was dropped: it favours horizontal/vertical borders.)
// Normalised so a map of circles scores about 1.
constexpr Float64 NEIGHBOURHOOD_WEIGHT = 3.0;
constexpr SignedInteger32 NEIGHBOURHOOD_RADIUS = 3;

// Random "terrain" the borders follow: a smooth multi-octave noise field, freshly seeded per state.
// Each pixel pair in the neighbourhood score is weighted by 1 + NOISE_STRENGTH * noise, so borders
// are cheap through the field's low parts and up to (1 + NOISE_STRENGTH) times as expensive elsewhere.
// That makes them meander along random routes instead of settling into straight lines and a honeycomb.
constexpr Float64 NOISE_STRENGTH = 3.0;
constexpr Float64 NOISE_WAVELENGTH = 48.0; // pixels, largest octave
constexpr SignedInteger32 NOISE_OCTAVES = 3;
// Ridged: cheap routes are the thin winding lines where the noise crosses its midpoint (like rivers),
// rather than broad blobby valleys
constexpr Boolean NOISE_RIDGED = true;

// Cleanup after the main run: cut off any part of a province too thin to fit a disc of radius
// THIN_RADIUS (plus anything only attached through such a part), hand those pixels to neighbouring
// provinces, then rebalance with CLEANUP_ITERATIONS_PER_PIXEL flips per pixel at TEMPERATURE_END.
// Repeated CLEANUP_PASSES times.
constexpr SizeT CLEANUP_PASSES = 2;
constexpr SignedInteger32 THIN_RADIUS = 3;
constexpr SizeT CLEANUP_ITERATIONS_PER_PIXEL = 2;

// Border smoothing at the very end: each border pixel switches to whichever province owns most of the
// disc of radius SMOOTHNESS around it (never splitting a province). Rounds off jagged edges and small
// bumps without favouring any direction. 0 = off, 1.5 = light, 2-3 = noticeably smooth, 4+ = very smooth.
// Fractional values are fine (the disc includes pixels within that distance).
constexpr Float64 SMOOTHNESS = 2.0;

// ======================================================================

struct Pixel {
    UnsignedInteger16 x, y;
};

struct State {
    ColourRGB colour;
    UnsignedInteger16 x0 = UINT16_MAX, x1 = 0, y0 = UINT16_MAX, y1 = 0;
    UnsignedInteger16 width = 0, height = 0;
    Vector<Pixel> pixels;
    Vector<Boolean> barrierPixels;

    State(): colour(), pixels(), barrierPixels() {
        pixels.reserve(6000);
        barrierPixels.reserve(6000);
    }
    State(const ColourRGB colour): colour(colour), pixels(), barrierPixels() {
        pixels.reserve(6000);
        barrierPixels.reserve(6000);
    }

    void AddPixel(const Pixel p) {
        pixels.push_back(p);

        if (p.x < x0) { x0 = p.x; }
        else if (p.x > x1) { x1 = p.x; }
        if (p.y < y0) { y0 = p.y; }
        else if (p.y > y1) { y1 = p.y; }
    }
    void AddPixel(const UnsignedInteger16 x, const UnsignedInteger16 y) {
        pixels.emplace_back(x, y);

        if (x < x0) { x0 = x; }
        else if (x > x1) { x1 = x; }
        if (y < y0) { y0 = y; }
        else if (y > y1) { y1 = y; }
    }
    void AddBarrierPixel(const UnsignedInteger32 colour) {
        // Land/water
        if (colour == 0x00ffffff || colour == 0x007a7a7a) {
            barrierPixels.push_back(false);
        }
        else {
            barrierPixels.push_back(true);
        }
    }

    void UpdateBoundaries() {
        width = x1 - x0 + 1;
        height = y1 - y0 + 1;
    }
};

Vector<State> LoadStates(SignedInteger32& mapWidth, SignedInteger32& mapHeight){
    Vector<State> statesVector; statesVector.reserve(1600);

    SignedInteger32 stateMapChannels{};
    UnsignedInteger8 *stateMapData = stbi_load("in/statemap.png", &mapWidth, &mapHeight, &stateMapChannels, 4);
    SignedInteger32 riverMapWidth{}, riverMapHeight{}, riverMapChannels{};
    UnsignedInteger8 *riverMapData = stbi_load("in/rivers.png", &riverMapWidth, &riverMapHeight, &riverMapChannels, 4);

    if (riverMapWidth != mapWidth || riverMapHeight != mapHeight) {
        FatalError("ERROR: statemap.png and rivers.png have different sizes.");
    }

    HashMap<UnsignedInteger32, UnsignedInteger16> stateColourToIndexMap;

    SizeT imgIndex = 0;
    // Cache most recently found state
    UnsignedInteger32 previousColourInt = 0;
    SizeT previousIndex = 0;

    for (SizeT y = 0; y < mapHeight; y += 1) {
        for (SizeT x = 0; x < mapWidth; x += 1) {
            const ColourRGB pixelColour(stateMapData[imgIndex + 0], stateMapData[imgIndex + 1], stateMapData[imgIndex + 2]);
            const UnsignedInteger32 colourInt = pixelColour.ToInteger();

            const ColourRGB riversColour(riverMapData[imgIndex + 0], riverMapData[imgIndex + 1], riverMapData[imgIndex + 2]);
            const UnsignedInteger32 riversColourInt = riversColour.ToInteger();

            imgIndex += 4;

            // Ocean tile
            if (colourInt == 0x00141414) { continue; }

            if (previousColourInt == colourInt) {
                statesVector[previousIndex].AddPixel(x, y);
                statesVector[previousIndex].AddBarrierPixel(riversColourInt);
            }
            else if (stateColourToIndexMap.contains(colourInt)){
                const SizeT stateIndex = stateColourToIndexMap.at(colourInt);
                statesVector[stateIndex].AddPixel(x, y);
                statesVector[stateIndex].AddBarrierPixel(riversColourInt);
                previousColourInt = colourInt;
                previousIndex = stateIndex;
            }
            else {
                const SizeT newIndex = statesVector.size();
                statesVector.emplace_back(pixelColour);
                statesVector[newIndex].AddPixel(x, y);
                statesVector[newIndex].AddBarrierPixel(riversColourInt);
                previousColourInt = colourInt;
                previousIndex = newIndex;
                stateColourToIndexMap[colourInt] = newIndex;
            }
        }
    }

	stbi_image_free(stateMapData);
	stbi_image_free(riverMapData);

	statesVector.shrink_to_fit();
	for (auto& state: statesVector) {
	    state.pixels.shrink_to_fit();
	    state.barrierPixels.shrink_to_fit();
	    state.UpdateBoundaries();
	}

	return statesVector;
}

// ---- Barriers (rivers) ----
// Provinces can't cross barrier pixels: a province's land pixels must stay connected without passing
// through a barrier. Barrier pixels still belong to a province - always one that touches them by a
// full side from land - so provinces on either side of a river share its pixels between them.
enum PixelKind : UnsignedInteger8 {
    LAND = 0,          // ordinary pixel
    BARRIER = 1,       // blocks connections; owned by a province touching it from land
    INNER_BARRIER = 2, // barrier pixel with no land side-neighbour (e.g. a thick river junction);
                       // left out of the optimisation and given to a neighbouring province at the end
};

// Local grid over the state's bounding box: index into state.pixels, or -1 if not in the state
Vector<SignedInteger32> LocalPixelGrid(const State& state) {
    Vector<SignedInteger32> grid(SizeT(state.width) * state.height, -1);
    for (SizeT i = 0; i < state.pixels.size(); ++i)
        grid[SizeT(state.pixels[i].y - state.y0) * state.width + (state.pixels[i].x - state.x0)] = SignedInteger32(i);
    return grid;
}

// Label the 4-connected land areas of the state (barriers and other states separate them).
// Fills regionOf (per state pixel, -1 for non-land) and returns each region's size.
Vector<SizeT> LabelLandRegions(const State& state, const Vector<SignedInteger32>& grid,
                               const Vector<UnsignedInteger8>& kinds, Vector<SignedInteger32>& regionOf) {
    static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
    const SignedInteger32 W = state.width, H = state.height;
    regionOf.assign(state.pixels.size(), -1);
    Vector<SizeT> sizes;
    Vector<SizeT> stack;
    for (SizeT start = 0; start < state.pixels.size(); ++start) {
        if (kinds[start] != LAND || regionOf[start] >= 0) continue;
        const SignedInteger32 id = SignedInteger32(sizes.size());
        sizes.push_back(0);
        regionOf[start] = id;
        stack.push_back(start);
        while (!stack.empty()) {
            const SizeT i = stack.back(); stack.pop_back();
            ++sizes[id];
            const SignedInteger32 x = state.pixels[i].x - state.x0, y = state.pixels[i].y - state.y0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
                if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
                const SignedInteger32 n = grid[SizeT(ny) * W + nx];
                if (n >= 0 && kinds[n] == LAND && regionOf[n] < 0) { regionOf[n] = id; stack.push_back(SizeT(n)); }
            }
        }
    }
    return sizes;
}

// Work out each pixel's PixelKind. A land area smaller than MIN_REGION_SIZE cut off by a barrier
// (e.g. a sliver between a river and the state border) has the barrier pixels next to it opened up,
// merging it with its neighbour across the river rather than forcing a tiny province there.
Vector<UnsignedInteger8> ClassifyPixels(const State& state) {
    static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
    const SignedInteger32 W = state.width, H = state.height;
    const Vector<SignedInteger32> grid = LocalPixelGrid(state);

    Vector<UnsignedInteger8> kinds(state.pixels.size());
    for (SizeT i = 0; i < kinds.size(); ++i) kinds[i] = state.barrierPixels[i] ? BARRIER : LAND;

    auto forEachSideNeighbour = [&](SizeT i, auto&& fn) {
        const SignedInteger32 x = state.pixels[i].x - state.x0, y = state.pixels[i].y - state.y0;
        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
            if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
            const SignedInteger32 n = grid[SizeT(ny) * W + nx];
            if (n >= 0) fn(SizeT(n));
        }
    };

    // Repeat, since a river more than one pixel thick needs opening up one layer at a time
    Vector<SignedInteger32> regionOf;
    for (SizeT round = 0; round < 8; ++round) {
        const Vector<SizeT> sizes = LabelLandRegions(state, grid, kinds, regionOf);
        if (sizes.size() <= 1) break;
        Vector<SizeT> toOpen;
        for (SizeT i = 0; i < kinds.size(); ++i) {
            if (regionOf[i] < 0 || sizes[regionOf[i]] >= MIN_REGION_SIZE) continue;
            forEachSideNeighbour(i, [&](SizeT n) { if (kinds[n] == BARRIER) toOpen.push_back(n); });
        }
        if (toOpen.empty()) break;
        for (const SizeT n : toOpen) kinds[n] = LAND;
    }

    for (SizeT i = 0; i < kinds.size(); ++i) {
        if (kinds[i] != BARRIER) continue;
        Boolean touchesLand = false;
        forEachSideNeighbour(i, [&](SizeT n) { if (kinds[n] == LAND) touchesLand = true; });
        if (!touchesLand) kinds[i] = INNER_BARRIER;
    }
    return kinds;
}

// Pick the province centres: every land area of at least MIN_REGION_SIZE pixels gets
// max(1, round(area / PIXELS_PER_PROVINCE)) of them, placed uniformly at random inside it
// (no attempt to spread them out). Areas split off by a river therefore get their own provinces.
Vector<Pixel> SelectSeeds(const State& state, const Vector<UnsignedInteger8>& kinds, std::mt19937& rng) {
    const Vector<SignedInteger32> grid = LocalPixelGrid(state);
    Vector<SignedInteger32> regionOf;
    const Vector<SizeT> sizes = LabelLandRegions(state, grid, kinds, regionOf);
    if (sizes.empty()) return { state.pixels.front() };

    Vector<Vector<Pixel>> regionPixels(sizes.size());
    for (SizeT i = 0; i < state.pixels.size(); ++i)
        if (regionOf[i] >= 0) regionPixels[regionOf[i]].push_back(state.pixels[i]);

    Vector<Pixel> seeds;
    for (SizeT r = 0; r < sizes.size(); ++r) {
        if (sizes[r] < MIN_REGION_SIZE) continue;
        const SizeT n = std::max<SizeT>(1, SizeT(std::llround(Float64(sizes[r]) / Float64(PIXELS_PER_PROVINCE))));
        std::sample(regionPixels[r].begin(), regionPixels[r].end(), std::back_inserter(seeds), n, rng);
    }
    // A state made only of small areas still gets one province, in its largest area
    if (seeds.empty()) {
        const SizeT largest = SizeT(std::max_element(sizes.begin(), sizes.end()) - sizes.begin());
        std::sample(regionPixels[largest].begin(), regionPixels[largest].end(), std::back_inserter(seeds), 1, rng);
    }
    return seeds;
}

// Starting map: grow every seed outwards over land (4-connected, never through a barrier) until the
// land is shared out, then give each barrier pixel to a province touching it from land
Vector<UnsignedInteger16> AssignProvinces(const State& state, const Vector<UnsignedInteger8>& kinds, const Vector<Pixel>& seeds) {
    static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
    const SignedInteger32 W = state.width, H = state.height;
    const Vector<SignedInteger32> grid = LocalPixelGrid(state);
    constexpr SignedInteger32 UNASSIGNED = -1;

    Vector<SignedInteger32> label(state.pixels.size(), UNASSIGNED);
    std::queue<SizeT> frontier;
    for (SizeT s = 0; s < seeds.size(); ++s) {
        const SizeT i = SizeT(grid[SizeT(seeds[s].y - state.y0) * W + (seeds[s].x - state.x0)]);
        label[i] = SignedInteger32(s);
        frontier.push(i);
    }
    auto sideNeighbour = [&](SizeT i, SignedInteger32 d) -> SignedInteger32 {
        const SignedInteger32 nx = state.pixels[i].x - state.x0 + dx4[d], ny = state.pixels[i].y - state.y0 + dy4[d];
        if (nx < 0 || ny < 0 || nx >= W || ny >= H) return -1;
        return grid[SizeT(ny) * W + nx];
    };

    while (!frontier.empty()) {
        const SizeT i = frontier.front(); frontier.pop();
        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 n = sideNeighbour(i, d);
            if (n >= 0 && kinds[n] == LAND && label[n] == UNASSIGNED) { label[n] = label[i]; frontier.push(SizeT(n)); }
        }
    }
    // Barrier pixels join a province touching them from land
    for (SizeT i = 0; i < label.size(); ++i) {
        if (kinds[i] != BARRIER) continue;
        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 n = sideNeighbour(i, d);
            if (n >= 0 && kinds[n] == LAND && label[n] != UNASSIGNED) { label[i] = label[n]; break; }
        }
    }

    // Fallback for anything still unassigned (a small island with no seed of its own, or an inner
    // barrier pixel): give it to the nearest seed by straight-line distance
    for (SizeT i = 0; i < label.size(); ++i) {
        if (label[i] != UNASSIGNED) continue;
        SignedInteger64 bestDist = -1;
        for (SizeT s = 0; s < seeds.size(); ++s) {
            const SignedInteger64 ddx = SignedInteger64(state.pixels[i].x) - seeds[s].x;
            const SignedInteger64 ddy = SignedInteger64(state.pixels[i].y) - seeds[s].y;
            const SignedInteger64 dist = ddx * ddx + ddy * ddy;
            if (bestDist < 0 || dist < bestDist) { bestDist = dist; label[i] = SignedInteger32(s); }
        }
    }

    Vector<UnsignedInteger16> provinceOf(label.size());
    for (SizeT i = 0; i < label.size(); ++i) provinceOf[i] = UnsignedInteger16(label[i]);
    return provinceOf;
}

Vector<ColourRGB> GenerateNRandomColours(
    const SizeT n,
    const UnsignedInteger8 rMin, const UnsignedInteger8 rMax,
    const UnsignedInteger8 gMin, const UnsignedInteger8 gMax,
    const UnsignedInteger8 bMin, const UnsignedInteger8 bMax,
    std::mt19937& rng
) {
    std::uniform_int_distribution<SignedInteger32> red(rMin, rMax);
    std::uniform_int_distribution<SignedInteger32> green(gMin, gMax);
    std::uniform_int_distribution<SignedInteger32> blue(bMin, bMax);

    std::vector<ColourRGB> colours(n);
    for (auto& c : colours) {
        c = { UnsignedInteger8(red(rng)), UnsignedInteger8(green(rng)), UnsignedInteger8(blue(rng)) };
    }
    return colours;
}

struct Province {
    // When I add per-pixel density weights, this is what that'll measure - but for now,
    // each pixel is just worth 1 value
    SizeT value = 0;
    // Running coordinate sums, so the centre of gravity can be updated in O(1) per flip
    SignedInteger64 sumX = 0, sumY = 0;

    Float64 CentreX() const { return Float64(sumX) / Float64(value); }
    Float64 CentreY() const { return Float64(sumY) / Float64(value); }

    void Add(const SignedInteger64 x, const SignedInteger64 y) {
        value += 1; sumX += x; sumY += y;
    }
    void Remove(const SignedInteger64 x, const SignedInteger64 y) {
        value -= 1; sumX -= x; sumY -= y;
    }
};

// How far a province is from its target size, squared and relative to the target
Float64 SizeError(const SizeT size, const Float64 target) {
    const Float64 e = (Float64(size) - target) / target;
    return e * e;
}

Float64 ScoreMap(const Float64 sizeErrorSum, const Float64 neighbourhoodScore, const SizeT provincesCount) {
    return SIZE_WEIGHT * sizeErrorSum / Float64(provincesCount) + NEIGHBOURHOOD_WEIGHT * neighbourhoodScore;
}

// Smooth value noise in [0, 1]: random values on a lattice, blended with a smoothstep
Float64 ValueNoise(const Float64 x, const Float64 y, const UnsignedInteger32 seed) {
    auto hash = [seed](SignedInteger32 ix, SignedInteger32 iy) {
        UnsignedInteger32 h = UnsignedInteger32(ix) * 374761393u + UnsignedInteger32(iy) * 668265263u + seed * 2246822519u;
        h = (h ^ (h >> 13)) * 1274126177u;
        return Float64(h ^ (h >> 16)) / 4294967295.0;
    };
    const Float64 fx = std::floor(x), fy = std::floor(y);
    const SignedInteger32 ix = SignedInteger32(fx), iy = SignedInteger32(fy);
    Float64 tx = x - fx, ty = y - fy;
    tx = tx * tx * (3.0 - 2.0 * tx);
    ty = ty * ty * (3.0 - 2.0 * ty);
    const Float64 top = hash(ix, iy) + (hash(ix + 1, iy) - hash(ix, iy)) * tx;
    const Float64 bottom = hash(ix, iy + 1) + (hash(ix + 1, iy + 1) - hash(ix, iy + 1)) * tx;
    return top + (bottom - top) * ty;
}

// Several octaves of value noise, each half the wavelength and half the strength of the last
Float64 FractalNoise(const Float64 x, const Float64 y, const UnsignedInteger32 seed) {
    Float64 total = 0.0, amplitude = 1.0, amplitudeSum = 0.0, wavelength = NOISE_WAVELENGTH;
    for (SignedInteger32 o = 0; o < NOISE_OCTAVES; ++o) {
        total += amplitude * ValueNoise(x / wavelength, y / wavelength, seed + UnsignedInteger32(o) * 1013u);
        amplitudeSum += amplitude;
        amplitude *= 0.5;
        wavelength *= 0.5;
    }
    return total / amplitudeSum;
}

// Offsets inside a disc of the given radius, excluding the centre
Vector<std::pair<SignedInteger32, SignedInteger32>> DiscOffsets(const SignedInteger32 r) {
    Vector<std::pair<SignedInteger32, SignedInteger32>> offsets;
    for (SignedInteger32 oy = -r; oy <= r; ++oy)
        for (SignedInteger32 ox = -r; ox <= r; ++ox)
            if ((ox != 0 || oy != 0) && ox * ox + oy * oy <= r * r) offsets.emplace_back(ox, oy);
    return offsets;
}


void CleanProvinces(
    const State& state,
    const Vector<UnsignedInteger8>& kinds,
    Vector<UnsignedInteger16>& pixelProvinceIds,
    const UnsignedInteger16 provincesCount,
    std::mt19937& rng
) {
    const SignedInteger32 W = state.width, H = state.height;
    const SizeT G = SizeT(W) * H;
    const SizeT totalPixels = state.pixels.size();

    // Local grid covering the state's bounding box: province id, or -1 if not in this state
    Vector<SignedInteger32> label(G, -1);
    auto localIndex = [&](SignedInteger32 x, SignedInteger32 y) { return SizeT(y) * W + x; };
    auto inGrid = [&](SignedInteger32 x, SignedInteger32 y) { return x >= 0 && y >= 0 && x < W && y < H; };
    auto labelAt = [&](SignedInteger32 x, SignedInteger32 y) -> SignedInteger32 {
        return inGrid(x, y) ? label[localIndex(x, y)] : -1;
    };
    // Pixel kinds on the same grid (see PixelKind). Inner barrier pixels sit out the optimisation as if
    // they weren't in the state, and are handed to a neighbouring province at the very end.
    Vector<UnsignedInteger8> kindGrid(G, INNER_BARRIER);
    for (SizeT i = 0; i < totalPixels; ++i) {
        const SizeT li = localIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0);
        kindGrid[li] = kinds[i];
        if (kinds[i] != INNER_BARRIER) label[li] = pixelProvinceIds[i];
    }
    auto isLand = [&](SignedInteger32 x, SignedInteger32 y) { return inGrid(x, y) && kindGrid[localIndex(x, y)] == LAND; };
    auto isBarrier = [&](SignedInteger32 x, SignedInteger32 y) { return inGrid(x, y) && kindGrid[localIndex(x, y)] == BARRIER; };

    // 4-connected: provinces only count as touching (and as connected) through full sides
    static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 };
    static const SignedInteger32 dy4[4] = { 0, 1, 0, -1 };

    // The 8 surrounding pixels in clockwise order. Consecutive entries always share a side,
    // and even entries are the 4 side-neighbours.
    static const SignedInteger32 ringX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static const SignedInteger32 ringY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

    // ---- Running totals the score is built from ----
    Vector<Province> provinces(provincesCount);

    // Random target sizes, rescaled so they add up to the state's pixel count
    Vector<Float64> targetSize(provincesCount);
    {
        std::normal_distribution<Float64> normal(0.0, TARGET_SIZE_SPREAD);
        Float64 total = 0.0;
        for (auto& t : targetSize) { t = std::exp(normal(rng)); total += t; }
        for (auto& t : targetSize) t *= Float64(totalPixels) / total;
    }
    Float64 sizeErrorSum = 0.0;
    // Sum over ordered (pixel, pixel-in-disc) pairs in different provinces of the pair's terrain cost
    Float64 neighbourhoodEnergy = 0.0;

    // Terrain cost per pixel (see NOISE_STRENGTH); a pair costs the average of its two pixels
    Vector<Float64> terrainCost(G, 1.0);
    {
        const UnsignedInteger32 noiseSeed = UnsignedInteger32(rng());
        Float64 costSum = 0.0;
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x) {
                Float64 n = FractalNoise(Float64(x + state.x0), Float64(y + state.y0), noiseSeed);
                if (NOISE_RIDGED) n = std::min(1.0, 2.0 * std::abs(2.0 * n - 1.0));
                terrainCost[localIndex(x, y)] = 1.0 + NOISE_STRENGTH * n;
                if (label[localIndex(x, y)] >= 0) costSum += terrainCost[localIndex(x, y)];
            }
        // Rescale so the average in-state cost is 1, keeping the score's overall scale unchanged
        const Float64 meanCost = costSum / Float64(totalPixels);
        for (auto& c : terrainCost) c /= meanCost;
    }

    const auto neighbourhoodOffsets = DiscOffsets(NEIGHBOURHOOD_RADIUS);
    SignedInteger64 crossingsPerUnitBorder = 0; // what a straight border of length 1 adds to the energy
    for (const auto& [ox, oy] : neighbourhoodOffsets) crossingsPerUnitBorder += std::abs(oy);
    const Float64 meanSize = Float64(totalPixels) / Float64(provincesCount);
    const Float64 neighbourhoodScale = 1.0 / (Float64(std::max<SignedInteger64>(1, crossingsPerUnitBorder))
        * Float64(provincesCount) * 2.0 * std::sqrt(PI * meanSize));
    constexpr Boolean useNeighbourhood = NEIGHBOURHOOD_WEIGHT > 0.0;

    // Total pair cost between (x, y) and the pixels in its disc that belong to province `id`
    auto costInDisc = [&](SignedInteger32 x, SignedInteger32 y, SignedInteger32 id) {
        const Float64 own = terrainCost[localIndex(x, y)];
        Float64 c = 0.0;
        for (const auto& [ox, oy] : neighbourhoodOffsets)
            if (labelAt(x + ox, y + oy) == id) c += 0.5 * (own + terrainCost[localIndex(x + ox, y + oy)]);
        return c;
    };

    auto countForeignSides = [&](SignedInteger32 x, SignedInteger32 y) {
        const SignedInteger32 own = label[localIndex(x, y)];
        SignedInteger32 count = 0;
        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 n = labelAt(x + dx4[d], y + dy4[d]);
            if (n >= 0 && n != own) ++count;
        }
        return count;
    };

    // ---- Boundary pixel set (pixels with at least one side touching another province) ----
    // Kept as a flat list plus a position lookup, so add/remove are both O(1)
    Vector<UnsignedInteger32> boundary; boundary.reserve(totalPixels / 4);
    Vector<SignedInteger32> boundaryPos(G, -1);

    auto refreshBoundary = [&](SignedInteger32 x, SignedInteger32 y) {
        if (!inGrid(x, y)) return;
        const SizeT li = localIndex(x, y);
        if (label[li] < 0) return;

        const Boolean isBoundary = countForeignSides(x, y) > 0;
        if (isBoundary && boundaryPos[li] < 0) {
            boundaryPos[li] = SignedInteger32(boundary.size());
            boundary.push_back(UnsignedInteger32(li));
        }
        else if (!isBoundary && boundaryPos[li] >= 0) {
            const SignedInteger32 pos = boundaryPos[li];
            const UnsignedInteger32 last = boundary.back();
            boundary[pos] = last;
            boundaryPos[last] = pos;
            boundary.pop_back();
            boundaryPos[li] = -1;
        }
    };

    // Recompute every running total from the label grid
    auto rebuild = [&]() {
        provinces.assign(provincesCount, Province{});
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x) {
                const SignedInteger32 own = label[localIndex(x, y)];
                if (own >= 0) provinces[own].Add(x, y);
            }

        sizeErrorSum = 0.0;
        for (SizeT i = 0; i < provincesCount; ++i) sizeErrorSum += SizeError(provinces[i].value, targetSize[i]);

        neighbourhoodEnergy = 0.0;
        if (useNeighbourhood) {
            for (SignedInteger32 y = 0; y < H; ++y)
                for (SignedInteger32 x = 0; x < W; ++x) {
                    const SignedInteger32 own = label[localIndex(x, y)];
                    if (own < 0) continue;
                    for (const auto& [ox, oy] : neighbourhoodOffsets) {
                        const SignedInteger32 n = labelAt(x + ox, y + oy);
                        if (n >= 0 && n != own)
                            neighbourhoodEnergy += 0.5 * (terrainCost[localIndex(x, y)] + terrainCost[localIndex(x + ox, y + oy)]);
                    }
                }
        }

        boundary.clear();
        std::fill(boundaryPos.begin(), boundaryPos.end(), -1);
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x)
                refreshBoundary(x, y);
    };

    // ---- Move rules ----
    // Every province is its land pixels, 4-connected without passing through a barrier, plus barrier
    // pixels that each touch one of its land pixels by a full side. These two checks keep that true.

    // Can (x, y) leave province `own`?
    // A land pixel may leave only if the province's land stays in one piece and no barrier pixel of the
    // province is left without a land pixel of it next door. For the first part: removing the pixel can
    // only split the province's land if its same-province land side-neighbours can't reach each other
    // around the 3x3 ring. Walk the ring and count runs of such pixels that contain at least one
    // side-neighbour: more than one run means the flip might split the province, so it's rejected.
    // This is a local test, so it's conservative (it may reject a few safe flips) but never allows a
    // split. A barrier pixel can always leave, since nothing hangs off it.
    auto canRemove = [&](SignedInteger32 x, SignedInteger32 y, SignedInteger32 own) {
        if (provinces[own].value <= 1) return false;
        if (!isLand(x, y)) return true;

        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 bx = x + dx4[d], by = y + dy4[d];
            if (!isBarrier(bx, by) || labelAt(bx, by) != own) continue;
            Boolean stillAttached = false;
            for (SignedInteger32 e = 0; e < 4; ++e) {
                const SignedInteger32 nx = bx + dx4[e], ny = by + dy4[e];
                if ((nx != x || ny != y) && isLand(nx, ny) && labelAt(nx, ny) == own) stillAttached = true;
            }
            if (!stillAttached) return false;
        }

        Boolean inProvince[8];
        SignedInteger32 start = -1;
        for (SignedInteger32 k = 0; k < 8; ++k) {
            inProvince[k] = labelAt(x + ringX[k], y + ringY[k]) == own && isLand(x + ringX[k], y + ringY[k]);
            if (!inProvince[k] && start < 0) start = k;
        }
        if (start < 0) return true; // fully surrounded by its own land; can't be a boundary pixel anyway

        SignedInteger32 runsTouchingSides = 0;
        Boolean inRun = false, runTouchesSide = false;
        for (SignedInteger32 step = 1; step <= 8; ++step) {
            const SignedInteger32 k = (start + step) % 8;
            if (inProvince[k]) {
                if (!inRun) { inRun = true; runTouchesSide = false; }
                if (k % 2 == 0) runTouchesSide = true;
            }
            else if (inRun) {
                inRun = false;
                if (runTouchesSide) ++runsTouchingSides;
            }
        }
        // The walk always ends on `start`, which is not in the province, so every run is closed
        return runsTouchingSides <= 1;
    };

    // Can (x, y) join province `target`? Only if one of target's land pixels touches it by a side -
    // never across a barrier pixel, which is what stops provinces crossing rivers
    auto canJoin = [&](SignedInteger32 x, SignedInteger32 y, SignedInteger32 target) {
        for (SignedInteger32 d = 0; d < 4; ++d)
            if (isLand(x + dx4[d], y + dy4[d]) && labelAt(x + dx4[d], y + dy4[d]) == target) return true;
        return false;
    };

    // ---- Pixel weighting ----
    const Float64 maxWeight = 4.0 * (1.0 + DISTANCE_WEIGHT * MAX_DISTANCE_RATIO);
    auto pixelWeight = [&](SignedInteger32 x, SignedInteger32 y, SignedInteger32 own, SignedInteger32 foreignSides) {
        const Province& p = provinces[own];
        const Float64 ddx = Float64(x) - p.CentreX(), ddy = Float64(y) - p.CentreY();
        const Float64 radius = std::max(1.0, std::sqrt(Float64(p.value) / PI));
        const Float64 ratio = std::min(MAX_DISTANCE_RATIO, std::sqrt(ddx * ddx + ddy * ddy) / radius);
        return Float64(foreignSides) * (1.0 + DISTANCE_WEIGHT * ratio);
    };

    std::uniform_real_distribution<Float64> unit(0.0, 1.0);

    // ---- The flip loop ----
    // Score change of adding one pixel of straight border, the unit temperatures are measured in
    const Float64 borderPixelScore = NEIGHBOURHOOD_WEIGHT * neighbourhoodScale * 2.0 * Float64(crossingsPerUnitBorder);

    auto anneal = [&](const SizeT iterations, const Float64 temperatureStart, const Float64 temperatureEnd) {
        for (SizeT it = 0; it < iterations; ++it) {
            if (boundary.empty()) break;

            // Weighted pick via rejection sampling: choose a boundary pixel uniformly, keep it
            // with probability weight / maxWeight. Avoids re-weighting every pixel after each
            // flip, since every flip nudges two province centres.
            SignedInteger32 x = 0, y = 0, own = -1, foreignSides = 0;
            std::uniform_int_distribution<SizeT> pickBoundary(0, boundary.size() - 1);
            for (SignedInteger32 attempt = 0; attempt < 64; ++attempt) {
                const UnsignedInteger32 li = boundary[pickBoundary(rng)];
                x = SignedInteger32(li % W); y = SignedInteger32(li / W);
                own = label[li];
                foreignSides = countForeignSides(x, y);
                if (unit(rng) * maxWeight < pixelWeight(x, y, own, foreignSides)) break;
            }

            // Pick which neighbouring province it flips to - a random foreign side, so a province
            // touching two sides of the pixel is twice as likely as one touching one side. Only sides
            // touching another province's land count: a province can't reach across a barrier pixel.
            SignedInteger32 candidates[4], candidateCount = 0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 n = labelAt(x + dx4[d], y + dy4[d]);
                if (n >= 0 && n != own && isLand(x + dx4[d], y + dy4[d])) candidates[candidateCount++] = n;
            }
            if (candidateCount == 0) continue;
            const SignedInteger32 target = candidates[std::uniform_int_distribution<SignedInteger32>(0, candidateCount - 1)(rng)];

            // Provinces may not vanish, split or cross a barrier
            if (!canRemove(x, y, own)) continue;

            // ---- Build the two changed provinces as they'd be after the flip ----
            Province newOwn = provinces[own], newTarget = provinces[target];
            newOwn.Remove(x, y);
            newTarget.Add(x, y);

            // ---- Score old vs new map ----
            const Float64 newSizeErrorSum = sizeErrorSum
                + SizeError(newOwn.value, targetSize[own]) + SizeError(newTarget.value, targetSize[target])
                - SizeError(provinces[own].value, targetSize[own]) - SizeError(provinces[target].value, targetSize[target]);
            // Flipping p from own to target: every pair (p, q) and (q, p) with q in own becomes a
            // border pair, and every pair with q in target stops being one
            Float64 newNeighbourhoodEnergy = neighbourhoodEnergy;
            if (useNeighbourhood)
                newNeighbourhoodEnergy += 2.0 * (costInDisc(x, y, own) - costInDisc(x, y, target));

            const Float64 oldScore = ScoreMap(sizeErrorSum, neighbourhoodEnergy * neighbourhoodScale, provincesCount);
            const Float64 newScore = ScoreMap(newSizeErrorSum, newNeighbourhoodEnergy * neighbourhoodScale, provincesCount);

            if (newScore > oldScore) {
                const Float64 progress = Float64(it) / Float64(iterations);
                const Float64 temperature = temperatureStart * std::pow(temperatureEnd / temperatureStart, progress);
                if (temperature <= 0.0 || unit(rng) >= std::exp(-(newScore - oldScore) / (temperature * borderPixelScore))) continue;
            }

            // ---- Apply the flip ----
            label[localIndex(x, y)] = target;
            provinces[own] = newOwn;
            provinces[target] = newTarget;
            sizeErrorSum = newSizeErrorSum;
            neighbourhoodEnergy = newNeighbourhoodEnergy;

            refreshBoundary(x, y);
            for (SignedInteger32 d = 0; d < 4; ++d) refreshBoundary(x + dx4[d], y + dy4[d]);
        }
    };

    // ---- Cleanup helpers ----

    // Mark (in `remove`) every pixel that isn't part of its province's main piece: the largest
    // 4-connected piece of the province's `include`d land pixels (never linked through a barrier), plus
    // the `include`d barrier pixels touching that piece from land
    auto markAllButLargestPiece = [&](const Vector<UnsignedInteger8>& include, Vector<UnsignedInteger8>& remove) {
        Vector<SignedInteger32> piece(G, -1);
        Vector<SizeT> pieceSize;
        Vector<SignedInteger32> pieceProvince;
        Vector<UnsignedInteger32> stack;
        for (SizeT li = 0; li < G; ++li) {
            if (!include[li] || piece[li] >= 0 || label[li] < 0 || kindGrid[li] != LAND) continue;
            const SignedInteger32 id = SignedInteger32(pieceSize.size());
            pieceSize.push_back(0);
            pieceProvince.push_back(label[li]);
            piece[li] = id;
            stack.push_back(UnsignedInteger32(li));
            while (!stack.empty()) {
                const UnsignedInteger32 c = stack.back(); stack.pop_back();
                pieceSize[id] += 1;
                const SignedInteger32 cx = SignedInteger32(c % W), cy = SignedInteger32(c / W);
                for (SignedInteger32 d = 0; d < 4; ++d) {
                    const SignedInteger32 nx = cx + dx4[d], ny = cy + dy4[d];
                    if (!isLand(nx, ny)) continue;
                    const SizeT ni = localIndex(nx, ny);
                    if (include[ni] && piece[ni] < 0 && label[ni] == label[li]) {
                        piece[ni] = id;
                        stack.push_back(UnsignedInteger32(ni));
                    }
                }
            }
        }
        Vector<SignedInteger32> largest(provincesCount, -1);
        for (SignedInteger32 id = 0; id < SignedInteger32(pieceSize.size()); ++id) {
            SignedInteger32& best = largest[pieceProvince[id]];
            if (best < 0 || pieceSize[id] > pieceSize[best]) best = id;
        }
        for (SizeT li = 0; li < G; ++li) {
            const SignedInteger32 own = label[li];
            if (own < 0) continue;
            if (kindGrid[li] == LAND) {
                if (!include[li] || piece[li] != largest[own]) remove[li] = 1;
                continue;
            }
            // Barrier pixel: kept only if it touches the main piece from land
            Boolean attached = false;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
                if (isLand(nx, ny) && piece[localIndex(nx, ny)] >= 0 && piece[localIndex(nx, ny)] == largest[own]) attached = true;
            }
            if (!include[li] || !attached) remove[li] = 1;
        }
    };

    // Give every barrier pixel that no longer touches its own province's land to the neighbouring
    // province with the most land sides against it (never back to `avoid`, if given)
    auto reattachBarrierPixels = [&](const Vector<UnsignedInteger8>* avoid) -> SizeT {
        SizeT moved = 0;
        for (SizeT li = 0; li < G; ++li) {
            if (label[li] < 0 || kindGrid[li] != BARRIER) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            const Boolean mustLeave = avoid && (*avoid)[li];
            if (!mustLeave && canJoin(x, y, label[li])) continue;
            SignedInteger32 best = -1, bestCount = 0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                if (!isLand(x + dx4[d], y + dy4[d])) continue;
                const SignedInteger32 n = labelAt(x + dx4[d], y + dy4[d]);
                if (mustLeave && n == label[li]) continue;
                SignedInteger32 count = 0;
                for (SignedInteger32 e = 0; e < 4; ++e)
                    if (isLand(x + dx4[e], y + dy4[e]) && labelAt(x + dx4[e], y + dy4[e]) == n) ++count;
                if (count > bestCount) { best = n; bestCount = count; }
            }
            if (best >= 0 && best != label[li]) { label[li] = best; ++moved; }
        }
        return moved;
    };

    // Hand every marked pixel to a neighbouring province (never back to its own), spreading out over
    // land from the unmarked pixels with a flood fill so every province gaining pixels stays in one piece
    // and nothing crosses a barrier. Marked barrier pixels, and any barrier pixels whose land neighbours
    // changed hands, then go to a province touching them from land. Pixels the fill can't reach (e.g. an
    // island with no other province on it) are left alone.
    auto reassignPixels = [&](const Vector<UnsignedInteger8>& remove) -> SizeT {
        Vector<SignedInteger32> newLabel(G, -1);
        std::queue<UnsignedInteger32> frontier;
        for (SizeT li = 0; li < G; ++li) {
            if (!remove[li] || kindGrid[li] != LAND) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            // Prefer the neighbouring province touching the most sides
            SignedInteger32 best = -1, bestCount = 0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
                if (!isLand(nx, ny)) continue;
                const SizeT ni = localIndex(nx, ny);
                const SignedInteger32 n = label[ni];
                if (n < 0 || remove[ni] || n == label[li]) continue;
                SignedInteger32 count = 0;
                for (SignedInteger32 e = 0; e < 4; ++e) {
                    const SignedInteger32 mx = x + dx4[e], my = y + dy4[e];
                    if (isLand(mx, my) && !remove[localIndex(mx, my)] && label[localIndex(mx, my)] == n) ++count;
                }
                if (count > bestCount) { best = n; bestCount = count; }
            }
            if (best >= 0) { newLabel[li] = best; frontier.push(UnsignedInteger32(li)); }
        }
        while (!frontier.empty()) {
            const UnsignedInteger32 c = frontier.front(); frontier.pop();
            const SignedInteger32 cx = SignedInteger32(c % W), cy = SignedInteger32(c / W);
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 nx = cx + dx4[d], ny = cy + dy4[d];
                if (!isLand(nx, ny)) continue;
                const SizeT ni = localIndex(nx, ny);
                if (remove[ni] && newLabel[ni] < 0 && label[ni] != newLabel[c]) {
                    newLabel[ni] = newLabel[c];
                    frontier.push(UnsignedInteger32(ni));
                }
            }
        }
        SizeT moved = 0;
        for (SizeT li = 0; li < G; ++li)
            if (remove[li] && newLabel[li] >= 0) { label[li] = newLabel[li]; ++moved; }
        return moved + reattachBarrierPixels(&remove);
    };

    const auto thinOffsets = DiscOffsets(THIN_RADIUS);

    // Majority-filter smoothing (see SMOOTHNESS). Pixels are visited in random order and updated one at a
    // time, so every switch is checked against the current map and can't split a province.
    auto smoothBorders = [&]() {
        // Own generator, seeded with exactly one draw whatever SMOOTHNESS is - so changing SMOOTHNESS
        // smooths the same map differently instead of changing every state generated after this one
        std::mt19937 smoothRng{ UnsignedInteger32(rng()) };
        if (SMOOTHNESS <= 0.0) return;
        constexpr SizeT smoothingPasses = 3;
        Vector<std::pair<SignedInteger32, SignedInteger32>> disc;
        const SignedInteger32 r = SignedInteger32(std::ceil(SMOOTHNESS));
        for (SignedInteger32 oy = -r; oy <= r; ++oy)
            for (SignedInteger32 ox = -r; ox <= r; ++ox)
                if (Float64(ox * ox + oy * oy) <= SMOOTHNESS * SMOOTHNESS + 1e-9) disc.emplace_back(ox, oy);

        Vector<UnsignedInteger32> order;
        for (SizeT li = 0; li < G; ++li) if (label[li] >= 0) order.push_back(UnsignedInteger32(li));
        Vector<SignedInteger32> votes(provincesCount, 0);
        Vector<SignedInteger32> touched;

        for (SizeT pass = 0; pass < smoothingPasses; ++pass) {
            std::shuffle(order.begin(), order.end(), smoothRng);
            SizeT changed = 0;
            for (const UnsignedInteger32 li : order) {
                const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W), own = label[li];
                if (countForeignSides(x, y) == 0) continue; // not on a border

                // Count the disc's pixels per province (the pixel itself included)
                for (const auto& [ox, oy] : disc) {
                    const SignedInteger32 n = labelAt(x + ox, y + oy);
                    if (n < 0) continue;
                    if (votes[n] == 0) touched.push_back(n);
                    ++votes[n];
                }
                // The winner must touch the pixel by a side from land, so it stays in one piece after
                // gaining it and never reaches across a barrier
                SignedInteger32 best = own;
                for (SignedInteger32 d = 0; d < 4; ++d) {
                    const SignedInteger32 n = labelAt(x + dx4[d], y + dy4[d]);
                    if (n >= 0 && isLand(x + dx4[d], y + dy4[d]) && votes[n] > votes[best]) best = n;
                }
                for (const SignedInteger32 n : touched) votes[n] = 0;
                touched.clear();

                if (best == own || !canRemove(x, y, own)) continue;
                label[li] = best;
                provinces[own].Remove(x, y);
                provinces[best].Add(x, y);
                ++changed;
            }
            if (changed == 0) break;
        }
    };

    // Share river pixels between the provinces on either side. A "stretch" is a run of barrier pixels
    // whose land neighbours are exactly two provinces, A and B. Its pixels are put in order along the
    // river and split in half - one half to each province - so the border crosses the river only once
    // per stretch. Every such pixel touches both provinces' land, so either can own it.
    auto splitRiverStretches = [&]() {
        std::mt19937 splitRng{ UnsignedInteger32(rng()) };
        Vector<SignedInteger32> pairA(G, -1), pairB(G, -1);
        for (SizeT li = 0; li < G; ++li) {
            if (label[li] < 0 || kindGrid[li] != BARRIER) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            SignedInteger32 a = -1, b = -1;
            Boolean tooMany = false;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                if (!isLand(x + dx4[d], y + dy4[d])) continue;
                const SignedInteger32 n = labelAt(x + dx4[d], y + dy4[d]);
                if (n == a || n == b) continue;
                if (a < 0) a = n; else if (b < 0) b = n; else tooMany = true;
            }
            if (b < 0 || tooMany) continue;
            pairA[li] = std::min(a, b);
            pairB[li] = std::max(a, b);
        }

        Vector<SignedInteger32> distance(G, -1);
        Vector<UnsignedInteger32> stretch, queue;
        for (SizeT start = 0; start < G; ++start) {
            if (pairA[start] < 0 || distance[start] >= 0) continue;
            const SignedInteger32 a = pairA[start], b = pairB[start];
            auto sameStretch = [&](SignedInteger32 x, SignedInteger32 y) {
                return inGrid(x, y) && pairA[localIndex(x, y)] == a && pairB[localIndex(x, y)] == b;
            };

            // Collect the stretch (8-connected, since rivers can step diagonally)
            stretch.clear();
            stretch.push_back(UnsignedInteger32(start));
            distance[start] = 0;
            for (SizeT k = 0; k < stretch.size(); ++k) {
                const SignedInteger32 x = SignedInteger32(stretch[k] % W), y = SignedInteger32(stretch[k] / W);
                for (SignedInteger32 r = 0; r < 8; ++r)
                    if (sameStretch(x + ringX[r], y + ringY[r]) && distance[localIndex(x + ringX[r], y + ringY[r])] < 0) {
                        distance[localIndex(x + ringX[r], y + ringY[r])] = 0;
                        stretch.push_back(UnsignedInteger32(localIndex(x + ringX[r], y + ringY[r])));
                    }
            }
            if (stretch.size() < 2) continue;

            // Order it along the river: breadth-first distance from its far end. The pixel furthest
            // from an arbitrary pixel is one end; distances measured from there run along the river.
            auto measureFrom = [&](UnsignedInteger32 origin) {
                for (const UnsignedInteger32 li : stretch) distance[li] = -1;
                queue.clear();
                queue.push_back(origin);
                distance[origin] = 0;
                for (SizeT k = 0; k < queue.size(); ++k) {
                    const SignedInteger32 x = SignedInteger32(queue[k] % W), y = SignedInteger32(queue[k] / W);
                    for (SignedInteger32 r = 0; r < 8; ++r) {
                        const SignedInteger32 nx = x + ringX[r], ny = y + ringY[r];
                        if (sameStretch(nx, ny) && distance[localIndex(nx, ny)] < 0) {
                            distance[localIndex(nx, ny)] = distance[queue[k]] + 1;
                            queue.push_back(UnsignedInteger32(localIndex(nx, ny)));
                        }
                    }
                }
                return queue.back();
            };
            measureFrom(measureFrom(stretch.front()));
            std::sort(stretch.begin(), stretch.end(),
                      [&](UnsignedInteger32 p, UnsignedInteger32 q) { return distance[p] < distance[q]; });

            // First half to one province, second half to the other (which way round is random)
            const Boolean aFirst = (splitRng() & 1u) != 0;
            for (SizeT k = 0; k < stretch.size(); ++k) {
                const SizeT li = stretch[k];
                const SignedInteger32 newOwner = ((k < stretch.size() / 2) == aFirst) ? a : b;
                if (newOwner == label[li]) continue;
                const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
                provinces[label[li]].Remove(x, y);
                provinces[newOwner].Add(x, y);
                label[li] = newOwner;
            }
        }
    };

    // Cut off every part of a province that a disc of radius THIN_RADIUS can't fit inside, plus anything
    // only connected to the rest of the province through such a part. Other states, sea and barrier
    // pixels count as "inside" here, so a province isn't punished for a coastline, state border or river
    // it can't change. (Barrier pixels are kept or cut along with the land they touch.)
    auto removeThinParts = [&]() -> SizeT {
        Vector<UnsignedInteger8> kept(G, 0), remove(G, 0);
        Vector<UnsignedInteger8> hasCore(provincesCount, 0);
        Vector<UnsignedInteger32> corePixels;
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x) {
                const SignedInteger32 own = label[localIndex(x, y)];
                if (own < 0 || !isLand(x, y)) continue;
                Boolean isCore = true;
                for (const auto& [ox, oy] : thinOffsets) {
                    const SignedInteger32 n = labelAt(x + ox, y + oy);
                    if (n >= 0 && n != own && isLand(x + ox, y + oy)) { isCore = false; break; }
                }
                if (isCore) { corePixels.push_back(UnsignedInteger32(localIndex(x, y))); hasCore[own] = 1; }
            }
        // Everything a core disc covers is part of the province's thick body
        for (const UnsignedInteger32 li : corePixels) {
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W), own = label[li];
            kept[li] = 1;
            for (const auto& [ox, oy] : thinOffsets)
                if (labelAt(x + ox, y + oy) == own) kept[localIndex(x + ox, y + oy)] = 1;
        }
        // Barrier pixels are judged by the land they touch (see markAllButLargestPiece)
        for (SizeT li = 0; li < G; ++li)
            if (label[li] >= 0 && kindGrid[li] == BARRIER) kept[li] = 1;
        // Provinces too small or thin to have any core are left as they are
        for (SizeT li = 0; li < G; ++li)
            if (label[li] >= 0 && !hasCore[label[li]]) kept[li] = 1;

        markAllButLargestPiece(kept, remove);
        return reassignPixels(remove);
    };

    // Give away every piece of a province that isn't its largest 4-connected piece
    auto removeDisconnectedPieces = [&]() -> SizeT {
        Vector<UnsignedInteger8> include(G, 0), remove(G, 0);
        for (SizeT li = 0; li < G; ++li) include[li] = label[li] >= 0;
        markAllButLargestPiece(include, remove);
        return reassignPixels(remove);
    };

    // ---- Run ----
    if (provincesCount < 2) return;

    reattachBarrierPixels(nullptr);
    rebuild();
    anneal(ITERATIONS_PER_PIXEL * totalPixels, TEMPERATURE_START, TEMPERATURE_END);

    for (SizeT pass = 0; pass < CLEANUP_PASSES; ++pass) {
        if (removeThinParts() + removeDisconnectedPieces() == 0) break;
        rebuild();
        anneal(CLEANUP_ITERATIONS_PER_PIXEL * totalPixels, TEMPERATURE_END, TEMPERATURE_END);
    }

    smoothBorders();
    splitRiverStretches();

    // Inner barrier pixels go to the most common province among their 8 neighbours
    for (SizeT pass = 0; pass < 16; ++pass) {
        Boolean anyLeft = false;
        for (SizeT li = 0; li < G; ++li) {
            if (label[li] >= 0 || kindGrid[li] != INNER_BARRIER) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            SignedInteger32 best = -1, bestCount = 0;
            for (SignedInteger32 k = 0; k < 8; ++k) {
                const SignedInteger32 n = labelAt(x + ringX[k], y + ringY[k]);
                if (n < 0) continue;
                SignedInteger32 count = 0;
                for (SignedInteger32 m = 0; m < 8; ++m) if (labelAt(x + ringX[m], y + ringY[m]) == n) ++count;
                if (count > bestCount) { best = n; bestCount = count; }
            }
            if (best >= 0) label[li] = best; else anyLeft = true;
        }
        if (!anyLeft) break;
    }

    // Write the result back (anything still unlabelled keeps its starting province)
    for (SizeT i = 0; i < totalPixels; ++i) {
        const SignedInteger32 l = label[localIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0)];
        if (l >= 0) pixelProvinceIds[i] = UnsignedInteger16(l);
    }
}

int main() {
    Timestamp startTime = std::chrono::high_resolution_clock::now();

    SignedInteger32 mapWidth{}, mapHeight{};
    Vector<State> statesVector = LoadStates(mapWidth, mapHeight);

    const SizeT provincesMapDataSize = mapWidth * mapHeight * 3;
    UnsignedInteger8 *provincesMapData = new UnsignedInteger8[provincesMapDataSize];
    std::fill_n(provincesMapData, provincesMapDataSize, 20);

    std::mt19937 rng(std::random_device{}());
    for (auto& state : statesVector) {
        // Work out which pixels are barriers (rivers), then pick random points in each state - per
        // separate land area - to be our province centres
        const Vector<UnsignedInteger8> pixelKinds = ClassifyPixels(state);
        Vector<Pixel> randomPixels = SelectSeeds(state, pixelKinds, rng);
        const UnsignedInteger16 provincesCount = randomPixels.size();
        
        // Given our semi-random points on the map, assign each pixel in the state to it's nearest point
        Vector<UnsignedInteger16> pixelProvinceIds = AssignProvinces(state, pixelKinds, randomPixels);

        // Balance province sizes and shapes by flipping border pixels between neighbouring provinces
        CleanProvinces(state, pixelKinds, pixelProvinceIds, provincesCount, rng);

        // Colour in the provinces and print the map
        /*
        Vector<ColourRGB> colours = GenerateNRandomColours(
            provincesCount,
            std::max(0, SignedInteger32(state.colour.r) - 25), std::min(255, SignedInteger32(state.colour.r) + 25),
            std::max(0, SignedInteger32(state.colour.g) - 25), std::min(255, SignedInteger32(state.colour.g) + 25),
            std::max(0, SignedInteger32(state.colour.b) - 25), std::min(255, SignedInteger32(state.colour.b) + 25),
            rng
        );
        */

        Vector<ColourRGB> colours = GenerateNRandomColours(
            provincesCount,
            0, 255,
            0, 255,
            0, 255,
            rng
        );

        for (SizeT i = 0; i < state.pixels.size(); i++) {
            UnsignedInteger16 localId = pixelProvinceIds[i];
            SizeT index = ((state.pixels[i].y * mapWidth) + state.pixels[i].x) * 3;
            provincesMapData[index + 0] = colours[localId].r;
            provincesMapData[index + 1] = colours[localId].g;
            provincesMapData[index + 2] = colours[localId].b;
        }
    }

    stbi_write_png("out/provinces.png", mapWidth, mapHeight, 3, provincesMapData, mapWidth * 3);
    delete[] provincesMapData;

    std::cout << std::format("Processed statemap in {0}\n", GetTimeElapsedFromStart(startTime));
    return 0;
}
