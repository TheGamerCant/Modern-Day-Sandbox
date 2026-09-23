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
#include <thread>


#include "functions.hpp"
#include "data_types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

constexpr Float64 PI = 3.14159265358979323846;




// Thread count - set to 0 for maximum thread usage
constexpr SizeT THREAD_COUNT = 8;

// Province density is calculated by summing it's rgb values (0-765) and normalising them
// between MIN_DENSITY (all black) and MAX_DENSITY (all white)
// DENSITY_PER_PROVINCE is how much density a single province should aim for
constexpr Float64 DENSITY_PER_PROVINCE = 160.0;
constexpr Float64 MIN_DENSITY = 0.1;
constexpr Float64 MAX_DENSITY = 1.0;

// Minium province size
constexpr SizeT MIN_REGION_SIZE = 100;

// Total flip attempts per state = ITERATIONS_PER_PIXEL * state pixel count
// Higher = more normal shapes but slower
constexpr SizeT ITERATIONS_PER_PIXEL = 10;

// A flip that makes the map worse is sometimes kept, but the likelihood of being kept goes down
// after each iteration, otherwise the map can get very fuzzy
constexpr Float64 TEMPERATURE_START = 3.0;
constexpr Float64 TEMPERATURE_END = 0.12;

// How strongly pixels far from their province centre are favoured for flipping
constexpr Float64 DISTANCE_WEIGHT = 1.0;
constexpr Float64 DISTANCE_WEIGHT_RATIO_CAP = 3.0;

// Each province is given a random size to aim for (based on normal distribution)
// 0.3 = every province will aim to be between 70% and 130% the size of the average
// province in a state
constexpr Float64 TARGET_SIZE_SPREAD = 0.3;

// How hard provinces get pushed towards their target density
constexpr Float64 SIZE_WEIGHT = 100.0;

// Check every pixel within NEIGHBOURHOOD_RADIUS and see if they are part of other provinces, making pixels
// surrounded by other provinces more expensive to maintain. A higher NEIGHBOURHOOD_WEIGHT means more
// rounded and compact provinces, but too high becomes repetitive
constexpr Float64 NEIGHBOURHOOD_WEIGHT = 3.0;
constexpr SignedInteger32 NEIGHBOURHOOD_RADIUS = 3;

// How hard the province will try to only be of one province type. 10.0 - 25.0 will mean most provinces will be
// 90% < one terrain type (terrain map permitting), so it's kept low to ensure good province shapes while taking
// terrain somewhat into account
constexpr Float64 TERRAIN_WEIGHT = 1.0;

// Random noise pattern that provinces try to follow. Almost like rivers where crossing a threshold
// on the noise map becomes expensive

// Higher NOISE_STRENGTH means that province borders follow the ridges more closely, lower means they
// are straighter
constexpr Float64 NOISE_STRENGTH = 3.0;
// In pixels, measures the largest octave. Lower means smaller, more frequent wriggles along province borders
constexpr Float64 NOISE_WAVELENGTH = 48.0;
constexpr SignedInteger32 NOISE_OCTAVE_LAYERS = 3;
// Cheap routes are the thin winding lines where the noise crosses its midpoint (like rivers),
// rather than broad blobby valleys
constexpr Boolean NOISE_RIDGED = true;

// No. of clean-up passes to make (duh). Expensive with diminishing returns the more you do it
constexpr SizeT CLEANUP_PASSES = 3;
// If a circle with THIN_RADIUS radius doesn't fit in a protrusion, it attempts to be cleaned up.
// lower means only smaller protrusions will be noticed and cut
constexpr SignedInteger32 THIN_RADIUS = 3;
// Flips per pixel after each clean-up round
constexpr SizeT CLEANUP_ITERATIONS_PER_PIXEL = 4;

// Simple smoothness weight. 0.0 = off, 2.0-3.0 is a fairly smooth province, anything above 4.0 becomes very smooth
constexpr Float64 SMOOTHNESS = 2.0;


// Terrain type enum and return terrain type from colour
enum TerrainType : UnsignedInteger8 {
    PLAINS, FOREST, HILLS, DESERT, MOUNTAIN, MARSH, URBAN, OCEAN, JUNGLE,
    // Use the enum itself to count the no. of terrains
    TERRAIN_TYPE_COUNT,
    NO_TERRAIN = 255,
};
TerrainType TerrainFromColour(const UnsignedInteger32 colour) {
    switch (colour) {
        case 0x567C1B: //  86, 124,  27
        case 0xFF0018: // 255,   0,  24
        case 0x728969: // 114, 137, 105
            return PLAINS;

        case 0x005606: //   0,  86,   6
        case 0x06C80B: //   6, 200,  11
            return FOREST;

        case 0x704A1F: // 112,  74,  31
        case 0x84FF00: // 132, 255,   0
            return HILLS;

        case 0xCEA963: // 206, 169,  99
        case 0xFCFF00: // 252, 255,   0
        case 0x493B0F: //  73,  59,  15
        case 0xFF00F0: // 255,   0, 240
            return DESERT;

        case 0x86541E: // 134,  84,  30
        case 0xAE00FF: // 174,   0, 255
        case 0x5C534C: //  92,  83,  76
        case 0xFFFFFF: // 255, 255, 255
        case 0x3A8352: //  58, 131,  82
        case 0x1B1B1B: //  27,  27,  27
        case 0xFF7E00: // 255, 126,   0
        case 0xF3C793: // 243, 199, 147
            return MOUNTAIN;

        case 0x4B93AE: //  75, 147, 174
            return MARSH;

        case 0xF0FF00: // 240, 255,   0
            return URBAN;

        case 0x081F82: //   8,  31, 130
            return OCEAN;

        case 0xFF007F: // 255,   0, 127
        case 0x005252: //   0,  82,  82
            return JUNGLE;

        default:
            return NO_TERRAIN;
    }
}

struct Pixel {
    UnsignedInteger16 x, y;
};

struct State {
    ColourRGB colour;
    UnsignedInteger16 x0 = UINT16_MAX, x1 = 0, y0 = UINT16_MAX, y1 = 0;
    UnsignedInteger16 width = 0, height = 0;

    // Each vector entry indicates one pixel. Vectors are kept separate to optimise RAM usage.
    // Merging them into one vector offers no actual speed improvements
    Vector<Pixel> pixels;
    Vector<Boolean> barrierPixels;
    Vector<Float32> densityWeights; // MIN_DENSITY (black) to MAX_DENSITY (white), from densitymap.png
    Vector<TerrainType> terrains;

