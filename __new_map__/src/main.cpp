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

// Average province size in pixels (each state gets max(1, statePixels / PIXELS_PER_PROVINCE) provinces)
constexpr SizeT PIXELS_PER_PROVINCE = 500;

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

// ======================================================================

struct Pixel {
    UnsignedInteger16 x, y;
};

struct State {
    ColourRGB colour;
    UnsignedInteger16 x0 = UINT16_MAX, x1 = 0, y0 = UINT16_MAX, y1 = 0;
    UnsignedInteger16 width = 0, height = 0;
    Vector<Pixel> pixels;

    State(): colour(), pixels() { pixels.reserve(6000); }
    State(const ColourRGB colour): colour(colour), pixels() { pixels.reserve(6000); }

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
    void UpdateBoundaries() {
        width = x1 - x0 + 1;
        height = y1 - y0 + 1;
    }
};

Vector<State> LoadStates(SignedInteger32& mapWidth, SignedInteger32& mapHeight){
    Vector<State> statesVector; statesVector.reserve(1600);

    SignedInteger32 stateMapChannels{};
    UnsignedInteger8 *stateMapData = stbi_load("in/statemap.png", &mapWidth, &mapHeight, &stateMapChannels, 4);

    HashMap<UnsignedInteger32, UnsignedInteger16> stateColourToIndexMap;

    SizeT imgIndex = 0;
    // Cache most recently found state
    UnsignedInteger32 previousColourInt = 0;
    SizeT previousIndex = 0;

    for (SizeT y = 0; y < mapHeight; y += 1) {
        for (SizeT x = 0; x < mapWidth; x += 1) {
            const ColourRGB pixelColour(stateMapData[imgIndex + 0], stateMapData[imgIndex + 1], stateMapData[imgIndex + 2]);
            const UnsignedInteger32 colourInt = pixelColour.ToInteger();

            imgIndex += 4;

            // Ocean tile
            if (colourInt == 0x00141414) { continue; }

            if (previousColourInt == colourInt) {
                statesVector[previousIndex].AddPixel(x, y);
            }
            else if (stateColourToIndexMap.contains(colourInt)){
                const SizeT stateIndex = stateColourToIndexMap.at(colourInt);
                statesVector[stateIndex].AddPixel(x, y);
                previousColourInt = colourInt;
                previousIndex = stateIndex;
            }
            else {
                const SizeT newIndex = statesVector.size();
                statesVector.emplace_back(pixelColour);
                statesVector[newIndex].AddPixel(x, y);
                previousColourInt = colourInt;
                previousIndex = newIndex;
                stateColourToIndexMap[colourInt] = newIndex;
            }
        }
    }

	stbi_image_free(stateMapData);

	statesVector.shrink_to_fit();
	for (auto& state: statesVector) {
	    state.pixels.shrink_to_fit();
	    state.UpdateBoundaries();
	}

	return statesVector;
}

// Pick n distinct pixels from the state, uniformly at random (no attempt to spread them out)
Vector<Pixel> SelectRandomPixels(const State& state, SizeT n, std::mt19937& rng) {
    const auto& pixels = state.pixels;
    if (n == 0 || pixels.empty()) return {};
    n = std::min(n, pixels.size());

    Vector<Pixel> randomPixels;
    randomPixels.reserve(n);
    std::sample(pixels.begin(), pixels.end(), std::back_inserter(randomPixels), n, rng);
    return randomPixels;
}

