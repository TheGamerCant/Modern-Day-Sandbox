//g++ src/*.cpp -std=c++20 -O3 -o map_generator_mac

#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <limits>
#include <tuple>
#include <queue>


#include "functions.hpp"
#include "data_types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

Vector<Pixel> SelectRandomPixels(const State& state, SizeT n, std::mt19937& rng) {
    const auto& pixels = state.pixels;
    if (n == 0 || pixels.empty()) return {};
    n = std::min(n, pixels.size());

    Vector<Vector<UnsignedInteger32>> groups;
    groups.emplace_back(pixels.size());
    for (UnsignedInteger32 i = 0; i < pixels.size(); ++i) groups[0][i] = i;

    while (groups.size() < n) {
        SizeT biggest = 0;
        for (SizeT i = 1; i < groups.size(); ++i)
            if (groups[i].size() > groups[biggest].size()) biggest = i;
        if (groups[biggest].size() < 2) break;

        auto& g = groups[biggest];

        UnsignedInteger16 minX, maxX, minY, maxY;
        if (groups.size() == 1) {
            // first split only: reuse the state's precomputed bbox, skip the scan
            minX = state.x0; maxX = state.x1;
            minY = state.y0; maxY = state.y1;
        } else {
            minX = maxX = pixels[g[0]].x;
            minY = maxY = pixels[g[0]].y;
            for (UnsignedInteger32 idx : g) {
                minX = std::min(minX, pixels[idx].x); maxX = std::max(maxX, pixels[idx].x);
                minY = std::min(minY, pixels[idx].y); maxY = std::max(maxY, pixels[idx].y);
            }
        }
        bool splitOnX = (maxX - minX) >= (maxY - minY);

        SizeT mid = g.size() / 2;
        std::nth_element(g.begin(), g.begin() + mid, g.end(),
            [&](UnsignedInteger32 a, UnsignedInteger32 b) {
                return splitOnX ? pixels[a].x < pixels[b].x
                                 : pixels[a].y < pixels[b].y;
            });

        std::vector<UnsignedInteger32> right(g.begin() + mid, g.end());
        g.erase(g.begin() + mid, g.end());
        groups.push_back(std::move(right));
    }

    Vector<Pixel> randomPixels;
    randomPixels.reserve(groups.size());
    for (auto& g : groups) {
        std::uniform_int_distribution<SizeT> pick(0, g.size() - 1);
        randomPixels.push_back(pixels[g[pick(rng)]]);
    }
    return randomPixels;
}

Vector<UnsignedInteger16> assignRegions(const State& state, const Vector<Pixel>& seeds) {
    const auto& pixels = state.pixels;

    constexpr int32_t NOT_IN_STATE = -1;
    constexpr int32_t UNASSIGNED   = -2;

    // Create a vector of size state width * height that functions as a look up table
    // for whether a province is done or not
    Vector<SignedInteger32> label(size_t(state.width) * state.height, NOT_IN_STATE);
    auto localIndex = [&](UnsignedInteger16 x, UnsignedInteger16 y) {
        return size_t(y - state.y0) * state.width + (x - state.x0);
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

            size_t ni = localIndex(UnsignedInteger16(nx), UnsignedInteger16(ny));
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
        size_t li = localIndex(p.x, p.y);
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
    for (size_t i = 0; i < pixels.size(); ++i)
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

int main() {
    Timestamp startTime = std::chrono::high_resolution_clock::now();

    SignedInteger32 mapWidth{}, mapHeight{};
    Vector<State> statesVector = LoadStates(mapWidth, mapHeight);

    const SizeT provincesMapDataSize = mapWidth * mapHeight * 3;
    UnsignedInteger8 *provincesMapData = new UnsignedInteger8[provincesMapDataSize];
    std::fill_n(provincesMapData, provincesMapDataSize, 20);

    std::mt19937 rng(std::random_device{}());
    for (auto& state : statesVector) {
        // Select semi-random points in each state to be our province centers
        // One province = ~500 pixels for now
        SizeT n = std::max<SizeT>(1, state.pixels.size() / 500);

        Vector<Pixel> randomPixels = SelectRandomPixels(state, n, rng);
        Vector<UnsignedInteger16> provinces = assignRegions(state, randomPixels);


        // Generate n-random numbers for each province
        /*
        Vector<ColourRGB> colours = GenerateNRandomColours(
            randomPixels.size(),
            std::max(0, SignedInteger32(state.colour.r) - 25), std::min(255, SignedInteger32(state.colour.r) + 25),
            std::max(0, SignedInteger32(state.colour.g) - 25), std::min(255, SignedInteger32(state.colour.g) + 25),
            std::max(0, SignedInteger32(state.colour.b) - 25), std::min(255, SignedInteger32(state.colour.b) + 25),
            rng
        );
        */

        Vector<ColourRGB> colours = GenerateNRandomColours(
            randomPixels.size(),
            0, 255,
            0, 255,
            0, 255,
            rng
        );

        for (SizeT i = 0; i < state.pixels.size(); i++) {
            UnsignedInteger16 localId = provinces[i];
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