    State(): colour(), pixels(), barrierPixels(), densityWeights(), terrains() {
        pixels.reserve(6000);
        barrierPixels.reserve(6000);
        densityWeights.reserve(6000);
        terrains.reserve(6000);
    }
    State(const ColourRGB colour): colour(colour), pixels(), barrierPixels(), densityWeights(), terrains() {
        pixels.reserve(6000);
        barrierPixels.reserve(6000);
        densityWeights.reserve(6000);
        terrains.reserve(6000);
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
    void AddTerrain(const ColourRGB colour) {
        terrains.push_back(TerrainFromColour(colour.ToInteger()));
    }
    void AddDensity(const ColourRGB colour) {
        const Float32 brightness = Float32(UnsignedInteger32(colour.r) + colour.g + colour.b) / 765.0;
        densityWeights.push_back(
            MIN_DENSITY + (MAX_DENSITY - MIN_DENSITY) * brightness
        );
    }
    void AddBarrierPixel(const UnsignedInteger32 colour) {
        // Land/water colour on rivers.png
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

    // Return a Vector<SignedInteger32> of size width * height that maps the state's provinces,
    // with -1 being "not in state"
    Vector<SignedInteger32> GetLocalPixelGrid() const {
        Vector<SignedInteger32> grid(SizeT(width) * height, -1);
        for (SizeT i = 0; i < pixels.size(); ++i)
            grid[SizeT(pixels[i].y - y0) * width + (pixels[i].x - x0)] = SignedInteger32(i);
        return grid;
    }
};

// Load our map from the in/ folder
Vector<State> LoadStates(SignedInteger32& mapWidth, SignedInteger32& mapHeight){
    Vector<State> statesVector; statesVector.reserve(1600);

    SignedInteger32 stateMapChannels{};
    UnsignedInteger8 *stateMapData = stbi_load("in/statemap.png", &mapWidth, &mapHeight, &stateMapChannels, 4);
    SignedInteger32 riverMapWidth{}, riverMapHeight{}, riverMapChannels{};
    UnsignedInteger8 *riverMapData = stbi_load("in/rivers.png", &riverMapWidth, &riverMapHeight, &riverMapChannels, 4);

    if (riverMapWidth != mapWidth || riverMapHeight != mapHeight) {
        FatalError("ERROR: statemap.png and rivers.png have different sizes.");
    }
    SignedInteger32 densityMapWidth{}, densityMapHeight{}, densityMapChannels{};
    UnsignedInteger8 *densityMapData = stbi_load("in/densitymap.png", &densityMapWidth, &densityMapHeight, &densityMapChannels, 4);

    if (densityMapWidth != mapWidth || densityMapHeight != mapHeight) {
        FatalError("ERROR: statemap.png and densitymap.png have different sizes.");
    }
    SignedInteger32 terrainMapWidth{}, terrainMapHeight{}, terrainMapChannels{};
    UnsignedInteger8 *terrainMapData = stbi_load("in/terrain.png", &terrainMapWidth, &terrainMapHeight, &terrainMapChannels, 4);

    if (terrainMapWidth != mapWidth || terrainMapHeight != mapHeight) {
        FatalError("ERROR: statemap.png and terrain.png have different sizes.");
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

            const ColourRGB densityColour(densityMapData[imgIndex + 0], densityMapData[imgIndex + 1], densityMapData[imgIndex + 2]);
            const ColourRGB terrainColour(terrainMapData[imgIndex + 0], terrainMapData[imgIndex + 1], terrainMapData[imgIndex + 2]);

            imgIndex += 4;

            // Ocean tile
            if (colourInt == 0x00141414) { continue; }

            // Current pixel is the same state as the last pixel - cache hit
            if (previousColourInt == colourInt) {
                statesVector[previousIndex].AddPixel(x, y);
                statesVector[previousIndex].AddBarrierPixel(riversColourInt);
                statesVector[previousIndex].AddDensity(densityColour);
                statesVector[previousIndex].AddTerrain(terrainColour);
            }

            else if (stateColourToIndexMap.contains(colourInt)){
                const SizeT stateIndex = stateColourToIndexMap.at(colourInt);
                statesVector[stateIndex].AddPixel(x, y);
                statesVector[stateIndex].AddBarrierPixel(riversColourInt);
                statesVector[stateIndex].AddDensity(densityColour);
                statesVector[stateIndex].AddTerrain(terrainColour);
                previousColourInt = colourInt;
                previousIndex = stateIndex;
            }
            else {
                const SizeT newIndex = statesVector.size();
                statesVector.emplace_back(pixelColour);
                statesVector[newIndex].AddPixel(x, y);
                statesVector[newIndex].AddBarrierPixel(riversColourInt);
                statesVector[newIndex].AddDensity(densityColour);
                statesVector[newIndex].AddTerrain(terrainColour);
                previousColourInt = colourInt;
                previousIndex = newIndex;
                stateColourToIndexMap[colourInt] = newIndex;
            }
        }
    }

	stbi_image_free(stateMapData);
	stbi_image_free(riverMapData);
	stbi_image_free(densityMapData);
	stbi_image_free(terrainMapData);

    // Save some RAM
	statesVector.shrink_to_fit();
	for (auto& state: statesVector) {
	    state.pixels.shrink_to_fit();
	    state.barrierPixels.shrink_to_fit();
	    state.densityWeights.shrink_to_fit();
	    state.terrains.shrink_to_fit();
	    state.UpdateBoundaries();
	}

	return statesVector;
}


// Use rivers as a barrier - INNER_BARRIER means it a river pixel surrounded on all four sides by
// other rivers and thus ignored
enum PixelType : UnsignedInteger8 {
    LAND = 0,
    BARRIER = 1,
    INNER_BARRIER = 2,
};

// Split a state into based on it's rivers, so if a state is split in half by a river, every pixel will
// be assigned to region 0 & region 1
Vector<SizeT> LabelLandRegions(
    const State& state,
    const Vector<SignedInteger32>& grid,
    const Vector<UnsignedInteger8>& kinds,
    Vector<SignedInteger32>& regionOf
) {
    static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };

    regionOf.assign(state.pixels.size(), -1);
    Vector<SizeT> sizes;
    Vector<SizeT> stack;

    // Perform flood-fill algorithm - load a pixel into the stack, then (where possible),
    // remove it from the stack and add it's neighbours into the stack, declaring all of
    // them to be of region n
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
                if (nx < 0 || ny < 0 || nx >= state.width || ny >= state.height) continue;
                const SignedInteger32 n = grid[SizeT(ny) * W + nx];
                if (n >= 0 && kinds[n] == LAND && regionOf[n] < 0) { regionOf[n] = id; stack.push_back(SizeT(n)); }
            }
        }
    }
    return sizes;
}

// Work out each pixel's PixelType
Vector<PixelType> ClassifyPixels(const State& state) {
    static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
    const Vector<SignedInteger32> grid = state.GetLocalPixelGrid();

    Vector<PixelType> pixelTypes(state.pixels.size());
    for (SizeT i = 0; i < pixelTypes.size(); ++i) pixelTypes[i] = state.barrierPixels[i] ? BARRIER : LAND;

    // Execute function fn on all of a pixel's neighbours
    auto ForEachSideNeighbour = [&](SizeT i, auto&& fn) {
        const SignedInteger32 x = state.pixels[i].x - state.x0, y = state.pixels[i].y - state.y0;
        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
            if (nx < 0 || ny < 0 || nx >= state.width || ny >= state.height) continue;
            const SignedInteger32 n = grid[SizeT(ny) * state.width + nx];
            if (n >= 0) fn(SizeT(n));
        }
    };

    // Repeat to account for river intersections
    Vector<SignedInteger32> regionOf;
    for (SizeT round = 0; round < 8; ++round) {
        const Vector<SizeT> sizes = LabelLandRegions(state, grid, pixelTypes, regionOf);
        if (sizes.size() <= 1) break;
        Vector<SizeT> toOpen;
        for (SizeT i = 0; i < pixelTypes.size(); ++i) {
            if (regionOf[i] < 0 || sizes[regionOf[i]] >= MIN_REGION_SIZE) continue;
            ForEachSideNeighbour(i, [&](SizeT n) { if (pixelTypes[n] == BARRIER) toOpen.push_back(n); });
        }
        if (toOpen.empty()) break;
        for (const SizeT n : toOpen) pixelTypes[n] = LAND;
    }

    // Assign inner barrier type to any barrier pixels bordered only by other barrier pixels
    for (SizeT i = 0; i < pixelTypes.size(); ++i) {
        if (pixelTypes[i] != BARRIER) continue;
        Boolean touchesLand = false;
        ForEachSideNeighbour(i, [&](SizeT n) { if (pixelTypes[n] == LAND) touchesLand = true; });
        if (!touchesLand) pixelTypes[i] = INNER_BARRIER;
    }
    return pixelTypes;
}