Vector<UnsignedInteger16> AssignProvinces(const State& state, const Vector<Pixel>& seeds) {
    const auto& pixels = state.pixels;

    constexpr int32_t NOT_IN_STATE = -1;
    constexpr int32_t UNASSIGNED   = -2;

    // Create a vector of size state width * height that functions as a look up table
    // for whether a province is done or not
    Vector<SignedInteger32> label(SizeT(state.width) * state.height, NOT_IN_STATE);
    auto localIndex = [&](UnsignedInteger16 x, UnsignedInteger16 y) {
        return SizeT(y - state.y0) * state.width + (x - state.x0);
    };

    for (const auto& p : pixels) label[localIndex(p.x, p.y)] = UNASSIGNED;

    std::queue<std::pair<UnsignedInteger16, UnsignedInteger16>> frontier;
    for (UnsignedInteger32 s = 0; s < seeds.size(); ++s) {
        label[localIndex(seeds[s].x, seeds[s].y)] = int32_t(s);
        frontier.push({seeds[s].x, seeds[s].y});
    }

    // 8-connected so boundaries can run diagonally instead of stair-stepping
    static const int dx[8] = {1,-1,0,0,1,1,-1,-1};
    static const int dy[8] = {0,0,1,-1,1,-1,1,-1};

    while (!frontier.empty()) {
        auto [x, y] = frontier.front(); frontier.pop();
        int32_t myLabel = label[localIndex(x, y)];

        for (SignedInteger32 d = 0; d < 8; ++d) {
            SignedInteger32 nx = SignedInteger32(x) + dx[d], ny = SignedInteger32(y) + dy[d];
            if (nx < SignedInteger32(state.x0) || nx > SignedInteger32(state.x1) ||
                ny < SignedInteger32(state.y0) || ny > SignedInteger32(state.y1)) continue;

            SizeT ni = localIndex(UnsignedInteger16(nx), UnsignedInteger16(ny));
            if (label[ni] == UNASSIGNED) {
                label[ni] = myLabel;
                frontier.push({UnsignedInteger16(nx), UnsignedInteger16(ny)});
            }
        }
    }

    // Fallback: a disconnected chunk of the state (e.g. a small island with
    // no seed of its own) never gets reached by the BFS and stays UNASSIGNED.
    // Give those leftovers to their nearest seed by straight-line distance.
    for (const auto& p : pixels) {
        SizeT li = localIndex(p.x, p.y);
        if (label[li] != UNASSIGNED) continue;
        uint32_t best = 0; long bestDist = -1;
        for (uint32_t s = 0; s < seeds.size(); ++s) {
            long ddx = long(p.x) - long(seeds[s].x);
            long ddy = long(p.y) - long(seeds[s].y);
            long dist = ddx*ddx + ddy*ddy;
            if (bestDist < 0 || dist < bestDist) { bestDist = dist; best = s; }
        }
        label[li] = SignedInteger32(best);
    }

    Vector<UnsignedInteger16> regionOf(pixels.size());
    for (SizeT i = 0; i < pixels.size(); ++i)
        regionOf[i] = UnsignedInteger16(label[localIndex(pixels[i].x, pixels[i].y)]);
    return regionOf;
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
    for (SizeT i = 0; i < totalPixels; ++i)
        label[localIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0)] = pixelProvinceIds[i];

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

    // ---- Split check ----
    // Removing a pixel can only split its province if its same-province side-neighbours
    // can't reach each other around the 3x3 ring. Walk the ring and count runs of
    // same-province pixels that contain at least one side-neighbour: more than one such
    // run means the flip might split the province, so it's rejected. This is a local test,
    // so it's conservative (it may reject a few safe flips) but never allows a split.
    auto removalKeepsConnected = [&](SignedInteger32 x, SignedInteger32 y, SignedInteger32 own) {
        Boolean inProvince[8];
        SignedInteger32 start = -1;
        for (SignedInteger32 k = 0; k < 8; ++k) {
            inProvince[k] = labelAt(x + ringX[k], y + ringY[k]) == own;
            if (!inProvince[k] && start < 0) start = k;
        }
        if (start < 0) return true; // fully surrounded; can't be a boundary pixel anyway

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
            // touching two sides of the pixel is twice as likely as one touching one side
            std::uniform_int_distribution<SignedInteger32> pickSide(0, foreignSides - 1);
            SignedInteger32 sideIndex = pickSide(rng), target = -1;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 n = labelAt(x + dx4[d], y + dy4[d]);
                if (n >= 0 && n != own && sideIndex-- == 0) { target = n; break; }
            }

            // Provinces may not vanish or split
            if (provinces[own].value <= 1) continue;
            if (!removalKeepsConnected(x, y, own)) continue;

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

    // Mark (in `remove`) every pixel that isn't in its province's largest 4-connected piece of `include`
    auto markAllButLargestPiece = [&](const Vector<UnsignedInteger8>& include, Vector<UnsignedInteger8>& remove) {
        Vector<SignedInteger32> piece(G, -1);
        Vector<SizeT> pieceSize;
        Vector<SignedInteger32> pieceProvince;
        Vector<UnsignedInteger32> stack;
        for (SizeT li = 0; li < G; ++li) {
            if (!include[li] || piece[li] >= 0) continue;
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
                    if (!inGrid(nx, ny)) continue;
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
        for (SizeT li = 0; li < G; ++li)
            if (label[li] >= 0 && (!include[li] || piece[li] != largest[label[li]])) remove[li] = 1;
    };

    // Hand every marked pixel to a neighbouring province (never back to its own), spreading out from
    // the unmarked pixels with a flood fill so every province gaining pixels stays in one piece.
    // Pixels the fill can't reach (e.g. an island with no other province on it) are left alone.
    auto reassignPixels = [&](const Vector<UnsignedInteger8>& remove) -> SizeT {
        Vector<SignedInteger32> newLabel(G, -1);
        std::queue<UnsignedInteger32> frontier;
        for (SizeT li = 0; li < G; ++li) {
            if (!remove[li]) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            // Prefer the neighbouring province touching the most sides
            SignedInteger32 best = -1, bestCount = 0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
                if (!inGrid(nx, ny)) continue;
                const SizeT ni = localIndex(nx, ny);
                const SignedInteger32 n = label[ni];
                if (n < 0 || remove[ni] || n == label[li]) continue;
                SignedInteger32 count = 0;
                for (SignedInteger32 e = 0; e < 4; ++e) {
                    const SignedInteger32 mx = x + dx4[e], my = y + dy4[e];
                    if (inGrid(mx, my) && !remove[localIndex(mx, my)] && label[localIndex(mx, my)] == n) ++count;
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
                if (!inGrid(nx, ny)) continue;
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
        return moved;
    };

    const auto thinOffsets = DiscOffsets(THIN_RADIUS);

    // Cut off every part of a province that a disc of radius thinRadius can't fit inside, plus anything
    // only connected to the rest of the province through such a part. Other states and sea count as
    // "inside" here, so a province isn't punished for a coastline or state border it can't change.
    auto removeThinParts = [&]() -> SizeT {
        Vector<UnsignedInteger8> kept(G, 0), remove(G, 0);
        Vector<UnsignedInteger8> hasCore(provincesCount, 0);
        Vector<UnsignedInteger32> corePixels;
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x) {
                const SignedInteger32 own = label[localIndex(x, y)];
                if (own < 0) continue;
                Boolean isCore = true;
                for (const auto& [ox, oy] : thinOffsets) {
                    const SignedInteger32 n = labelAt(x + ox, y + oy);
                    if (n >= 0 && n != own) { isCore = false; break; }
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

    rebuild();
    anneal(ITERATIONS_PER_PIXEL * totalPixels, TEMPERATURE_START, TEMPERATURE_END);

    for (SizeT pass = 0; pass < CLEANUP_PASSES; ++pass) {
        if (removeThinParts() + removeDisconnectedPieces() == 0) break;
        rebuild();
        anneal(CLEANUP_ITERATIONS_PER_PIXEL * totalPixels, TEMPERATURE_END, TEMPERATURE_END);
    }

    // Write the result back
    for (SizeT i = 0; i < totalPixels; ++i)
        pixelProvinceIds[i] = UnsignedInteger16(label[localIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0)]);
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
        // Select random points in each state to be our province centers
        SizeT n = std::max<SizeT>(1, state.pixels.size() / PIXELS_PER_PROVINCE);
        Vector<Pixel> randomPixels = SelectRandomPixels(state, n, rng);
        const UnsignedInteger16 provincesCount = randomPixels.size();
        
        // Given our semi-random points on the map, assign each pixel in the state to it's nearest point
        Vector<UnsignedInteger16> pixelProvinceIds = AssignProvinces(state, randomPixels);

        // Balance province sizes and shapes by flipping border pixels between neighbouring provinces
        CleanProvinces(state, pixelProvinceIds, provincesCount, rng);

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