// Pick the province centres for every land area, weighted towards areas with higher density values
Vector<Pixel> SelectSeeds(const State& state, const Vector<UnsignedInteger8>& kinds, std::mt19937& rng) {
    const Vector<SignedInteger32> grid = state.GetLocalPixelGrid();
    Vector<SignedInteger32> regionOf;
    const Vector<SizeT> sizes = LabelLandRegions(state, grid, kinds, regionOf);
    if (sizes.empty()) return { state.pixels.front() };

    Vector<Vector<SizeT>> regionPixels(sizes.size());
    Vector<Float64> regionDensity(sizes.size(), 0.0);
    for (SizeT i = 0; i < state.pixels.size(); ++i) {
        if (regionOf[i] < 0) continue;
        regionPixels[regionOf[i]].push_back(i);
        regionDensity[regionOf[i]] += state.densityWeights[i];
    }

    // Weighted sampling without replacement: give each pixel the key u^(1 / weight) with u uniform in
    // (0, 1), and take the n largest keys
    std::uniform_real_distribution<Float64> unit(0.0, 1.0);
    Vector<std::pair<Float64, SizeT>> keys;
    auto sampleWeighted = [&](const Vector<SizeT>& candidates, SizeT n, Vector<Pixel>& out) {
        keys.clear();
        for (const SizeT i : candidates)
            keys.emplace_back(std::pow(std::max(unit(rng), 1e-300), 1.0 / state.densityWeights[i]), i);
        n = std::min(n, keys.size());
        std::partial_sort(keys.begin(), keys.begin() + n, keys.end(), [](const auto& a, const auto& b) { return a.first > b.first; });
        for (SizeT k = 0; k < n; ++k) out.push_back(state.pixels[keys[k].second]);
    };

    Vector<Pixel> seeds;
    for (SizeT r = 0; r < sizes.size(); ++r) {
        if (sizes[r] < MIN_REGION_SIZE) continue;
        const SizeT n = std::max<SizeT>(1, SizeT(std::llround(regionDensity[r] / DENSITY_PER_PROVINCE)));
        sampleWeighted(regionPixels[r], n, seeds);
    }
    // A state made only of small areas still gets one province, in its largest area
    if (seeds.empty()) {
        const SizeT largest = SizeT(std::max_element(sizes.begin(), sizes.end()) - sizes.begin());
        sampleWeighted(regionPixels[largest], 1, seeds);
    }
    return seeds;
}

// Starting map: grow every seed outwards over land (4-connected, never through a barrier) until the
// land is shared out, then give each barrier pixel to a province touching it from land
Vector<UnsignedInteger16> AssignProvinces(const State& state, const Vector<UnsignedInteger8>& kinds, const Vector<Pixel>& seeds) {
    static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
    const SignedInteger32 W = state.width, H = state.height;
    const Vector<SignedInteger32> grid = state.GetLocalPixelGrid();
    constexpr SignedInteger32 UNASSIGNED = -1;

    Vector<SignedInteger32> label(state.pixels.size(), UNASSIGNED);
    Queue<SizeT> frontier;
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
    // The province's density: the sum of its pixels' weights (see State::densityWeights)
    Float64 value = 0.0;
    SizeT pixelCount = 0;
    // Running coordinate sums, so the centre of gravity can be updated in O(1) per flip
    SignedInteger64 sumX = 0, sumY = 0;
    // Pixels of each terrain type, their total, and the sum of the squared counts (for TerrainMix)
    Array<UnsignedInteger32, TERRAIN_TYPE_COUNT> terrainCount{};
    UnsignedInteger32 terrainTotal = 0;
    UnsignedInteger64 terrainSquares = 0;

    Float64 CentreX() const { return Float64(sumX) / Float64(pixelCount); }
    Float64 CentreY() const { return Float64(sumY) / Float64(pixelCount); }

    // The province's most common terrain type (NO_TERRAIN if it has no terrain pixels)
    UnsignedInteger8 MainTerrain() const {
        if (terrainTotal == 0) return NO_TERRAIN;
        return UnsignedInteger8(std::max_element(terrainCount.begin(), terrainCount.end()) - terrainCount.begin());
    }

    // 1 - sum over terrain types of (share of that type)^2: 0 for a single terrain, higher when mixed
    Float64 TerrainMix() const {
        if (terrainTotal == 0) return 0.0;
        return 1.0 - Float64(terrainSquares) / (Float64(terrainTotal) * Float64(terrainTotal));
    }

    void Add(const SignedInteger64 x, const SignedInteger64 y, const Float64 weight, const UnsignedInteger8 terrain) {
        value += weight; pixelCount += 1; sumX += x; sumY += y;
        if (terrain < TERRAIN_TYPE_COUNT) {
            terrainSquares += 2 * UnsignedInteger64(terrainCount[terrain]) + 1;
            terrainCount[terrain] += 1;
            terrainTotal += 1;
        }
    }
    void Remove(const SignedInteger64 x, const SignedInteger64 y, const Float64 weight, const UnsignedInteger8 terrain) {
        value -= weight; pixelCount -= 1; sumX -= x; sumY -= y;
        if (terrain < TERRAIN_TYPE_COUNT) {
            terrainSquares -= 2 * UnsignedInteger64(terrainCount[terrain]) - 1;
            terrainCount[terrain] -= 1;
            terrainTotal -= 1;
        }
    }
};

// How far a province is from its target density, squared and relative to the target
Float64 SizeError(const Float64 density, const Float64 target) {
    const Float64 e = (density - target) / target;
    return e * e;
}

Float64 ScoreMap(const Float64 sizeErrorSum, const Float64 neighbourhoodScore, const Float64 terrainMixSum, const SizeT provincesCount) {
    return (SIZE_WEIGHT * sizeErrorSum + TERRAIN_WEIGHT * terrainMixSum) / Float64(provincesCount)
        + NEIGHBOURHOOD_WEIGHT * neighbourhoodScore;
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
    for (SignedInteger32 o = 0; o < NOISE_OCTAVE_LAYERS; ++o) {
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


// All the working data for balancing one state's provinces, and the steps that do it. CleanProvinces()
// creates one of these per state and runs it.
struct ProvinceBalancer {
    // ---- Inputs ----
    const State& state;
    const Vector<UnsignedInteger8>& kinds;
    const UnsignedInteger16 provincesCount;
    std::mt19937& rng;

    // ---- Local grids covering the state's bounding box (index = y * W + x) ----
    const SignedInteger32 W, H;
    const SizeT G;
    const SizeT totalPixels;
    // Province id per pixel, or -1 if not in this state
    Vector<SignedInteger32> label;
    // Pixel kinds (see PixelType). Inner barrier pixels sit out the optimisation as if they weren't in
    // the state, and are handed to a neighbouring province at the very end.
    Vector<UnsignedInteger8> kindGrid;
    // Each pixel's weight towards its province's density (see State::densityWeights), and its terrain
    Vector<Float64> weightGrid;
    Vector<UnsignedInteger8> terrainGrid;
    // Terrain cost per pixel (see NOISE_STRENGTH); a pixel pair costs the average of its two pixels
    Vector<Float64> terrainCost;
    Float64 totalDensity = 0.0;

    // 4-connected: provinces only count as touching (and as connected) through full sides
    static constexpr SignedInteger32 dx4[4] = { 1, 0, -1, 0 };
    static constexpr SignedInteger32 dy4[4] = { 0, 1, 0, -1 };
    // The 8 surrounding pixels in clockwise order. Consecutive entries always share a side,
    // and even entries are the 4 side-neighbours.
    static constexpr SignedInteger32 ringX[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
    static constexpr SignedInteger32 ringY[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };

    // ---- Running totals the score is built from ----
    Vector<Province> provinces;
    // Random target densities, rescaled so they add up to the state's total density
    Vector<Float64> targetSize;
    Float64 sizeErrorSum = 0.0;
    Float64 terrainMixSum = 0.0;
    // Sum over ordered (pixel, pixel-in-disc) pairs in different provinces of the pair's terrain cost
    Float64 neighbourhoodEnergy = 0.0;

    // ---- Neighbourhood border score constants ----
    const Boolean useNeighbourhood = NEIGHBOURHOOD_WEIGHT > 0.0;
    Vector<std::pair<SignedInteger32, SignedInteger32>> neighbourhoodOffsets;
    SignedInteger64 crossingsPerUnitBorder = 0; // what a straight border of length 1 adds to the energy
    Float64 neighbourhoodScale = 0.0;
    // Score change of adding one pixel of straight border, the unit temperatures are measured in
    Float64 borderPixelScore = 0.0;
    Vector<std::pair<SignedInteger32, SignedInteger32>> thinOffsets;

    // ---- Boundary pixel set (pixels with at least one side touching another province) ----
    // Kept as a flat list plus a position lookup, so add/remove are both O(1)
    Vector<UnsignedInteger32> boundary;
    Vector<SignedInteger32> boundaryPos;

    // Largest possible pixel weight when picking pixels to flip (see FlipWeight)
    Float64 maxWeight = 4.0 * (1.0 + DISTANCE_WEIGHT * DISTANCE_WEIGHT_RATIO_CAP);
    std::uniform_real_distribution<Float64> unit{ 0.0, 1.0 };

    ProvinceBalancer(
        const State& state,
        const Vector<UnsignedInteger8>& kinds,
        const Vector<UnsignedInteger16>& pixelProvinceIds,
        const UnsignedInteger16 provincesCount,
        std::mt19937& rng
    ) : state(state), kinds(kinds), provincesCount(provincesCount), rng(rng),
        W(state.width), H(state.height), G(SizeT(state.width) * state.height), totalPixels(state.pixels.size()),
        label(G, -1), kindGrid(G, INNER_BARRIER), weightGrid(G, 0.0), terrainGrid(G, NO_TERRAIN), terrainCost(G, 1.0),
        provinces(provincesCount), targetSize(provincesCount), boundaryPos(G, -1)
    {
        for (SizeT i = 0; i < totalPixels; ++i) {
            const SizeT li = LocalIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0);
            kindGrid[li] = kinds[i];
            weightGrid[li] = state.densityWeights[i];
            terrainGrid[li] = state.terrains[i];
            if (kinds[i] != INNER_BARRIER) totalDensity += weightGrid[li];
            if (kinds[i] != INNER_BARRIER) label[li] = pixelProvinceIds[i];
        }

        // Random target densities
        {
            std::normal_distribution<Float64> normal(0.0, TARGET_SIZE_SPREAD);
            Float64 total = 0.0;
            for (auto& t : targetSize) { t = std::exp(normal(rng)); total += t; }
            for (auto& t : targetSize) t *= totalDensity / total;
        }

        // Terrain cost from the noise field
        {
            const UnsignedInteger32 noiseSeed = UnsignedInteger32(rng());
            Float64 costSum = 0.0;
            for (SignedInteger32 y = 0; y < H; ++y)
                for (SignedInteger32 x = 0; x < W; ++x) {
                    Float64 n = FractalNoise(Float64(x + state.x0), Float64(y + state.y0), noiseSeed);
                    if (NOISE_RIDGED) n = std::min(1.0, 2.0 * std::abs(2.0 * n - 1.0));
                    terrainCost[LocalIndex(x, y)] = 1.0 + NOISE_STRENGTH * n;
                    if (label[LocalIndex(x, y)] >= 0) costSum += terrainCost[LocalIndex(x, y)];
                }
            // Rescale so the average in-state cost is 1, keeping the score's overall scale unchanged
            const Float64 meanCost = costSum / Float64(totalPixels);
            for (auto& c : terrainCost) c /= meanCost;
        }

        // Neighbourhood score constants, normalised so a map of circles scores about 1
        neighbourhoodOffsets = DiscOffsets(NEIGHBOURHOOD_RADIUS);
        for (const auto& [ox, oy] : neighbourhoodOffsets) crossingsPerUnitBorder += std::abs(oy);
        const Float64 meanSize = Float64(totalPixels) / Float64(provincesCount);
        neighbourhoodScale = 1.0 / (Float64(std::max<SignedInteger64>(1, crossingsPerUnitBorder))
            * Float64(provincesCount) * 2.0 * std::sqrt(PI * meanSize));
        borderPixelScore = NEIGHBOURHOOD_WEIGHT * neighbourhoodScale * 2.0 * Float64(crossingsPerUnitBorder);

        thinOffsets = DiscOffsets(THIN_RADIUS);
        boundary.reserve(totalPixels / 4);
    }

    // ---- The whole process ----
    void Run() {
        ReattachBarrierPixels(nullptr);
        Rebuild();
        Anneal(ITERATIONS_PER_PIXEL * totalPixels, TEMPERATURE_START, TEMPERATURE_END);

        for (SizeT pass = 0; pass < CLEANUP_PASSES; ++pass) {
            const SizeT reseeded = ReseedCorelessProvinces();
            if (reseeded + RemoveThinParts() + RemoveDisconnectedPieces() == 0) break;
            Rebuild();
            Anneal(CLEANUP_ITERATIONS_PER_PIXEL * totalPixels, TEMPERATURE_END, TEMPERATURE_END);
        }

        SmoothBorders();
        SplitRiverStretches();
        FillInnerBarrierPixels();
    }

    // Write the result back (anything still unlabelled keeps its starting province)
    void WriteBack(Vector<UnsignedInteger16>& pixelProvinceIds) const {
        for (SizeT i = 0; i < totalPixels; ++i) {
            const SignedInteger32 l = label[LocalIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0)];
            if (l >= 0) pixelProvinceIds[i] = UnsignedInteger16(l);
        }
    }

    // ---- Grid helpers ----

    SizeT LocalIndex(SignedInteger32 x, SignedInteger32 y) const {
        return SizeT(y) * W + x;
    }

    Boolean InGrid(SignedInteger32 x, SignedInteger32 y) const {
        return x >= 0 && y >= 0 && x < W && y < H;
    }

    SignedInteger32 LabelAt(SignedInteger32 x, SignedInteger32 y) const {
        return InGrid(x, y) ? label[LocalIndex(x, y)] : -1;
    }

    Boolean IsLand(SignedInteger32 x, SignedInteger32 y) const {
        return InGrid(x, y) && kindGrid[LocalIndex(x, y)] == LAND;
    }

    Boolean IsBarrier(SignedInteger32 x, SignedInteger32 y) const {
        return InGrid(x, y) && kindGrid[LocalIndex(x, y)] == BARRIER;
    }

    // ---- Scoring helpers ----

    // Total pair cost between (x, y) and the pixels in its disc that belong to province `id`
    Float64 CostInDisc(SignedInteger32 x, SignedInteger32 y, SignedInteger32 id) const {
        const Float64 own = terrainCost[LocalIndex(x, y)];
        Float64 c = 0.0;
        for (const auto& [ox, oy] : neighbourhoodOffsets)
            if (LabelAt(x + ox, y + oy) == id) c += 0.5 * (own + terrainCost[LocalIndex(x + ox, y + oy)]);
        return c;
    }

    SignedInteger32 CountForeignSides(SignedInteger32 x, SignedInteger32 y) const {
        const SignedInteger32 own = label[LocalIndex(x, y)];
        SignedInteger32 count = 0;
        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
            if (n >= 0 && n != own) ++count;
        }
        return count;
    }

    // ---- Keeping the running totals up to date ----

    void RefreshBoundary(SignedInteger32 x, SignedInteger32 y) {
        if (!InGrid(x, y)) return;
        const SizeT li = LocalIndex(x, y);
        if (label[li] < 0) return;

        const Boolean isBoundary = CountForeignSides(x, y) > 0;
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
    }

    // Recompute every running total from the label grid
    void Rebuild() {
        provinces.assign(provincesCount, Province{});
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x) {
                const SignedInteger32 own = label[LocalIndex(x, y)];
                if (own >= 0) provinces[own].Add(x, y, weightGrid[LocalIndex(x, y)], terrainGrid[LocalIndex(x, y)]);
            }

        sizeErrorSum = 0.0;
        terrainMixSum = 0.0;
        for (SizeT i = 0; i < provincesCount; ++i) {
            sizeErrorSum += SizeError(provinces[i].value, targetSize[i]);
            terrainMixSum += provinces[i].TerrainMix();
        }

        neighbourhoodEnergy = 0.0;
        if (useNeighbourhood) {
            for (SignedInteger32 y = 0; y < H; ++y)
                for (SignedInteger32 x = 0; x < W; ++x) {
                    const SignedInteger32 own = label[LocalIndex(x, y)];
                    if (own < 0) continue;
                    for (const auto& [ox, oy] : neighbourhoodOffsets) {
                        const SignedInteger32 n = LabelAt(x + ox, y + oy);
                        if (n >= 0 && n != own)
                            neighbourhoodEnergy += 0.5 * (terrainCost[LocalIndex(x, y)] + terrainCost[LocalIndex(x + ox, y + oy)]);
                    }
                }
        }

        boundary.clear();
        std::fill(boundaryPos.begin(), boundaryPos.end(), -1);
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x)
                RefreshBoundary(x, y);
    }

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
    Boolean CanRemove(SignedInteger32 x, SignedInteger32 y, SignedInteger32 own) const {
        if (provinces[own].pixelCount <= 1) return false;
        if (!IsLand(x, y)) return true;

        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 bx = x + dx4[d], by = y + dy4[d];
            if (!IsBarrier(bx, by) || LabelAt(bx, by) != own) continue;
            Boolean stillAttached = false;
            for (SignedInteger32 e = 0; e < 4; ++e) {
                const SignedInteger32 nx = bx + dx4[e], ny = by + dy4[e];
                if ((nx != x || ny != y) && IsLand(nx, ny) && LabelAt(nx, ny) == own) stillAttached = true;
            }
            if (!stillAttached) return false;
        }

        Boolean inProvince[8];
        SignedInteger32 start = -1;
        for (SignedInteger32 k = 0; k < 8; ++k) {
            inProvince[k] = LabelAt(x + ringX[k], y + ringY[k]) == own && IsLand(x + ringX[k], y + ringY[k]);
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
    }

    // Can (x, y) join province `target`? Only if one of target's land pixels touches it by a side -
    // never across a barrier pixel, which is what stops provinces crossing rivers
    Boolean CanJoin(SignedInteger32 x, SignedInteger32 y, SignedInteger32 target) const {
        for (SignedInteger32 d = 0; d < 4; ++d)
            if (IsLand(x + dx4[d], y + dy4[d]) && LabelAt(x + dx4[d], y + dy4[d]) == target) return true;
        return false;
    }

    // ---- The flip loop ----

    Float64 FlipWeight(SignedInteger32 x, SignedInteger32 y, SignedInteger32 own, SignedInteger32 foreignSides) const {
        const Province& p = provinces[own];
        const Float64 ddx = Float64(x) - p.CentreX(), ddy = Float64(y) - p.CentreY();
        const Float64 radius = std::max(1.0, std::sqrt(Float64(p.pixelCount) / PI));
        const Float64 ratio = std::min(DISTANCE_WEIGHT_RATIO_CAP, std::sqrt(ddx * ddx + ddy * ddy) / radius);
        return Float64(foreignSides) * (1.0 + DISTANCE_WEIGHT * ratio);
    }

    void Anneal(const SizeT iterations, const Float64 temperatureStart, const Float64 temperatureEnd) {
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
                foreignSides = CountForeignSides(x, y);
                if (unit(rng) * maxWeight < FlipWeight(x, y, own, foreignSides)) break;
            }

            // Pick which neighbouring province it flips to - a random foreign side, so a province
            // touching two sides of the pixel is twice as likely as one touching one side. Only sides
            // touching another province's land count: a province can't reach across a barrier pixel.
            SignedInteger32 candidates[4], candidateCount = 0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
                if (n >= 0 && n != own && IsLand(x + dx4[d], y + dy4[d])) candidates[candidateCount++] = n;
            }
            if (candidateCount == 0) continue;
            const SignedInteger32 target = candidates[std::uniform_int_distribution<SignedInteger32>(0, candidateCount - 1)(rng)];

            // Provinces may not vanish, split or cross a barrier
            if (!CanRemove(x, y, own)) continue;

            // ---- Build the two changed provinces as they'd be after the flip ----
            Province newOwn = provinces[own], newTarget = provinces[target];
            newOwn.Remove(x, y, weightGrid[LocalIndex(x, y)], terrainGrid[LocalIndex(x, y)]);
            newTarget.Add(x, y, weightGrid[LocalIndex(x, y)], terrainGrid[LocalIndex(x, y)]);

            // ---- Score old vs new map ----
            const Float64 newSizeErrorSum = sizeErrorSum
                + SizeError(newOwn.value, targetSize[own]) + SizeError(newTarget.value, targetSize[target])
                - SizeError(provinces[own].value, targetSize[own]) - SizeError(provinces[target].value, targetSize[target]);
            const Float64 newTerrainMixSum = terrainMixSum
                + newOwn.TerrainMix() + newTarget.TerrainMix() - provinces[own].TerrainMix() - provinces[target].TerrainMix();
            // Flipping p from own to target: every pair (p, q) and (q, p) with q in own becomes a
            // border pair, and every pair with q in target stops being one
            Float64 newNeighbourhoodEnergy = neighbourhoodEnergy;
            if (useNeighbourhood)
                newNeighbourhoodEnergy += 2.0 * (CostInDisc(x, y, own) - CostInDisc(x, y, target));

            const Float64 oldScore = ScoreMap(sizeErrorSum, neighbourhoodEnergy * neighbourhoodScale, terrainMixSum, provincesCount);
            const Float64 newScore = ScoreMap(newSizeErrorSum, newNeighbourhoodEnergy * neighbourhoodScale, newTerrainMixSum, provincesCount);

            if (newScore > oldScore) {
                const Float64 progress = Float64(it) / Float64(iterations);
                const Float64 temperature = temperatureStart * std::pow(temperatureEnd / temperatureStart, progress);
                if (temperature <= 0.0 || unit(rng) >= std::exp(-(newScore - oldScore) / (temperature * borderPixelScore))) continue;
            }

            // ---- Apply the flip ----
            label[LocalIndex(x, y)] = target;
            provinces[own] = newOwn;
            provinces[target] = newTarget;
            sizeErrorSum = newSizeErrorSum;
            terrainMixSum = newTerrainMixSum;
            neighbourhoodEnergy = newNeighbourhoodEnergy;

            RefreshBoundary(x, y);
            for (SignedInteger32 d = 0; d < 4; ++d) RefreshBoundary(x + dx4[d], y + dy4[d]);
        }
    }

    // ---- Cleanup ----

    // Mark (in `remove`) every pixel that isn't part of its province's main piece: the largest
    // 4-connected piece of the province's `include`d land pixels (never linked through a barrier), plus
    // the `include`d barrier pixels touching that piece from land
    void MarkAllButLargestPiece(const Vector<UnsignedInteger8>& include, Vector<UnsignedInteger8>& remove) {
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
                    if (!IsLand(nx, ny)) continue;
                    const SizeT ni = LocalIndex(nx, ny);
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
                if (IsLand(nx, ny) && piece[LocalIndex(nx, ny)] >= 0 && piece[LocalIndex(nx, ny)] == largest[own]) attached = true;
            }
            if (!include[li] || !attached) remove[li] = 1;
        }
    }

    // Give every barrier pixel that no longer touches its own province's land to the neighbouring
    // province with the most land sides against it (never back to `avoid`, if given)
    SizeT ReattachBarrierPixels(const Vector<UnsignedInteger8>* avoid) {
        SizeT moved = 0;
        for (SizeT li = 0; li < G; ++li) {
            if (label[li] < 0 || kindGrid[li] != BARRIER) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            const Boolean mustLeave = avoid && (*avoid)[li];
            if (!mustLeave && CanJoin(x, y, label[li])) continue;
            SignedInteger32 best = -1, bestCount = 0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                if (!IsLand(x + dx4[d], y + dy4[d])) continue;
                const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
                if (mustLeave && n == label[li]) continue;
                SignedInteger32 count = 0;
                for (SignedInteger32 e = 0; e < 4; ++e)
                    if (IsLand(x + dx4[e], y + dy4[e]) && LabelAt(x + dx4[e], y + dy4[e]) == n) ++count;
                if (count > bestCount) { best = n; bestCount = count; }
            }
            if (best >= 0 && best != label[li]) { label[li] = best; ++moved; }
        }
        return moved;
    }

    // Hand every marked pixel to a neighbouring province (never back to its own), spreading out over
    // land from the unmarked pixels with a flood fill so every province gaining pixels stays in one piece
    // and nothing crosses a barrier. Marked barrier pixels, and any barrier pixels whose land neighbours
    // changed hands, then go to a province touching them from land. Pixels the fill can't reach (e.g. an
    // island with no other province on it) are left alone.
    SizeT ReassignPixels(const Vector<UnsignedInteger8>& remove) {
        Vector<SignedInteger32> newLabel(G, -1);
        Queue<UnsignedInteger32> frontier;
        for (SizeT li = 0; li < G; ++li) {
            if (!remove[li] || kindGrid[li] != LAND) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            // Prefer the neighbouring province touching the most sides
            SignedInteger32 best = -1, bestCount = 0;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
                if (!IsLand(nx, ny)) continue;
                const SizeT ni = LocalIndex(nx, ny);
                const SignedInteger32 n = label[ni];
                if (n < 0 || remove[ni] || n == label[li]) continue;
                SignedInteger32 count = 0;
                for (SignedInteger32 e = 0; e < 4; ++e) {
                    const SignedInteger32 mx = x + dx4[e], my = y + dy4[e];
                    if (IsLand(mx, my) && !remove[LocalIndex(mx, my)] && label[LocalIndex(mx, my)] == n) ++count;
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
                if (!IsLand(nx, ny)) continue;
                const SizeT ni = LocalIndex(nx, ny);
                if (remove[ni] && newLabel[ni] < 0 && label[ni] != newLabel[c]) {
                    newLabel[ni] = newLabel[c];
                    frontier.push(UnsignedInteger32(ni));
                }
            }
        }
        SizeT moved = 0;
        for (SizeT li = 0; li < G; ++li)
            if (remove[li] && newLabel[li] >= 0) { label[li] = newLabel[li]; ++moved; }
        return moved + ReattachBarrierPixels(&remove);
    }

    // Cut off every part of a province that a disc of radius THIN_RADIUS can't fit inside, plus anything
    // only connected to the rest of the province through such a part. Other states, sea and barrier
    // pixels count as "inside" here, so a province isn't punished for a coastline, state border or river
    // it can't change. (Barrier pixels are kept or cut along with the land they touch.)
    // Core pixels: land pixels with no other province's land within THIN_RADIUS - the centres of the
    // discs that fit inside their province. hasCore says which provinces have at least one.
    void FindCores(Vector<UnsignedInteger8>& hasCore, Vector<UnsignedInteger32>& corePixels) {
        hasCore.assign(provincesCount, 0);
        corePixels.clear();
        for (SignedInteger32 y = 0; y < H; ++y)
            for (SignedInteger32 x = 0; x < W; ++x) {
                const SignedInteger32 own = label[LocalIndex(x, y)];
                if (own < 0 || !IsLand(x, y)) continue;
                Boolean isCore = true;
                for (const auto& [ox, oy] : thinOffsets) {
                    const SignedInteger32 n = LabelAt(x + ox, y + oy);
                    if (n >= 0 && n != own && IsLand(x + ox, y + oy)) { isCore = false; break; }
                }
                if (isCore) { corePixels.push_back(UnsignedInteger32(LocalIndex(x, y))); hasCore[own] = 1; }
            }
    }

    // Split province `donor` in two, giving one half to the (empty) province `freed`: grow both halves at
    // once over the donor's land from two far-apart pixels, so each half is one compact piece
    void SplitProvince(SignedInteger32 donor, SignedInteger32 freed) {
        Vector<UnsignedInteger32> land;
        Float64 cx = 0.0, cy = 0.0;
        for (SizeT li = 0; li < G; ++li)
            if (label[li] == donor && kindGrid[li] == LAND) { land.push_back(UnsignedInteger32(li)); cx += Float64(li % W); cy += Float64(li / W); }
        if (land.size() < 2) return;
        cx /= Float64(land.size()); cy /= Float64(land.size());

        // First source: the donor's pixel furthest from its centre. Second: the pixel furthest from the
        // first, walking over the donor's land
        UnsignedInteger32 first = land.front();
        Float64 bestDist = -1.0;
        for (const UnsignedInteger32 li : land) {
            const Float64 dx = Float64(li % W) - cx, dy = Float64(li / W) - cy;
            if (dx * dx + dy * dy > bestDist) { bestDist = dx * dx + dy * dy; first = li; }
        }
        Vector<SignedInteger32> owner(G, -1);
        Queue<UnsignedInteger32> frontier;
        auto grow = [&]() {
            UnsignedInteger32 last = 0;
            while (!frontier.empty()) {
                const UnsignedInteger32 c = frontier.front(); frontier.pop();
                last = c;
                const SignedInteger32 x = SignedInteger32(c % W), y = SignedInteger32(c / W);
                for (SignedInteger32 d = 0; d < 4; ++d) {
                    const SignedInteger32 nx = x + dx4[d], ny = y + dy4[d];
                    if (!IsLand(nx, ny) || label[LocalIndex(nx, ny)] != donor || owner[LocalIndex(nx, ny)] >= 0) continue;
                    owner[LocalIndex(nx, ny)] = owner[c];
                    frontier.push(UnsignedInteger32(LocalIndex(nx, ny)));
                }
            }
            return last;
        };
        owner[first] = donor;
        frontier.push(first);
        const UnsignedInteger32 second = grow();
        if (second == first) return;

        std::fill(owner.begin(), owner.end(), -1);
        owner[first] = donor;
        owner[second] = freed;
        frontier.push(first);
        frontier.push(second);
        grow();
        for (const UnsignedInteger32 li : land) if (owner[li] == freed) label[li] = freed;
        ReattachBarrierPixels(nullptr);
    }

    // A province with no core anywhere (e.g. squeezed into a thin ring around its neighbours) can't be
    // fixed by trimming, and single-pixel flips won't reshape it. Dissolve it into its neighbours, then
    // bring it back by splitting the province furthest over its target density in two, so the province
    // count stays the same. Provinces that can't be absorbed (e.g. alone on their side of a river) stay.
    SizeT ReseedCorelessProvinces() {
        Vector<UnsignedInteger8> hasCore;
        Vector<UnsignedInteger32> corePixels;
        FindCores(hasCore, corePixels);
        Boolean anyWithCore = false;
        for (const auto h : hasCore) anyWithCore = anyWithCore || h;
        if (!anyWithCore) return 0;

        Vector<UnsignedInteger8> remove(G, 0);
        for (SizeT li = 0; li < G; ++li)
            if (label[li] >= 0 && !hasCore[label[li]]) remove[li] = 1;
        SizeT moved = ReassignPixels(remove);
        if (moved == 0) return 0;
        Rebuild();

        for (SignedInteger32 freed = 0; freed < SignedInteger32(provincesCount); ++freed) {
            if (hasCore[freed] || provinces[freed].pixelCount > 0) continue; // untouched, or couldn't be absorbed
            SignedInteger32 donor = -1;
            Float64 worst = 0.0;
            for (SignedInteger32 i = 0; i < SignedInteger32(provincesCount); ++i) {
                if (!hasCore[i] || provinces[i].pixelCount < 2) continue;
                const Float64 ratio = provinces[i].value / targetSize[i];
                if (donor < 0 || ratio > worst) { donor = i; worst = ratio; }
            }
            if (donor < 0) break;
            SplitProvince(donor, freed);
            Rebuild();
        }
        return moved;
    }

    SizeT RemoveThinParts() {
        Vector<UnsignedInteger8> kept(G, 0), remove(G, 0);
        Vector<UnsignedInteger8> hasCore;
        Vector<UnsignedInteger32> corePixels;
        FindCores(hasCore, corePixels);
        // Everything a core disc covers is part of the province's thick body
        for (const UnsignedInteger32 li : corePixels) {
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W), own = label[li];
            kept[li] = 1;
            for (const auto& [ox, oy] : thinOffsets)
                if (LabelAt(x + ox, y + oy) == own) kept[LocalIndex(x + ox, y + oy)] = 1;
        }
        // Barrier pixels are judged by the land they touch (see markAllButLargestPiece)
        for (SizeT li = 0; li < G; ++li)
            if (label[li] >= 0 && kindGrid[li] == BARRIER) kept[li] = 1;
        // Provinces too small or thin to have any core are left as they are
        for (SizeT li = 0; li < G; ++li)
            if (label[li] >= 0 && !hasCore[label[li]]) kept[li] = 1;

        MarkAllButLargestPiece(kept, remove);
        return ReassignPixels(remove);
    }

    // Give away every piece of a province that isn't its largest 4-connected piece
    SizeT RemoveDisconnectedPieces() {
        Vector<UnsignedInteger8> include(G, 0), remove(G, 0);
        for (SizeT li = 0; li < G; ++li) include[li] = label[li] >= 0;
        MarkAllButLargestPiece(include, remove);
        return ReassignPixels(remove);
    }

    // ---- Finishing touches ----

    // Majority-filter smoothing (see SMOOTHNESS). Pixels are visited in random order and updated one at a
    // time, so every switch is checked against the current map and can't split a province.
    void SmoothBorders() {
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
                if (CountForeignSides(x, y) == 0) continue; // not on a border

                // Count the disc's pixels per province (the pixel itself included)
                for (const auto& [ox, oy] : disc) {
                    const SignedInteger32 n = LabelAt(x + ox, y + oy);
                    if (n < 0) continue;
                    if (votes[n] == 0) touched.push_back(n);
                    ++votes[n];
                }
                // The winner must touch the pixel by a side from land, so it stays in one piece after
                // gaining it and never reaches across a barrier
                SignedInteger32 best = own;
                for (SignedInteger32 d = 0; d < 4; ++d) {
                    const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
                    if (n >= 0 && IsLand(x + dx4[d], y + dy4[d]) && votes[n] > votes[best]) best = n;
                }
                for (const SignedInteger32 n : touched) votes[n] = 0;
                touched.clear();

                if (best == own || !CanRemove(x, y, own)) continue;
                // Terrain veto: don't move a pixel out of a province whose main terrain it matches into
                // one whose main terrain it doesn't, so smoothing doesn't undo the terrain alignment
                const UnsignedInteger8 terrain = terrainGrid[li];
                if (TERRAIN_WEIGHT > 0.0 && terrain != NO_TERRAIN &&
                    provinces[own].MainTerrain() == terrain && provinces[best].MainTerrain() != terrain) continue;
                label[li] = best;
                provinces[own].Remove(x, y, weightGrid[li], terrainGrid[li]);
                provinces[best].Add(x, y, weightGrid[li], terrainGrid[li]);
                ++changed;
            }
            if (changed == 0) break;
        }
    }

    // Share river pixels between the provinces on either side. A "stretch" is a run of barrier pixels
    // whose land neighbours are exactly two provinces, A and B. Its pixels are put in order along the
    // river and split in half - one half to each province - so the border crosses the river only once
    // per stretch. Every such pixel touches both provinces' land, so either can own it.
    void SplitRiverStretches() {
        std::mt19937 splitRng{ UnsignedInteger32(rng()) };
        Vector<SignedInteger32> pairA(G, -1), pairB(G, -1);
        for (SizeT li = 0; li < G; ++li) {
            if (label[li] < 0 || kindGrid[li] != BARRIER) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            SignedInteger32 a = -1, b = -1;
            Boolean tooMany = false;
            for (SignedInteger32 d = 0; d < 4; ++d) {
                if (!IsLand(x + dx4[d], y + dy4[d])) continue;
                const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
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
                return InGrid(x, y) && pairA[LocalIndex(x, y)] == a && pairB[LocalIndex(x, y)] == b;
            };

            // Collect the stretch (8-connected, since rivers can step diagonally)
            stretch.clear();
            stretch.push_back(UnsignedInteger32(start));
            distance[start] = 0;
            for (SizeT k = 0; k < stretch.size(); ++k) {
                const SignedInteger32 x = SignedInteger32(stretch[k] % W), y = SignedInteger32(stretch[k] / W);
                for (SignedInteger32 r = 0; r < 8; ++r)
                    if (sameStretch(x + ringX[r], y + ringY[r]) && distance[LocalIndex(x + ringX[r], y + ringY[r])] < 0) {
                        distance[LocalIndex(x + ringX[r], y + ringY[r])] = 0;
                        stretch.push_back(UnsignedInteger32(LocalIndex(x + ringX[r], y + ringY[r])));
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
                        if (sameStretch(nx, ny) && distance[LocalIndex(nx, ny)] < 0) {
                            distance[LocalIndex(nx, ny)] = distance[queue[k]] + 1;
                            queue.push_back(UnsignedInteger32(LocalIndex(nx, ny)));
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
                provinces[label[li]].Remove(x, y, weightGrid[li], terrainGrid[li]);
                provinces[newOwner].Add(x, y, weightGrid[li], terrainGrid[li]);
                label[li] = newOwner;
            }
        }
    }
    // Inner barrier pixels go to the most common province among their 4 side-neighbours. Only side
    // neighbours count: a province touching the pixel just at a corner would leave it detached (HOI4
    // provinces connect through sides, not corners). Pixels whose side-neighbours are all unassigned
    // wait for a later pass, once their neighbours have been filled in.
    void FillInnerBarrierPixels() {
        for (SizeT pass = 0; pass < 64; ++pass) {
            Vector<std::pair<SizeT, SignedInteger32>> fills;
            Boolean anyLeft = false;
            for (SizeT li = 0; li < G; ++li) {
                if (label[li] >= 0 || kindGrid[li] != INNER_BARRIER) continue;
                const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
                SignedInteger32 best = -1, bestCount = 0;
                for (SignedInteger32 d = 0; d < 4; ++d) {
                    const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
                    if (n < 0) continue;
                    SignedInteger32 count = 0;
                    for (SignedInteger32 e = 0; e < 4; ++e) if (LabelAt(x + dx4[e], y + dy4[e]) == n) ++count;
                    if (count > bestCount) { best = n; bestCount = count; }
                }
                if (best >= 0) fills.emplace_back(li, best); else anyLeft = true;
            }
            // Applied after the scan, so each pass only grows from pixels filled in earlier passes
            for (const auto& [li, owner] : fills) label[li] = owner;
            if (!anyLeft || fills.empty()) break;
        }
    }
};

// Balance one state's province sizes and shapes by flipping border pixels between neighbouring provinces
void CleanProvinces(
    const State& state,
    const Vector<UnsignedInteger8>& kinds,
    Vector<UnsignedInteger16>& pixelProvinceIds,
    const UnsignedInteger16 provincesCount,
    std::mt19937& rng
) {
    // Set up first even for a single-province state: it draws from rng, and keeping those draws the
    // same keeps every later state's result the same
    ProvinceBalancer balancer(state, kinds, pixelProvinceIds, provincesCount, rng);
    if (provincesCount < 2) return;
    balancer.Run();
    balancer.WriteBack(pixelProvinceIds);
}

// Everything for one state, start to finish: seeds, starting layout, balancing, then colouring its
// provinces into provincesMapData. States never share pixels, so several can run at once.
void ProcessState(const State& state, std::mt19937& rng, UnsignedInteger8* provincesMapData, const SignedInteger32 mapWidth) {
    // Work out which pixels are barriers (rivers), then pick random points in each state - per
    // separate land area, more of them where it's denser - to be our province centres
    const Vector<PixelType> pixelTypes = ClassifyPixels(state);
    Vector<Pixel> randomPixels = SelectSeeds(state, pixelTypes, rng);
    const UnsignedInteger16 provincesCount = randomPixels.size();

    // Given our semi-random points on the map, assign each pixel in the state to it's nearest point
    Vector<UnsignedInteger16> pixelProvinceIds = AssignProvinces(state, pixelTypes, randomPixels);

    // Balance province sizes and shapes by flipping border pixels between neighbouring provinces
    CleanProvinces(state, pixelTypes, pixelProvinceIds, provincesCount, rng);

    // Colour in the provinces
    /*
    Vector<ColourRGB> colours = GenerateNRandomColours(
        provincesCount,
        std::max(0, SignedInteger32(state.colour.r) - 25), std::min(255, SignedInteger32(state.colour.r) + 25),
        std::max(0, SignedInteger32(state.colour.g) - 25), std::min(255, SignedInteger32(state.colour.g) + 25),
        std::max(0, SignedInteger32(state.colour.b) - 25), std::min(255, SignedInteger32(state.colour.b) + 25),
        rng
    );
    */
    Vector<ColourRGB> colours = GenerateNRandomColours(provincesCount, 0, 255, 0, 255, 0, 255, rng);
    for (SizeT i = 0; i < state.pixels.size(); i++) {
        const UnsignedInteger16 localId = pixelProvinceIds[i];
        const SizeT index = ((SizeT(state.pixels[i].y) * mapWidth) + state.pixels[i].x) * 3;
        provincesMapData[index + 0] = colours[localId].r;
        provincesMapData[index + 1] = colours[localId].g;
        provincesMapData[index + 2] = colours[localId].b;
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

    // One seed per state, drawn up front, so the map doesn't depend on which thread runs which state
    Vector<UnsignedInteger32> stateSeeds(statesVector.size());
    for (auto& seed : stateSeeds) seed = UnsignedInteger32(rng());

    // Order the states based on size and divide them into threadCount vectors
    const SizeT threadCount = (THREAD_COUNT == 0) ? std::thread::hardware_concurrency() : std::min<SizeT>(THREAD_COUNT, std::thread::hardware_concurrency());
    Vector<Vector<SizeT>> stateGroups(threadCount);
    {
        Vector<SizeT> order(statesVector.size());
        for (SizeT s = 0; s < order.size(); ++s) order[s] = s;
        std::sort(
            order.begin(), order.end(), [&](SizeT a, SizeT b) {
                return statesVector[a].pixels.size() > statesVector[b].pixels.size();
            }
        );
        Vector<SizeT> groupPixels(threadCount, 0);
        for (const SizeT s : order) {
            const SizeT lightest = SizeT(std::min_element(groupPixels.begin(), groupPixels.end()) - groupPixels.begin());
            stateGroups[lightest].push_back(s);
            groupPixels[lightest] += statesVector[s].pixels.size();
        }
    }

    // Compute the states and write the provinces to provincesMapData concurrently, as no
    // pixel will ever be overwritten by another state
    Vector<std::thread> threads;
    for (SizeT t = 0; t < threadCount; ++t) {
        threads.emplace_back([&, t]() {
            for (const SizeT s : stateGroups[t]) {
                std::mt19937 stateRng(stateSeeds[s]);
                ProcessState(statesVector[s], stateRng, provincesMapData, mapWidth);
            }
        });
    }
    for (auto& thread : threads) thread.join();

    stbi_write_png("out/provinces.png", mapWidth, mapHeight, 3, provincesMapData, mapWidth * 3);
    delete[] provincesMapData;

    std::cout << std::format("Processed statemap in {0}\n", GetTimeElapsedFromStart(startTime));
    return 0;
}
