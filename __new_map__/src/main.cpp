// g++ src/*.cpp -std=c++20 -O3 -static -o map_generator.exe
// g++ src/*.cpp -std=c++20 -O3 -o map_generator_mac

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
#include <cstdio>
#include <fstream>
#include <string>
#include <stdexcept>
#include <filesystem>


#include "functions.hpp"
#include "data_types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

constexpr Float64 PI = 3.14159265358979323846;




// ---- Settings ----
// Every tunable value lives in settings.txt (in the folder the program is run from, next to in/ and out/)
// and is read by LoadSettings() at start-up, so they can be changed without recompiling. See that file for
// what each one does. They're only written by LoadSettings - treat them as constants everywhere else.

SizeT THREAD_COUNT;
UnsignedInteger32 NO_PRESET_COLOUR;
UnsignedInteger32 LAND_COLOUR;
UnsignedInteger32 SEA_COLOUR;
UnsignedInteger32 LAKE_COLOUR;
SignedInteger32 STATE_COLOUR_VARIATION;
UnsignedInteger8 SEA_RED_MIN;
UnsignedInteger8 SEA_RED_MAX;
UnsignedInteger8 SEA_GREEN_MIN;
UnsignedInteger8 SEA_GREEN_MAX;
UnsignedInteger8 SEA_BLUE_MIN;
UnsignedInteger8 SEA_BLUE_MAX;
Float64 LAND_DENSITY_PER_PROVINCE;
Float64 SEA_DENSITY_PER_PROVINCE;
Float64 LAKE_DENSITY_PER_PROVINCE;
Float64 LAND_MIN_DENSITY;
Float64 SEA_MIN_DENSITY;
Float64 LAKE_MIN_DENSITY;
Float64 LAND_MAX_DENSITY;
Float64 SEA_MAX_DENSITY;
Float64 LAKE_MAX_DENSITY;
SizeT LAND_MIN_REGION_SIZE;
SizeT SEA_MIN_REGION_SIZE;
SizeT LAKE_MIN_REGION_SIZE;
Float64 LAND_MIN_RIVER_POCKET_SHARE;
SizeT LAND_ITERATIONS_PER_PIXEL;
SizeT SEA_ITERATIONS_PER_PIXEL;
SizeT LAKE_ITERATIONS_PER_PIXEL;
Float64 TEMPERATURE_START;
Float64 TEMPERATURE_END;
Float64 DISTANCE_WEIGHT;
Float64 DISTANCE_WEIGHT_RATIO_CAP;
Float64 LAND_TARGET_SIZE_SPREAD;
Float64 SEA_TARGET_SIZE_SPREAD;
Float64 LAKE_TARGET_SIZE_SPREAD;
Float64 LAND_SIZE_WEIGHT;
Float64 SEA_SIZE_WEIGHT;
Float64 LAKE_SIZE_WEIGHT;
Float64 LAND_NEIGHBOURHOOD_WEIGHT;
Float64 SEA_NEIGHBOURHOOD_WEIGHT;
Float64 LAKE_NEIGHBOURHOOD_WEIGHT;
SignedInteger32 NEIGHBOURHOOD_RADIUS;
Float64 LAND_TERRAIN_WEIGHT;
Float64 LAND_NOISE_STRENGTH;
Float64 SEA_NOISE_STRENGTH;
Float64 LAKE_NOISE_STRENGTH;
Float64 NOISE_WAVELENGTH;
SignedInteger32 NOISE_OCTAVE_LAYERS;
Boolean NOISE_RIDGED;
SizeT CLEANUP_PASSES;
SignedInteger32 THIN_RADIUS;
SizeT CLEANUP_ITERATIONS_PER_PIXEL;
Float64 LAND_SMOOTHNESS;
Float64 SEA_SMOOTHNESS;
Float64 LAKE_SMOOTHNESS;
SizeT STATE_MANPOWER;
String STATE_CATEGORY;
SizeT CONTINENT;

// The per-type settings, gathered so the code can look them up by province type (filled in by LoadSettings)
enum ProvinceType : UnsignedInteger8 { LAND_PROVINCE, SEA_PROVINCE, LAKE_PROVINCE, PROVINCE_TYPE_COUNT };
struct ProvinceTypeSettings {
    Float64 densityPerProvince, minDensity, maxDensity;
    SizeT minRegionSize, iterationsPerPixel;
    Float64 targetSizeSpread, sizeWeight, neighbourhoodWeight, terrainWeight, noiseStrength, smoothness;
};
ProvinceTypeSettings PROVINCE_TYPE_SETTINGS[PROVINCE_TYPE_COUNT];

void LoadSettings(const char* path) {
    enum Kind { SIZE, FLOAT, INT, COLOUR, BYTE, BOOL, TEXT };
    struct Setting { const char* name; Kind kind; void* target; Boolean found = false; };
    Setting settings[] = {
        { "THREAD_COUNT", SIZE, &THREAD_COUNT },
        { "NO_PRESET_COLOUR", COLOUR, &NO_PRESET_COLOUR },
        { "LAND_COLOUR", COLOUR, &LAND_COLOUR },
        { "SEA_COLOUR", COLOUR, &SEA_COLOUR },
        { "LAKE_COLOUR", COLOUR, &LAKE_COLOUR },
        { "STATE_COLOUR_VARIATION", INT, &STATE_COLOUR_VARIATION },
        { "SEA_RED_MIN", BYTE, &SEA_RED_MIN },
        { "SEA_RED_MAX", BYTE, &SEA_RED_MAX },
        { "SEA_GREEN_MIN", BYTE, &SEA_GREEN_MIN },
        { "SEA_GREEN_MAX", BYTE, &SEA_GREEN_MAX },
        { "SEA_BLUE_MIN", BYTE, &SEA_BLUE_MIN },
        { "SEA_BLUE_MAX", BYTE, &SEA_BLUE_MAX },
        { "LAND_DENSITY_PER_PROVINCE", FLOAT, &LAND_DENSITY_PER_PROVINCE },
        { "SEA_DENSITY_PER_PROVINCE", FLOAT, &SEA_DENSITY_PER_PROVINCE },
        { "LAKE_DENSITY_PER_PROVINCE", FLOAT, &LAKE_DENSITY_PER_PROVINCE },
        { "LAND_MIN_DENSITY", FLOAT, &LAND_MIN_DENSITY },
        { "SEA_MIN_DENSITY", FLOAT, &SEA_MIN_DENSITY },
        { "LAKE_MIN_DENSITY", FLOAT, &LAKE_MIN_DENSITY },
        { "LAND_MAX_DENSITY", FLOAT, &LAND_MAX_DENSITY },
        { "SEA_MAX_DENSITY", FLOAT, &SEA_MAX_DENSITY },
        { "LAKE_MAX_DENSITY", FLOAT, &LAKE_MAX_DENSITY },
        { "LAND_MIN_REGION_SIZE", SIZE, &LAND_MIN_REGION_SIZE },
        { "SEA_MIN_REGION_SIZE", SIZE, &SEA_MIN_REGION_SIZE },
        { "LAKE_MIN_REGION_SIZE", SIZE, &LAKE_MIN_REGION_SIZE },
        { "LAND_MIN_RIVER_POCKET_SHARE", FLOAT, &LAND_MIN_RIVER_POCKET_SHARE },
        { "LAND_ITERATIONS_PER_PIXEL", SIZE, &LAND_ITERATIONS_PER_PIXEL },
        { "SEA_ITERATIONS_PER_PIXEL", SIZE, &SEA_ITERATIONS_PER_PIXEL },
        { "LAKE_ITERATIONS_PER_PIXEL", SIZE, &LAKE_ITERATIONS_PER_PIXEL },
        { "TEMPERATURE_START", FLOAT, &TEMPERATURE_START },
        { "TEMPERATURE_END", FLOAT, &TEMPERATURE_END },
        { "DISTANCE_WEIGHT", FLOAT, &DISTANCE_WEIGHT },
        { "DISTANCE_WEIGHT_RATIO_CAP", FLOAT, &DISTANCE_WEIGHT_RATIO_CAP },
        { "LAND_TARGET_SIZE_SPREAD", FLOAT, &LAND_TARGET_SIZE_SPREAD },
        { "SEA_TARGET_SIZE_SPREAD", FLOAT, &SEA_TARGET_SIZE_SPREAD },
        { "LAKE_TARGET_SIZE_SPREAD", FLOAT, &LAKE_TARGET_SIZE_SPREAD },
        { "LAND_SIZE_WEIGHT", FLOAT, &LAND_SIZE_WEIGHT },
        { "SEA_SIZE_WEIGHT", FLOAT, &SEA_SIZE_WEIGHT },
        { "LAKE_SIZE_WEIGHT", FLOAT, &LAKE_SIZE_WEIGHT },
        { "LAND_NEIGHBOURHOOD_WEIGHT", FLOAT, &LAND_NEIGHBOURHOOD_WEIGHT },
        { "SEA_NEIGHBOURHOOD_WEIGHT", FLOAT, &SEA_NEIGHBOURHOOD_WEIGHT },
        { "LAKE_NEIGHBOURHOOD_WEIGHT", FLOAT, &LAKE_NEIGHBOURHOOD_WEIGHT },
        { "NEIGHBOURHOOD_RADIUS", INT, &NEIGHBOURHOOD_RADIUS },
        { "LAND_TERRAIN_WEIGHT", FLOAT, &LAND_TERRAIN_WEIGHT },
        { "LAND_NOISE_STRENGTH", FLOAT, &LAND_NOISE_STRENGTH },
        { "SEA_NOISE_STRENGTH", FLOAT, &SEA_NOISE_STRENGTH },
        { "LAKE_NOISE_STRENGTH", FLOAT, &LAKE_NOISE_STRENGTH },
        { "NOISE_WAVELENGTH", FLOAT, &NOISE_WAVELENGTH },
        { "NOISE_OCTAVE_LAYERS", INT, &NOISE_OCTAVE_LAYERS },
        { "NOISE_RIDGED", BOOL, &NOISE_RIDGED },
        { "CLEANUP_PASSES", SIZE, &CLEANUP_PASSES },
        { "THIN_RADIUS", INT, &THIN_RADIUS },
        { "CLEANUP_ITERATIONS_PER_PIXEL", SIZE, &CLEANUP_ITERATIONS_PER_PIXEL },
        { "LAND_SMOOTHNESS", FLOAT, &LAND_SMOOTHNESS },
        { "SEA_SMOOTHNESS", FLOAT, &SEA_SMOOTHNESS },
        { "LAKE_SMOOTHNESS", FLOAT, &LAKE_SMOOTHNESS },
        { "STATE_MANPOWER", SIZE, &STATE_MANPOWER },
        { "STATE_CATEGORY", TEXT, &STATE_CATEGORY },
        { "CONTINENT", SIZE, &CONTINENT },
    };

    std::ifstream file(path);
    if (!file) FatalError(String("ERROR: couldn't open ") + path);
    String line;
    SizeT lineNumber = 0;
    auto fail = [&](const String& why) {
        FatalError(String(path) + " line " + std::to_string(lineNumber) + ": " + why + "\n  " + line);
    };
    auto trim = [](String text) {
        const SizeT first = text.find_first_not_of(" \t\r"), last = text.find_last_not_of(" \t\r");
        return first == String::npos ? String() : text.substr(first, last - first + 1);
    };
    while (std::getline(file, line)) {
        ++lineNumber;
        const String content = trim(line.substr(0, line.find('#')));
        if (content.empty()) continue;
        const SizeT equals = content.find('=');
        if (equals == String::npos) fail("expected NAME = value");
        const String name = trim(content.substr(0, equals)), value = trim(content.substr(equals + 1));
        Setting* setting = nullptr;
        for (Setting& candidate : settings) if (name == candidate.name) setting = &candidate;
        if (setting == nullptr) fail("unknown setting \"" + name + "\"");
        if (setting->found) fail("\"" + name + "\" is set twice");
        setting->found = true;
        try {
            SizeT used = 0;
            switch (setting->kind) {
                case SIZE: {
                    if (value.find('-') != String::npos) fail("\"" + name + "\" can't be negative");
                    *static_cast<SizeT*>(setting->target) = SizeT(std::stoull(value, &used, 10)); break;
                }
                case FLOAT:  *static_cast<Float64*>(setting->target) = std::stod(value, &used); break;
                case INT:    *static_cast<SignedInteger32*>(setting->target) = SignedInteger32(std::stol(value, &used, 10)); break;
                case COLOUR: {
                    const UnsignedInteger64 colour = std::stoull(value, &used, 0);
                    if (colour > 0xFFFFFF) fail("\"" + name + "\" must be a colour from 0x000000 to 0xFFFFFF");
                    *static_cast<UnsignedInteger32*>(setting->target) = UnsignedInteger32(colour); break;
                }
                case BYTE: {
                    const SignedInteger64 byte = std::stoll(value, &used, 10);
                    if (byte < 0 || byte > 255) fail("\"" + name + "\" must be from 0 to 255");
                    *static_cast<UnsignedInteger8*>(setting->target) = UnsignedInteger8(byte); break;
                }
                case TEXT: {
                    if (value.empty() || value.find_first_of(" \t\"{}=") != String::npos)
                        fail("\"" + name + "\" must be a single word");
                    *static_cast<String*>(setting->target) = value;
                    used = value.size(); break;
                }
                case BOOL: {
                    if (value == "true") *static_cast<Boolean*>(setting->target) = true;
                    else if (value == "false") *static_cast<Boolean*>(setting->target) = false;
                    else fail("\"" + name + "\" must be true or false");
                    used = value.size(); break;
                }
            }
            if (used != value.size()) fail("couldn't read the value of \"" + name + "\"");
        }
        catch (const std::logic_error&) { fail("couldn't read the value of \"" + name + "\""); }
    }
    String missing;
    for (const Setting& setting : settings) if (!setting.found) missing += String("\n  ") + setting.name;
    if (!missing.empty()) FatalError(String("ERROR: settings missing from ") + path + ":" + missing);

    // A few values the code can't work with
    auto require = [&](Boolean ok, const String& why) { if (!ok) FatalError(String("ERROR: ") + path + ": " + why); };
    require(LAND_MIN_DENSITY > 0.0 && SEA_MIN_DENSITY > 0.0 && LAKE_MIN_DENSITY > 0.0, "MIN_DENSITY values must be above 0");
    require(LAND_MIN_DENSITY <= LAND_MAX_DENSITY && SEA_MIN_DENSITY <= SEA_MAX_DENSITY && LAKE_MIN_DENSITY <= LAKE_MAX_DENSITY,
            "each MIN_DENSITY must be at most its MAX_DENSITY");
    require(LAND_DENSITY_PER_PROVINCE > 0.0 && SEA_DENSITY_PER_PROVINCE > 0.0 && LAKE_DENSITY_PER_PROVINCE > 0.0,
            "DENSITY_PER_PROVINCE values must be above 0");
    require(SEA_RED_MIN <= SEA_RED_MAX && SEA_GREEN_MIN <= SEA_GREEN_MAX && SEA_BLUE_MIN <= SEA_BLUE_MAX,
            "each SEA_*_MIN must be at most its SEA_*_MAX");
    require(STATE_COLOUR_VARIATION >= 0, "STATE_COLOUR_VARIATION can't be negative");
    require(TEMPERATURE_START > 0.0 && TEMPERATURE_END > 0.0, "temperatures must be above 0");
    require(NEIGHBOURHOOD_RADIUS >= 1 && THIN_RADIUS >= 1, "NEIGHBOURHOOD_RADIUS and THIN_RADIUS must be at least 1");
    require(NOISE_WAVELENGTH > 0.0 && NOISE_OCTAVE_LAYERS >= 1, "NOISE_WAVELENGTH must be above 0 and NOISE_OCTAVE_LAYERS at least 1");
    require(LAND_COLOUR != SEA_COLOUR && LAND_COLOUR != LAKE_COLOUR && SEA_COLOUR != LAKE_COLOUR, "LAND/SEA/LAKE_COLOUR must all differ");

    PROVINCE_TYPE_SETTINGS[LAND_PROVINCE] = { LAND_DENSITY_PER_PROVINCE, LAND_MIN_DENSITY, LAND_MAX_DENSITY, LAND_MIN_REGION_SIZE, LAND_ITERATIONS_PER_PIXEL,
        LAND_TARGET_SIZE_SPREAD, LAND_SIZE_WEIGHT, LAND_NEIGHBOURHOOD_WEIGHT, LAND_TERRAIN_WEIGHT, LAND_NOISE_STRENGTH, LAND_SMOOTHNESS };
    PROVINCE_TYPE_SETTINGS[SEA_PROVINCE] = { SEA_DENSITY_PER_PROVINCE, SEA_MIN_DENSITY, SEA_MAX_DENSITY, SEA_MIN_REGION_SIZE, SEA_ITERATIONS_PER_PIXEL,
        SEA_TARGET_SIZE_SPREAD, SEA_SIZE_WEIGHT, SEA_NEIGHBOURHOOD_WEIGHT, 0.0, SEA_NOISE_STRENGTH, SEA_SMOOTHNESS };
    PROVINCE_TYPE_SETTINGS[LAKE_PROVINCE] = { LAKE_DENSITY_PER_PROVINCE, LAKE_MIN_DENSITY, LAKE_MAX_DENSITY, LAKE_MIN_REGION_SIZE, LAKE_ITERATIONS_PER_PIXEL,
        LAKE_TARGET_SIZE_SPREAD, LAKE_SIZE_WEIGHT, LAKE_NEIGHBOURHOOD_WEIGHT, 0.0, LAKE_NOISE_STRENGTH, LAKE_SMOOTHNESS };
}


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
    ColourRGB colour; // statemap colour, or strategic region colour for sea
    ProvinceType type = LAND_PROVINCE;
    UnsignedInteger16 x0 = UINT16_MAX, x1 = 0, y0 = UINT16_MAX, y1 = 0;
    UnsignedInteger16 width = 0, height = 0;

    // Each vector entry indicates one pixel. Vectors are kept separate to optimise RAM usage.
    // Merging them into one vector offers no actual speed improvements
    Vector<Pixel> pixels;
    Vector<Boolean> barrierPixels;
    Vector<Float32> densityWeights; // the type's MIN_DENSITY (black) to MAX_DENSITY (white), from densitymap.png
    Vector<TerrainType> terrains;

    // Preset provinces (presetprovinces.png). Their pixels are kept out of the vectors above so the generator
    // works around them, and are moved into pixels by MergePresetProvinces once the state has been generated
    Vector<Pixel> presetPixels;
    Vector<UnsignedInteger16> presetProvinceOf; // which of this state's preset provinces each preset pixel is in
    UnsignedInteger16 presetProvinceCount = 0;
    Vector<TerrainType> presetTerrains;

    // The strategic region the state's provinces go in: the region most of the state's pixels are in
    // (a state can't be split between regions in game). For sea, the region the state was made from
    UnsignedInteger32 regionColour = 0;

    State(): colour(), pixels(), barrierPixels(), densityWeights(), terrains() {
        pixels.reserve(6000);
        barrierPixels.reserve(6000);
        densityWeights.reserve(6000);
        terrains.reserve(6000);
    }
    State(const ColourRGB colour, const ProvinceType type): colour(colour), type(type), pixels(), barrierPixels(), densityWeights(), terrains() {
        pixels.reserve(6000);
        barrierPixels.reserve(6000);
        densityWeights.reserve(6000);
        terrains.reserve(6000);
    }

    void AddPixel(const Pixel p) {
        pixels.push_back(p);

        if (p.x < x0) { x0 = p.x; }
        if (p.x > x1) { x1 = p.x; }
        if (p.y < y0) { y0 = p.y; }
        if (p.y > y1) { y1 = p.y; }
    }
    void AddPixel(const UnsignedInteger16 x, const UnsignedInteger16 y) {
        pixels.emplace_back(x, y);

        if (x < x0) { x0 = x; }
        if (x > x1) { x1 = x; }
        if (y < y0) { y0 = y; }
        if (y > y1) { y1 = y; }
    }
    void AddTerrain(const ColourRGB colour) {
        // Sea and lake provinces ignore terrain
        terrains.push_back(type == LAND_PROVINCE ? TerrainFromColour(colour.ToInteger()) : NO_TERRAIN);
    }
    void AddDensity(const ColourRGB colour) {
        const ProvinceTypeSettings& settings = PROVINCE_TYPE_SETTINGS[type];
        const Float32 brightness = Float32(UnsignedInteger32(colour.r) + colour.g + colour.b) / 765.0;
        densityWeights.push_back(
            settings.minDensity + (settings.maxDensity - settings.minDensity) * brightness
        );
    }
    void AddBarrierPixel(const UnsignedInteger32 colour) {
        // Land/water colour on rivers.png. Rivers are only barriers on land.
        if (type != LAND_PROVINCE || colour == 0x00ffffff || colour == 0x007a7a7a) {
            barrierPixels.push_back(false);
        }
        else {
            barrierPixels.push_back(true);
        }
    }

    void AddPresetPixel(const UnsignedInteger16 x, const UnsignedInteger16 y, const UnsignedInteger16 presetProvince, const ColourRGB terrainColour) {
        presetPixels.emplace_back(x, y);
        presetProvinceOf.push_back(presetProvince);
        presetTerrains.push_back(type == LAND_PROVINCE ? TerrainFromColour(terrainColour.ToInteger()) : NO_TERRAIN);
    }

    // Move the preset provinces into pixels (and terrains), numbered after the generated provinces.
    // barrierPixels and densityWeights don't grow - they're only needed while generating
    void MergePresetProvinces(Vector<UnsignedInteger16>& provinceOf, UnsignedInteger16& provinceCount) {
        pixels.reserve(pixels.size() + presetPixels.size());
        provinceOf.reserve(provinceOf.size() + presetPixels.size());
        for (SizeT i = 0; i < presetPixels.size(); ++i) {
            AddPixel(presetPixels[i]);
            terrains.push_back(presetTerrains[i]);
            provinceOf.push_back(UnsignedInteger16(provinceCount + presetProvinceOf[i]));
        }
        provinceCount += presetProvinceCount;
        UpdateBoundaries();
        Vector<Pixel>().swap(presetPixels);
        Vector<UnsignedInteger16>().swap(presetProvinceOf);
        Vector<TerrainType>().swap(presetTerrains);
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
    Vector<State> statesVector; statesVector.reserve(2000);

    // Decode the input images at the same time - PNG decoding is the slow part of loading
    struct MapImage { const char* path; SignedInteger32 width = 0, height = 0, channels = 0; UnsignedInteger8* data = nullptr; };
    MapImage stateMap{ "in/statemap.png" }, riverMap{ "in/rivers.png" }, densityMap{ "in/densitymap.png" },
             terrainMap{ "in/terrain.png" }, typeMap{ "in/provtypemap.png" }, regionMap{ "in/strategicregionmap.png" },
             presetMap{ "in/presetprovinces.png" };
    {
        Vector<std::thread> loaders;
        for (MapImage* image : { &riverMap, &densityMap, &terrainMap, &typeMap, &regionMap, &presetMap })
            loaders.emplace_back([image]() { image->data = stbi_load(image->path, &image->width, &image->height, &image->channels, 4); });
        stateMap.data = stbi_load(stateMap.path, &stateMap.width, &stateMap.height, &stateMap.channels, 4);
        for (auto& loader : loaders) loader.join();
    }
    mapWidth = stateMap.width;
    mapHeight = stateMap.height;
    for (const MapImage* image : { &stateMap, &riverMap, &densityMap, &terrainMap, &typeMap, &regionMap, &presetMap }) {
        if (image->data == nullptr) FatalError(String("ERROR: couldn't load ") + image->path);
        if (image->width != mapWidth || image->height != mapHeight)
            FatalError(String("ERROR: statemap.png and ") + image->path + " have different sizes.");
    }

    auto colourAt = [](const MapImage& image, SizeT pixel) {
        return ColourRGB(image.data[pixel * 4 + 0], image.data[pixel * 4 + 1], image.data[pixel * 4 + 2]);
    };
    auto typeAt = [&](SizeT pixel) {
        const UnsignedInteger32 c = colourAt(typeMap, pixel).ToInteger();
        if (c == SEA_COLOUR) return SEA_PROVINCE;
        if (c == LAKE_COLOUR) return LAKE_PROVINCE;
        return LAND_PROVINCE; // LAND_COLOUR, and anything unexpected
    };

    // ---- Check statemap.png and provtypemap.png agree on where the sea is ----
    // Sea pixels must have no state, and every pixel with no state must be sea
    constexpr UnsignedInteger32 NO_STATE_COLOUR = 0x141414;
    const SizeT totalMapPixels = SizeT(mapWidth) * mapHeight;
    {
        SizeT seaWithState = 0, statelessNotSea = 0;
        String examples;
        SizeT exampleCount = 0;
        for (SizeT pixel = 0; pixel < totalMapPixels; ++pixel) {
            const Boolean isSea = typeAt(pixel) == SEA_PROVINCE;
            const Boolean hasState = colourAt(stateMap, pixel).ToInteger() != NO_STATE_COLOUR;
            if (isSea == hasState) {
                (isSea ? seaWithState : statelessNotSea)++;
                if (exampleCount++ < 10)
                    examples += "\n  (" + std::to_string(pixel % mapWidth) + ", " + std::to_string(pixel / mapWidth) + ") "
                              + (isSea ? "sea with a state" : "no state but not sea");
            }
        }
        if (seaWithState + statelessNotSea > 0)
            FatalError("ERROR: statemap.png and provtypemap.png don't agree on the sea: "
                       + std::to_string(seaWithState) + " sea pixels have a state, "
                       + std::to_string(statelessNotSea) + " pixels with no state aren't sea. First few:" + examples);
    }

    // ---- Work out every pixel's group (the State it goes into) ----
    // Sea is grouped by strategic region, land and lake by state. Groups are keyed by type and colour
    Vector<UnsignedInteger16> groupOf(totalMapPixels);
    HashMap<UnsignedInteger64, UnsignedInteger16> groupIndex;
    Vector<std::pair<ColourRGB, ProvinceType>> groups;
    UnsignedInteger64 previousKey = UINT64_MAX;
    UnsignedInteger16 previousGroup = 0;
    for (SizeT pixel = 0; pixel < totalMapPixels; ++pixel) {
        const ProvinceType type = typeAt(pixel);
        const ColourRGB colour = type == SEA_PROVINCE ? colourAt(regionMap, pixel) : colourAt(stateMap, pixel);
        const UnsignedInteger64 key = (UnsignedInteger64(type) << 32) | colour.ToInteger();
        if (key != previousKey) {
            const auto found = groupIndex.find(key);
            if (found != groupIndex.end()) previousGroup = found->second;
            else {
                previousGroup = UnsignedInteger16(groups.size());
                groupIndex[key] = previousGroup;
                groups.emplace_back(colour, type);
            }
            previousKey = key;
        }
        groupOf[pixel] = previousGroup;
    }

    // ---- Preset provinces ----
    // presetOf = the pixel's preset province (an index into presetGroup/presetLocalIndex), or -1
    constexpr SignedInteger32 NOT_PRESET = -1;
    Vector<SignedInteger32> presetOf(totalMapPixels, NOT_PRESET);
    Vector<UnsignedInteger16> presetGroup, presetLocalIndex; // its group, and its number within that group
    Vector<UnsignedInteger16> groupPresetCount(groups.size(), 0);
    {
        HashMap<UnsignedInteger32, SignedInteger32> presetIndex;
        Vector<SizeT> presetFirstPixel;
        HashMap<SignedInteger32, SizeT> crossings; // preset -> first pixel found outside its group
        UnsignedInteger32 previousColour = NO_PRESET_COLOUR;
        SignedInteger32 previousPreset = NOT_PRESET;
        for (SizeT pixel = 0; pixel < totalMapPixels; ++pixel) {
            const UnsignedInteger32 c = colourAt(presetMap, pixel).ToInteger();
            if (c == NO_PRESET_COLOUR) continue;
            if (c != previousColour) {
                const auto found = presetIndex.find(c);
                if (found != presetIndex.end()) previousPreset = found->second;
                else {
                    previousPreset = SignedInteger32(presetGroup.size());
                    presetIndex[c] = previousPreset;
                    presetGroup.push_back(groupOf[pixel]);
                    presetLocalIndex.push_back(groupPresetCount[groupOf[pixel]]++);
                    presetFirstPixel.push_back(pixel);
                }
                previousColour = c;
            }
            if (presetGroup[previousPreset] != groupOf[pixel]) crossings.try_emplace(previousPreset, pixel);
            presetOf[pixel] = previousPreset;
        }
        if (!crossings.empty()) {
            auto describe = [&](SizeT pixel) {
                const UnsignedInteger16 g = groupOf[pixel];
                const ColourRGB c = groups[g].first;
                const char* typeName = groups[g].second == SEA_PROVINCE ? "sea, strategic region" : groups[g].second == LAKE_PROVINCE ? "lake, state" : "land, state";
                char text[96];
                std::snprintf(text, sizeof(text), "(%zu, %zu) [%s %02X%02X%02X]", pixel % mapWidth, pixel / mapWidth, typeName, c.r, c.g, c.b);
                return String(text);
            };
            String message = "ERROR: " + std::to_string(crossings.size()) +
                " preset province(s) in presetprovinces.png cross a state, strategic region or province type border:";
            SizeT listed = 0;
            for (const auto& [preset, pixel] : crossings) {
                if (listed++ == 10) { message += "\n  ..."; break; }
                char colour[16];
                const ColourRGB c = colourAt(presetMap, pixel);
                std::snprintf(colour, sizeof(colour), "%02X%02X%02X", c.r, c.g, c.b);
                message += String("\n  ") + colour + ": " + describe(presetFirstPixel[preset]) + " and " + describe(pixel);
            }
            FatalError(message);
        }
    }

    // ---- Each group's strategic region: where most of its pixels are ----
    Vector<UnsignedInteger32> groupRegion(groups.size(), 0);
    {
        Vector<HashMap<UnsignedInteger32, SizeT>> regionCounts(groups.size());
        SizeT run = 0;
        UnsignedInteger16 runGroup = groupOf[0];
        UnsignedInteger32 runRegion = colourAt(regionMap, 0).ToInteger();
        for (SizeT pixel = 0; pixel <= totalMapPixels; ++pixel) {
            const Boolean end = pixel == totalMapPixels;
            const UnsignedInteger32 region = end ? 0 : colourAt(regionMap, pixel).ToInteger();
            if (end || groupOf[pixel] != runGroup || region != runRegion) {
                regionCounts[runGroup][runRegion] += run;
                if (end) break;
                run = 0; runGroup = groupOf[pixel]; runRegion = region;
            }
            ++run;
        }
        Vector<std::pair<SizeT, UnsignedInteger32>> split; // pixels outside the chosen region, state colour
        for (SizeT g = 0; g < groups.size(); ++g) {
            SizeT best = 0, total = 0;
            for (const auto& [region, count] : regionCounts[g]) {
                total += count;
                if (count > best || (count == best && region < groupRegion[g])) { best = count; groupRegion[g] = region; }
            }
            if (groups[g].second == LAND_PROVINCE && best < total) split.emplace_back(total - best, groups[g].first.ToInteger());
        }
        if (!split.empty()) {
            std::sort(split.rbegin(), split.rend());
            std::cerr << "WARNING: " << split.size() << " state(s) cross a strategic region border - each is put in the region most "
                         "of it is in. Most pixels outside that region:";
            for (SizeT i = 0; i < split.size() && i < 10; ++i) {
                char text[48];
                std::snprintf(text, sizeof(text), "%s state %06X (%zu px)", i ? "," : "", split[i].second, split[i].first);
                std::cerr << text;
            }
            std::cerr << (split.size() > 10 ? ", ...\n" : "\n");
        }
    }

    // ---- Build the States, adding pixels row by row (the balancer relies on that order) ----
    for (const auto& [colour, type] : groups) statesVector.emplace_back(colour, type);
    for (SizeT g = 0; g < groups.size(); ++g) {
        statesVector[g].presetProvinceCount = groupPresetCount[g];
        statesVector[g].regionColour = groupRegion[g];
    }
    for (SizeT pixel = 0; pixel < totalMapPixels; ++pixel) {
        State& state = statesVector[groupOf[pixel]];
        if (presetOf[pixel] != NOT_PRESET) {
            state.AddPresetPixel(UnsignedInteger16(pixel % mapWidth), UnsignedInteger16(pixel / mapWidth), presetLocalIndex[presetOf[pixel]], colourAt(terrainMap, pixel));
            continue;
        }
        state.AddPixel(UnsignedInteger16(pixel % mapWidth), UnsignedInteger16(pixel / mapWidth));
        state.AddBarrierPixel(colourAt(riverMap, pixel).ToInteger());
        state.AddDensity(colourAt(densityMap, pixel));
        state.AddTerrain(colourAt(terrainMap, pixel));
    }

    for (const MapImage* image : { &stateMap, &riverMap, &densityMap, &terrainMap, &typeMap, &regionMap, &presetMap })
        stbi_image_free(image->data);

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
    const Vector<PixelType>& pixelTypes,
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
        if (pixelTypes[start] != LAND || regionOf[start] >= 0) continue;
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
                const SignedInteger32 n = grid[SizeT(ny) * state.width + nx];
                if (n >= 0 && pixelTypes[n] == LAND && regionOf[n] < 0) { regionOf[n] = id; stack.push_back(SizeT(n)); }
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
    // An area is too small if it has fewer than minRegionSize pixels, or (land only) less than
    // LAND_MIN_RIVER_POCKET_SHARE of a province's density
    const ProvinceTypeSettings& settings = PROVINCE_TYPE_SETTINGS[state.type];
    const Float64 minPocketDensity = state.type == LAND_PROVINCE ? LAND_MIN_RIVER_POCKET_SHARE * settings.densityPerProvince : 0.0;
    Vector<SignedInteger32> regionOf;
    Vector<Float64> densities;
    for (SizeT round = 0; round < 8; ++round) {
        const Vector<SizeT> sizes = LabelLandRegions(state, grid, pixelTypes, regionOf);
        if (sizes.size() <= 1) break;
        densities.assign(sizes.size(), 0.0);
        for (SizeT i = 0; i < pixelTypes.size(); ++i)
            if (regionOf[i] >= 0) densities[regionOf[i]] += state.densityWeights[i];
        Vector<SizeT> toOpen;
        for (SizeT i = 0; i < pixelTypes.size(); ++i) {
            if (regionOf[i] < 0) continue;
            const SignedInteger32 r = regionOf[i];
            if (sizes[r] >= settings.minRegionSize && densities[r] >= minPocketDensity) continue;
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
Vector<Pixel> SelectSeeds(const State& state, const Vector<PixelType>& pixelTypes, std::mt19937& rng) {
    const Vector<SignedInteger32> grid = state.GetLocalPixelGrid();
    Vector<SignedInteger32> regionOf;
    const Vector<SizeT> sizes = LabelLandRegions(state, grid, pixelTypes, regionOf);
    if (sizes.empty()) return { state.pixels.front() };

    Vector<Vector<SizeT>> regionPixels(sizes.size());
    Vector<Float64> regionDensity(sizes.size(), 0.0);
    for (SizeT i = 0; i < state.pixels.size(); ++i) {
        if (regionOf[i] < 0) continue;
        regionPixels[regionOf[i]].push_back(i);
        regionDensity[regionOf[i]] += state.densityWeights[i];
    }

    // Areas touching a preset province are cut off by it, so they get a province however small they are
    Vector<Boolean> touchesPreset(sizes.size(), false);
    {
        static const SignedInteger32 dx4[4] = { 1, 0, -1, 0 }, dy4[4] = { 0, 1, 0, -1 };
        for (const Pixel& p : state.presetPixels) {
            for (SignedInteger32 d = 0; d < 4; ++d) {
                const SignedInteger32 nx = SignedInteger32(p.x) - state.x0 + dx4[d], ny = SignedInteger32(p.y) - state.y0 + dy4[d];
                if (nx < 0 || ny < 0 || nx >= state.width || ny >= state.height) continue;
                const SignedInteger32 n = grid[SizeT(ny) * state.width + nx];
                if (n >= 0 && regionOf[n] >= 0) touchesPreset[regionOf[n]] = true;
            }
        }
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
        const ProvinceTypeSettings& settings = PROVINCE_TYPE_SETTINGS[state.type];
        if (sizes[r] < settings.minRegionSize && !touchesPreset[r]) continue;
        const SizeT n = std::max<SizeT>(1, SizeT(std::llround(regionDensity[r] / settings.densityPerProvince)));
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
Vector<UnsignedInteger16> AssignProvinces(const State& state, const Vector<PixelType>& pixelTypes, const Vector<Pixel>& seeds) {
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
            if (n >= 0 && pixelTypes[n] == LAND && label[n] == UNASSIGNED) { label[n] = label[i]; frontier.push(SizeT(n)); }
        }
    }
    // Barrier pixels join a province touching them from land
    for (SizeT i = 0; i < label.size(); ++i) {
        if (pixelTypes[i] != BARRIER) continue;
        for (SignedInteger32 d = 0; d < 4; ++d) {
            const SignedInteger32 n = sideNeighbour(i, d);
            if (n >= 0 && pixelTypes[n] == LAND && label[n] != UNASSIGNED) { label[i] = label[n]; break; }
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

    // Centre of gravity and radius (of a circle of the same area), cached by UpdateCache() so the flip
    // loop's pixel picking doesn't recompute them on every try
    Float64 centreX = 0.0, centreY = 0.0, radius = 1.0;

    Float64 CentreX() const { return Float64(sumX) / Float64(pixelCount); }
    Float64 CentreY() const { return Float64(sumY) / Float64(pixelCount); }
    void UpdateCache() {
        centreX = CentreX();
        centreY = CentreY();
        radius = std::max(1.0, std::sqrt(Float64(pixelCount) / PI));
    }

    // The province's most common terrain type (NO_TERRAIN if it has no terrain pixels)
    UnsignedInteger8 MainTerrain() const {
        if (terrainTotal == 0) return NO_TERRAIN;
        return UnsignedInteger8(std::max_element(terrainCount.begin(), terrainCount.end()) - terrainCount.begin());
    }

    // 1 - sum over terrain types of (share of that type)^2: 0 for a single terrain, higher when mixed
    Float64 TerrainMix() const { return TerrainMixOf(terrainSquares, terrainTotal); }
    static Float64 TerrainMixOf(const UnsignedInteger64 squares, const UnsignedInteger32 total) {
        if (total == 0) return 0.0;
        return 1.0 - Float64(squares) / (Float64(total) * Float64(total));
    }
    // What TerrainMix() would be after Remove() / Add() of a pixel of this terrain, without changing anything
    Float64 TerrainMixAfterRemoving(const UnsignedInteger8 terrain) const {
        if (terrain >= TERRAIN_TYPE_COUNT) return TerrainMix();
        return TerrainMixOf(terrainSquares - (2 * UnsignedInteger64(terrainCount[terrain]) - 1), terrainTotal - 1);
    }
    Float64 TerrainMixAfterAdding(const UnsignedInteger8 terrain) const {
        if (terrain >= TERRAIN_TYPE_COUNT) return TerrainMix();
        return TerrainMixOf(terrainSquares + (2 * UnsignedInteger64(terrainCount[terrain]) + 1), terrainTotal + 1);
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

Float64 ScoreMap(const Float64 sizeErrorSum, const Float64 neighbourhoodScore, const Float64 terrainMixSum, const SizeT provincesCount,
                 const ProvinceTypeSettings& settings) {
    return (settings.sizeWeight * sizeErrorSum + settings.terrainWeight * terrainMixSum) / Float64(provincesCount)
        + settings.neighbourhoodWeight * neighbourhoodScore;
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
    const Vector<PixelType>& pixelTypes;
    const UnsignedInteger16 provincesCount;
    std::mt19937& rng;
    // This state's province type settings (land / sea / lake)
    const ProvinceTypeSettings& settings;

    // ---- Local grids covering the state's bounding box (index = y * W + x) ----
    const SignedInteger32 W, H;
    const SizeT G;
    const SizeT totalPixels;
    // Province id per pixel, or -1 if not in this state
    Vector<SignedInteger32> label;
    // Pixel pixelTypes (see PixelType). Inner barrier pixels sit out the optimisation as if they weren't in
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
    // The same neighbours as index steps in the grid, for pixels away from its edges (see IsInterior)
    SignedInteger64 sideSteps[4] = {}, ringSteps[8] = {};

    // ---- Running totals the score is built from ----
    Vector<Province> provinces;
    // Random target densities, rescaled so they add up to the state's total density
    Vector<Float64> targetSize;
    Float64 sizeErrorSum = 0.0;
    Float64 terrainMixSum = 0.0;
    // Sum over ordered (pixel, pixel-in-disc) pairs in different provinces of the pair's terrain cost
    Float64 neighbourhoodEnergy = 0.0;

    // ---- Neighbourhood border score constants ----
    const Boolean useNeighbourhood = settings.neighbourhoodWeight > 0.0;
    Vector<std::pair<SignedInteger32, SignedInteger32>> neighbourhoodOffsets;
    // The same offsets as index steps in the grid (oy * W + ox), for pixels far enough from its edges
    Vector<SignedInteger64> neighbourhoodSteps;
    // Grid index of each of the state's pixels, in state.pixels order - which is row by row, the same
    // order as the grid - so loops over it visit pixels in exactly the order a full grid scan would,
    // without wading through the empty parts of sparse states' bounding boxes
    Vector<UnsignedInteger32> pixelIndex;
    SignedInteger64 crossingsPerUnitBorder = 0; // what a straight border of length 1 adds to the energy
    Float64 neighbourhoodScale = 0.0;
    // Score change of adding one pixel of straight border, the unit temperatures are measured in
    Float64 borderPixelScore = 0.0;
    Vector<std::pair<SignedInteger32, SignedInteger32>> thinOffsets;
    Vector<SignedInteger64> thinSteps; // thinOffsets as index steps in the grid

    // ---- Boundary pixel set (pixels with at least one side touching another province) ----
    // Kept as a flat list plus a position lookup, so add/remove are both O(1)
    Vector<UnsignedInteger32> boundary;
    Vector<SignedInteger32> boundaryPos;

    // Largest possible pixel weight when picking pixels to flip (see FlipWeight)
    Float64 maxWeight = 4.0 * (1.0 + DISTANCE_WEIGHT * DISTANCE_WEIGHT_RATIO_CAP);
    std::uniform_real_distribution<Float64> unit{ 0.0, 1.0 };
    // The flip loop's own random numbers: 64-bit, so each random double costs one draw instead of two
    std::mt19937_64 flipRng;

    ProvinceBalancer(
        const State& state,
        const Vector<PixelType>& pixelTypes,
        const Vector<UnsignedInteger16>& pixelProvinceIds,
        const UnsignedInteger16 provincesCount,
        std::mt19937& rng
    ) : state(state), pixelTypes(pixelTypes), provincesCount(provincesCount), rng(rng), settings(PROVINCE_TYPE_SETTINGS[state.type]),
        W(state.width), H(state.height), G(SizeT(state.width) * state.height), totalPixels(state.pixels.size()),
        label(G, -1), kindGrid(G, INNER_BARRIER), weightGrid(G, 0.0), terrainGrid(G, NO_TERRAIN), terrainCost(G, 1.0),
        provinces(provincesCount), targetSize(provincesCount), boundaryPos(G, -1)
    {
        pixelIndex.reserve(totalPixels);
        for (SizeT i = 0; i < totalPixels; ++i) {
            const SizeT li = LocalIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0);
            pixelIndex.push_back(UnsignedInteger32(li));
            kindGrid[li] = pixelTypes[i];
            weightGrid[li] = state.densityWeights[i];
            terrainGrid[li] = state.terrains[i];
            if (pixelTypes[i] != INNER_BARRIER) totalDensity += weightGrid[li];
            if (pixelTypes[i] != INNER_BARRIER) label[li] = pixelProvinceIds[i];
        }

        // Random target densities
        {
            std::normal_distribution<Float64> normal(0.0, settings.targetSizeSpread);
            Float64 total = 0.0;
            for (auto& t : targetSize) { t = std::exp(normal(rng)); total += t; }
            for (auto& t : targetSize) t *= totalDensity / total;
        }

        // Terrain cost from the noise field
        {
            const UnsignedInteger32 noiseSeed = UnsignedInteger32(rng());
            Float64 costSum = 0.0;
            // Only the state's own pixels are ever used, so only they need the (relatively slow) noise
            for (const UnsignedInteger32 li : pixelIndex) {
                const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
                Float64 n = FractalNoise(Float64(x + state.x0), Float64(y + state.y0), noiseSeed);
                if (NOISE_RIDGED) n = std::min(1.0, 2.0 * std::abs(2.0 * n - 1.0));
                terrainCost[li] = 1.0 + settings.noiseStrength * n;
                if (label[li] >= 0) costSum += terrainCost[li];
            }
            // Rescale so the average in-state cost is 1, keeping the score's overall scale unchanged
            const Float64 meanCost = costSum / Float64(totalPixels);
            for (auto& c : terrainCost) c /= meanCost;
        }

        // Neighbourhood score constants, normalised so a map of circles scores about 1
        for (SignedInteger32 d = 0; d < 4; ++d) sideSteps[d] = SignedInteger64(dy4[d]) * W + dx4[d];
        for (SignedInteger32 k = 0; k < 8; ++k) ringSteps[k] = SignedInteger64(ringY[k]) * W + ringX[k];
        neighbourhoodOffsets = DiscOffsets(NEIGHBOURHOOD_RADIUS);
        for (const auto& [ox, oy] : neighbourhoodOffsets) neighbourhoodSteps.push_back(SignedInteger64(oy) * W + ox);
        for (const auto& [ox, oy] : neighbourhoodOffsets) crossingsPerUnitBorder += std::abs(oy);
        const Float64 meanSize = Float64(totalPixels) / Float64(provincesCount);
        neighbourhoodScale = 1.0 / (Float64(std::max<SignedInteger64>(1, crossingsPerUnitBorder))
            * Float64(provincesCount) * 2.0 * std::sqrt(PI * meanSize));
        borderPixelScore = settings.neighbourhoodWeight * neighbourhoodScale * 2.0 * Float64(crossingsPerUnitBorder);

        thinOffsets = DiscOffsets(THIN_RADIUS);
        for (const auto& [ox, oy] : thinOffsets) thinSteps.push_back(SignedInteger64(oy) * W + ox);
        flipRng.seed(rng());
        boundary.reserve(totalPixels / 4);
    }

    // ---- The whole process ----
    void Run() {
        ReattachBarrierPixels(nullptr);
        Rebuild();
        Anneal(settings.iterationsPerPixel * totalPixels, TEMPERATURE_START, TEMPERATURE_END);

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

    // At least `margin` pixels from every edge of the grid, so neighbours up to that far away can be
    // looked up by index step without bounds checks
    Boolean IsInterior(SignedInteger32 x, SignedInteger32 y, SignedInteger32 margin) const {
        return x >= margin && y >= margin && x < W - margin && y < H - margin;
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

    // Total pair cost between (x, y) and the pixels in its disc that belong to province `a`, and the
    // same for province `b`, in one pass. Pixels at least NEIGHBOURHOOD_RADIUS from the grid's edges
    // (nearly all of them) skip the bounds checks and step straight through the grid.
    void CostInDisc(SignedInteger32 x, SignedInteger32 y, SignedInteger32 a, SignedInteger32 b, Float64& costA, Float64& costB) const {
        const SizeT li = LocalIndex(x, y);
        const Float64 own = terrainCost[li];
        costA = 0.0;
        costB = 0.0;
        if (x >= NEIGHBOURHOOD_RADIUS && y >= NEIGHBOURHOOD_RADIUS && x < W - NEIGHBOURHOOD_RADIUS && y < H - NEIGHBOURHOOD_RADIUS) {
            for (const SignedInteger64 step : neighbourhoodSteps) {
                const SizeT ni = SizeT(SignedInteger64(li) + step);
                const SignedInteger32 n = label[ni];
                if (n == a) costA += 0.5 * (own + terrainCost[ni]);
                else if (n == b) costB += 0.5 * (own + terrainCost[ni]);
            }
            return;
        }
        for (const auto& [ox, oy] : neighbourhoodOffsets) {
            const SignedInteger32 n = LabelAt(x + ox, y + oy);
            if (n == a) costA += 0.5 * (own + terrainCost[LocalIndex(x + ox, y + oy)]);
            else if (n == b) costB += 0.5 * (own + terrainCost[LocalIndex(x + ox, y + oy)]);
        }
    }

    SignedInteger32 CountForeignSides(SignedInteger32 x, SignedInteger32 y) const {
        const SignedInteger32 own = label[LocalIndex(x, y)];
        SignedInteger32 count = 0;
        if (IsInterior(x, y, 1)) {
            const SignedInteger64 li = SignedInteger64(LocalIndex(x, y));
            for (const SignedInteger64 step : sideSteps) {
                const SignedInteger32 n = label[SizeT(li + step)];
                if (n >= 0 && n != own) ++count;
            }
            return count;
        }
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
        for (const UnsignedInteger32 li : pixelIndex) {
            const SignedInteger32 own = label[li];
            if (own >= 0) provinces[own].Add(SignedInteger32(li % W), SignedInteger32(li / W), weightGrid[li], terrainGrid[li]);
        }

        for (auto& p : provinces) p.UpdateCache();

        sizeErrorSum = 0.0;
        terrainMixSum = 0.0;
        for (SizeT i = 0; i < provincesCount; ++i) {
            sizeErrorSum += SizeError(provinces[i].value, targetSize[i]);
            terrainMixSum += provinces[i].TerrainMix();
        }

        neighbourhoodEnergy = 0.0;
        if (useNeighbourhood) {
            for (const UnsignedInteger32 li : pixelIndex) {
                const SignedInteger32 own = label[li];
                if (own < 0) continue;
                const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
                if (IsInterior(x, y, NEIGHBOURHOOD_RADIUS)) {
                    for (const SignedInteger64 step : neighbourhoodSteps) {
                        const SizeT ni = SizeT(SignedInteger64(li) + step);
                        const SignedInteger32 n = label[ni];
                        if (n >= 0 && n != own) neighbourhoodEnergy += 0.5 * (terrainCost[li] + terrainCost[ni]);
                    }
                    continue;
                }
                for (const auto& [ox, oy] : neighbourhoodOffsets) {
                    const SignedInteger32 n = LabelAt(x + ox, y + oy);
                    if (n >= 0 && n != own)
                        neighbourhoodEnergy += 0.5 * (terrainCost[li] + terrainCost[LocalIndex(x + ox, y + oy)]);
                }
            }
        }

        for (const UnsignedInteger32 li : boundary) boundaryPos[li] = -1;
        boundary.clear();
        for (const UnsignedInteger32 li : pixelIndex) RefreshBoundary(SignedInteger32(li % W), SignedInteger32(li / W));
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
    // CanRemove for a land pixel at least 2 pixels from the grid's edges: the same checks, but with
    // neighbours looked up by index step instead of bounds-checked coordinates
    Boolean CanRemoveInterior(SignedInteger32 x, SignedInteger32 y, SignedInteger32 own) const {
        const SignedInteger64 li = SignedInteger64(LocalIndex(x, y));
        for (const SignedInteger64 side : sideSteps) {
            const SizeT bi = SizeT(li + side);
            if (kindGrid[bi] != BARRIER || label[bi] != own) continue;
            Boolean stillAttached = false;
            for (const SignedInteger64 step : sideSteps) {
                const SizeT ni = SizeT(SignedInteger64(bi) + step);
                if (SignedInteger64(ni) != li && kindGrid[ni] == LAND && label[ni] == own) stillAttached = true;
            }
            if (!stillAttached) return false;
        }

        Boolean inProvince[8];
        SignedInteger32 start = -1;
        for (SignedInteger32 k = 0; k < 8; ++k) {
            const SizeT ni = SizeT(li + ringSteps[k]);
            inProvince[k] = label[ni] == own && kindGrid[ni] == LAND;
            if (!inProvince[k] && start < 0) start = k;
        }
        if (start < 0) return true;

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
        return runsTouchingSides <= 1;
    }

    Boolean CanRemove(SignedInteger32 x, SignedInteger32 y, SignedInteger32 own) const {
        if (provinces[own].pixelCount <= 1) return false;
        if (!IsLand(x, y)) return true;
        if (IsInterior(x, y, 2)) return CanRemoveInterior(x, y, own);

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
        const Float64 ddx = Float64(x) - p.centreX, ddy = Float64(y) - p.centreY;
        const Float64 ratio = std::min(DISTANCE_WEIGHT_RATIO_CAP, std::sqrt(ddx * ddx + ddy * ddy) / p.radius);
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
                const UnsignedInteger32 li = boundary[pickBoundary(flipRng)];
                x = SignedInteger32(li % W); y = SignedInteger32(li / W);
                own = label[li];
                foreignSides = CountForeignSides(x, y);
                if (unit(flipRng) * maxWeight < FlipWeight(x, y, own, foreignSides)) break;
            }

            // Pick which neighbouring province it flips to - a random foreign side, so a province
            // touching two sides of the pixel is twice as likely as one touching one side. Only sides
            // touching another province's land count: a province can't reach across a barrier pixel.
            SignedInteger32 candidates[4], candidateCount = 0;
            if (IsInterior(x, y, 1)) {
                const SignedInteger64 li = SignedInteger64(LocalIndex(x, y));
                for (const SignedInteger64 step : sideSteps) {
                    const SizeT ni = SizeT(li + step);
                    const SignedInteger32 n = label[ni];
                    if (n >= 0 && n != own && kindGrid[ni] == LAND) candidates[candidateCount++] = n;
                }
            }
            else {
                for (SignedInteger32 d = 0; d < 4; ++d) {
                    const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
                    if (n >= 0 && n != own && IsLand(x + dx4[d], y + dy4[d])) candidates[candidateCount++] = n;
                }
            }
            if (candidateCount == 0) continue;
            const SignedInteger32 target = candidates[std::uniform_int_distribution<SignedInteger32>(0, candidateCount - 1)(flipRng)];

            // Provinces may not vanish, split or cross a barrier
            if (!CanRemove(x, y, own)) continue;

            // ---- The two changed provinces' density and terrain mix as they'd be after the flip ----
            const SizeT flipIndex = LocalIndex(x, y);
            const Float64 weight = weightGrid[flipIndex];
            const auto terrain = terrainGrid[flipIndex];
            Province& ownProvince = provinces[own];
            Province& targetProvince = provinces[target];

            // ---- Score old vs new map ----
            const Float64 newSizeErrorSum = sizeErrorSum
                + SizeError(ownProvince.value - weight, targetSize[own]) + SizeError(targetProvince.value + weight, targetSize[target])
                - SizeError(ownProvince.value, targetSize[own]) - SizeError(targetProvince.value, targetSize[target]);
            const Float64 newTerrainMixSum = terrainMixSum
                + ownProvince.TerrainMixAfterRemoving(terrain) + targetProvince.TerrainMixAfterAdding(terrain)
                - ownProvince.TerrainMix() - targetProvince.TerrainMix();
            // Flipping p from own to target: every pair (p, q) and (q, p) with q in own becomes a
            // border pair, and every pair with q in target stops being one
            Float64 newNeighbourhoodEnergy = neighbourhoodEnergy;
            if (useNeighbourhood) {
                Float64 costOwn, costTarget;
                CostInDisc(x, y, own, target, costOwn, costTarget);
                newNeighbourhoodEnergy += 2.0 * (costOwn - costTarget);
            }

            const Float64 oldScore = ScoreMap(sizeErrorSum, neighbourhoodEnergy * neighbourhoodScale, terrainMixSum, provincesCount, settings);
            const Float64 newScore = ScoreMap(newSizeErrorSum, newNeighbourhoodEnergy * neighbourhoodScale, newTerrainMixSum, provincesCount, settings);

            if (newScore > oldScore) {
                const Float64 temperature = (temperatureStart == temperatureEnd) ? temperatureStart
                    : temperatureStart * std::pow(temperatureEnd / temperatureStart, Float64(it) / Float64(iterations));
                if (temperature <= 0.0 || unit(flipRng) >= std::exp(-(newScore - oldScore) / (temperature * borderPixelScore))) continue;
            }

            // ---- Apply the flip ----
            label[flipIndex] = target;
            ownProvince.Remove(x, y, weight, terrain);
            targetProvince.Add(x, y, weight, terrain);
            ownProvince.UpdateCache();
            targetProvince.UpdateCache();
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
        for (const SizeT li : pixelIndex) {
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
        for (const SizeT li : pixelIndex) {
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
        for (const SizeT li : pixelIndex) {
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
        for (const SizeT li : pixelIndex) {
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
        for (const SizeT li : pixelIndex)
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
        for (const UnsignedInteger32 li : pixelIndex) {
            const SignedInteger32 own = label[li];
            if (own < 0 || kindGrid[li] != LAND) continue;
            const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
            Boolean isCore = true;
            if (IsInterior(x, y, THIN_RADIUS)) {
                for (const SignedInteger64 step : thinSteps) {
                    const SizeT ni = SizeT(SignedInteger64(li) + step);
                    const SignedInteger32 n = label[ni];
                    if (n >= 0 && n != own && kindGrid[ni] == LAND) { isCore = false; break; }
                }
            }
            else {
                for (const auto& [ox, oy] : thinOffsets) {
                    const SignedInteger32 n = LabelAt(x + ox, y + oy);
                    if (n >= 0 && n != own && IsLand(x + ox, y + oy)) { isCore = false; break; }
                }
            }
            if (isCore) { corePixels.push_back(li); hasCore[own] = 1; }
        }
    }

    // Split province `donor` in two, giving one half to the (empty) province `freed`: grow both halves at
    // once over the donor's land from two far-apart pixels, so each half is one compact piece
    void SplitProvince(SignedInteger32 donor, SignedInteger32 freed) {
        Vector<UnsignedInteger32> land;
        Float64 cx = 0.0, cy = 0.0;
        for (const SizeT li : pixelIndex)
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
        for (const SizeT li : pixelIndex)
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
            if (IsInterior(x, y, THIN_RADIUS)) {
                for (const SignedInteger64 step : thinSteps) {
                    const SizeT ni = SizeT(SignedInteger64(li) + step);
                    if (label[ni] == own) kept[ni] = 1;
                }
                continue;
            }
            for (const auto& [ox, oy] : thinOffsets)
                if (LabelAt(x + ox, y + oy) == own) kept[LocalIndex(x + ox, y + oy)] = 1;
        }
        // Barrier pixels are judged by the land they touch (see markAllButLargestPiece)
        for (const SizeT li : pixelIndex)
            if (label[li] >= 0 && kindGrid[li] == BARRIER) kept[li] = 1;
        // Provinces too small or thin to have any core are left as they are
        for (const SizeT li : pixelIndex)
            if (label[li] >= 0 && !hasCore[label[li]]) kept[li] = 1;

        MarkAllButLargestPiece(kept, remove);
        return ReassignPixels(remove);
    }

    // Give away every piece of a province that isn't its largest 4-connected piece
    SizeT RemoveDisconnectedPieces() {
        Vector<UnsignedInteger8> include(G, 0), remove(G, 0);
        for (const SizeT li : pixelIndex) include[li] = label[li] >= 0;
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
        const Float64 smoothness = settings.smoothness;
        if (smoothness <= 0.0) return;
        constexpr SizeT smoothingPasses = 3;
        Vector<std::pair<SignedInteger32, SignedInteger32>> disc;
        const SignedInteger32 r = SignedInteger32(std::ceil(smoothness));
        for (SignedInteger32 oy = -r; oy <= r; ++oy)
            for (SignedInteger32 ox = -r; ox <= r; ++ox)
                if (Float64(ox * ox + oy * oy) <= smoothness * smoothness + 1e-9) disc.emplace_back(ox, oy);

        Vector<UnsignedInteger32> order;
        for (const SizeT li : pixelIndex) if (label[li] >= 0) order.push_back(UnsignedInteger32(li));
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
                if (settings.terrainWeight > 0.0 && terrain != NO_TERRAIN &&
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
        for (const SizeT li : pixelIndex) {
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
        for (const SizeT start : pixelIndex) {
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
        // The state's own inner barrier pixels, in grid order. (Cells outside the state also hold
        // INNER_BARRIER in kindGrid, as its default, so the grid itself can't be used to find them.)
        Vector<SizeT> waiting, stillWaiting;
        for (SizeT i = 0; i < totalPixels; ++i)
            if (pixelTypes[i] == INNER_BARRIER)
                waiting.push_back(LocalIndex(state.pixels[i].x - state.x0, state.pixels[i].y - state.y0));

        Vector<std::pair<SizeT, SignedInteger32>> fills;
        for (SizeT pass = 0; pass < 64 && !waiting.empty(); ++pass) {
            fills.clear();
            stillWaiting.clear();
            for (const SizeT li : waiting) {
                const SignedInteger32 x = SignedInteger32(li % W), y = SignedInteger32(li / W);
                SignedInteger32 best = -1, bestCount = 0;
                for (SignedInteger32 d = 0; d < 4; ++d) {
                    const SignedInteger32 n = LabelAt(x + dx4[d], y + dy4[d]);
                    if (n < 0) continue;
                    SignedInteger32 count = 0;
                    for (SignedInteger32 e = 0; e < 4; ++e) if (LabelAt(x + dx4[e], y + dy4[e]) == n) ++count;
                    if (count > bestCount) { best = n; bestCount = count; }
                }
                if (best >= 0) fills.emplace_back(li, best); else stillWaiting.push_back(li);
            }
            // Applied after the scan, so each pass only grows from pixels filled in earlier passes
            for (const auto& [li, owner] : fills) label[li] = owner;
            if (fills.empty()) break;
            waiting.swap(stillWaiting);
        }
    }
};

// Balance one state's province sizes and shapes by flipping border pixels between neighbouring provinces
void CleanProvinces(
    const State& state,
    const Vector<PixelType>& pixelTypes,
    Vector<UnsignedInteger16>& pixelProvinceIds,
    const UnsignedInteger16 provincesCount,
    std::mt19937& rng
) {
    // Set up first even for a single-province state: it draws from rng, and keeping those draws the
    // same keeps every later state's result the same
    ProvinceBalancer balancer(state, pixelTypes, pixelProvinceIds, provincesCount, rng);
    if (provincesCount < 2) return;
    balancer.Run();
    balancer.WriteBack(pixelProvinceIds);
}

// In game, four provinces can't meet at a point: no 2x2 block of pixels may hold four different provinces.
// Fix each such junction by moving one of its pixels into the province of a pixel beside it (in the same
// state/strategic region), picking the move that removes the most junctions without splitting a province,
// and touching preset provinces only when nothing else works. Junctions where no two pixels side by side
// share a state/strategic region can't be fixed without crossing its border - those are returned.
Vector<Pixel> FixFourWayJunctions(
    const Vector<State>& states,
    Vector<Vector<UnsignedInteger16>>& provinceIds,
    const Vector<UnsignedInteger16>& provinceCounts,
    const Vector<SizeT>& generatedPixelCounts,
    const SignedInteger32 W, const SignedInteger32 H
) {
    constexpr UnsignedInteger32 NONE = UINT32_MAX;
    const SizeT N = SizeT(W) * H;
    // Every pixel's province (numbered across the whole map), state, and whether it's in a preset province
    Vector<UnsignedInteger32> province(N, NONE), offset(states.size(), 0);
    Vector<UnsignedInteger16> stateOf(N, 0);
    Vector<Boolean> preset(N, false);
    UnsignedInteger32 total = 0;
    for (SizeT s = 0; s < states.size(); ++s) { offset[s] = total; total += provinceCounts[s]; }
    Vector<UnsignedInteger32> provinceSize(total, 0);
    for (SizeT s = 0; s < states.size(); ++s) {
        for (SizeT i = 0; i < states[s].pixels.size(); ++i) {
            const SizeT pixel = SizeT(states[s].pixels[i].y) * W + states[s].pixels[i].x;
            province[pixel] = offset[s] + provinceIds[s][i];
            stateOf[pixel] = UnsignedInteger16(s);
            preset[pixel] = i >= generatedPixelCounts[s];
            ++provinceSize[province[pixel]];
        }
    }

    // Is the 2x2 block with top-left corner (x, y) a four-way junction?
    auto isJunction = [&](const SignedInteger32 x, const SignedInteger32 y) {
        if (x < 0 || y < 0 || x + 1 >= W || y + 1 >= H) return false;
        const SizeT i = SizeT(y) * W + x;
        const UnsignedInteger32 a = province[i], b = province[i + 1], c = province[i + W], d = province[i + W + 1];
        return a != b && a != c && a != d && b != c && b != d && c != d;
    };
    // Junctions among the four blocks that contain pixel (x, y)
    auto junctionsAround = [&](const SignedInteger32 x, const SignedInteger32 y) {
        return SizeT(isJunction(x - 1, y - 1)) + isJunction(x, y - 1) + isJunction(x - 1, y) + isJunction(x, y);
    };
    // Can pixel (x, y) leave its province without splitting it? Its province's pixels among the eight around
    // it must form at most one 4-connected group touching it by a side
    auto canLeave = [&](const SignedInteger32 x, const SignedInteger32 y) {
        const UnsignedInteger32 own = province[SizeT(y) * W + x];
        if (provinceSize[own] < 2) return false;
        Boolean in[3][3] = {};
        for (SignedInteger32 dy = -1; dy <= 1; ++dy)
            for (SignedInteger32 dx = -1; dx <= 1; ++dx) {
                const SignedInteger32 nx = x + dx, ny = y + dy;
                in[dy + 1][dx + 1] = (dx || dy) && nx >= 0 && ny >= 0 && nx < W && ny < H && province[SizeT(ny) * W + nx] == own;
            }
        Boolean seen[3][3] = {};
        SizeT groups = 0;
        static const SignedInteger32 sides[4][2] = { { 0, 1 }, { 1, 0 }, { 1, 2 }, { 2, 1 } }; // [row][col]
        for (const auto& side : sides) {
            if (!in[side[0]][side[1]] || seen[side[0]][side[1]]) continue;
            ++groups;
            SignedInteger32 stack[9][2]; SizeT top = 0;
            stack[top][0] = side[0]; stack[top][1] = side[1]; ++top;
            seen[side[0]][side[1]] = true;
            while (top) {
                --top;
                const SignedInteger32 r = stack[top][0], c = stack[top][1];
                static const SignedInteger32 d4[4][2] = { { 0, 1 }, { 1, 0 }, { 0, -1 }, { -1, 0 } };
                for (const auto& d : d4) {
                    const SignedInteger32 nr = r + d[0], nc = c + d[1];
                    if (nr < 0 || nc < 0 || nr > 2 || nc > 2 || !in[nr][nc] || seen[nr][nc]) continue;
                    seen[nr][nc] = true; stack[top][0] = nr; stack[top][1] = nc; ++top;
                }
            }
        }
        return groups <= 1;
    };

    Vector<std::pair<SignedInteger32, SignedInteger32>> work;
    for (SignedInteger32 y = 0; y + 1 < H; ++y)
        for (SignedInteger32 x = 0; x + 1 < W; ++x)
            if (isJunction(x, y)) work.emplace_back(x, y);

    // Pairs of pixels side by side within a 2x2 block, as offsets from its top-left corner
    static const SignedInteger32 pairs[4][4] = { { 0, 0, 1, 0 }, { 0, 1, 1, 1 }, { 0, 0, 0, 1 }, { 1, 0, 1, 1 } };
    Vector<Pixel> unfixable;
    while (!work.empty()) {
        const auto [bx, by] = work.back(); work.pop_back();
        if (!isJunction(bx, by)) continue;

        // Try every move of a pixel into the province beside it; each move must strictly reduce the number
        // of junctions around it, so this always finishes
        SignedInteger32 bestX = -1, bestY = -1; UnsignedInteger32 bestProvince = NONE;
        SizeT bestScore = SIZE_MAX;
        for (const auto& pr : pairs) {
            for (SignedInteger32 flip = 0; flip < 2; ++flip) {
                const SignedInteger32 qx = bx + pr[flip ? 2 : 0], qy = by + pr[flip ? 3 : 1];
                const SignedInteger32 nx = bx + pr[flip ? 0 : 2], ny = by + pr[flip ? 1 : 3];
                const SizeT q = SizeT(qy) * W + qx, n = SizeT(ny) * W + nx;
                if (stateOf[q] != stateOf[n] || !canLeave(qx, qy)) continue;
                const SizeT before = junctionsAround(qx, qy);
                const UnsignedInteger32 own = province[q];
                province[q] = province[n];
                const SizeT after = junctionsAround(qx, qy);
                province[q] = own;
                if (after >= before) continue;
                const SizeT score = (preset[q] ? 16 : 0) + after;
                if (score < bestScore) { bestScore = score; bestX = qx; bestY = qy; bestProvince = province[n]; }
            }
        }
        if (bestX < 0) { unfixable.emplace_back(UnsignedInteger16(bx), UnsignedInteger16(by)); continue; }

        const SizeT q = SizeT(bestY) * W + bestX;
        --provinceSize[province[q]];
        ++provinceSize[bestProvince];
        province[q] = bestProvince;
        for (SignedInteger32 dy = -1; dy <= 0; ++dy)
            for (SignedInteger32 dx = -1; dx <= 0; ++dx)
                if (isJunction(bestX + dx, bestY + dy)) work.emplace_back(bestX + dx, bestY + dy);
    }

    for (SizeT s = 0; s < states.size(); ++s)
        for (SizeT i = 0; i < states[s].pixels.size(); ++i)
            provinceIds[s][i] = UnsignedInteger16(province[SizeT(states[s].pixels[i].y) * W + states[s].pixels[i].x] - offset[s]);
    return unfixable;
}

// Write the game files that go with provinces.bmp, all with fresh IDs:
// - out/definition.csv: every province's ID, colour, type, coastal, terrain and continent
// - out/states/<id>-State_<id>.txt: one per state, with its land and lake provinces (sea isn't in states)
// - out/strategicregions/<id>-StrategicRegion_<id>.txt: one per strategic region that has provinces
// Provinces are numbered from 1 in the order they first appear reading the map row by row; states and
// regions in the order of their first province. Old .txt files in the two folders are removed first.
void WriteGameFiles(
    const Vector<State>& states,
    const Vector<Vector<UnsignedInteger16>>& provinceIds,
    const Vector<Vector<ColourRGB>>& provinceColours,
    const SignedInteger32 W, const SignedInteger32 H
) {
    namespace fs = std::filesystem;
    static const char* TERRAIN_NAMES[TERRAIN_TYPE_COUNT] = { "plains", "forest", "hills", "desert", "mountain", "marsh", "urban", "ocean", "jungle" };
    static const char* TYPE_NAMES[PROVINCE_TYPE_COUNT] = { "land", "sea", "lake" };

    // ---- Number the provinces ----
    struct ProvinceInfo { SizeT state; UnsignedInteger16 local; SizeT firstPixel; };
    Vector<ProvinceInfo> provinces;
    Vector<SizeT> offset(states.size(), 0);
    for (SizeT s = 0; s < states.size(); ++s) {
        offset[s] = provinces.size();
        for (SizeT p = 0; p < provinceColours[s].size(); ++p) provinces.push_back({ s, UnsignedInteger16(p), SIZE_MAX });
        for (SizeT i = 0; i < states[s].pixels.size(); ++i) {
            SizeT& first = provinces[offset[s] + provinceIds[s][i]].firstPixel;
            first = std::min(first, SizeT(states[s].pixels[i].y) * W + states[s].pixels[i].x);
        }
    }
    Vector<SizeT> order(provinces.size());
    for (SizeT i = 0; i < order.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&](SizeT a, SizeT b) { return provinces[a].firstPixel < provinces[b].firstPixel; });
    Vector<UnsignedInteger32> gameId(provinces.size()); // index in provinces -> ID in game (from 1)
    for (SizeT k = 0; k < order.size(); ++k) gameId[order[k]] = UnsignedInteger32(k + 1);

    // ---- Terrain (most common terrain.png type) and coastal (land beside sea, or sea beside land) ----
    Vector<Array<UnsignedInteger32, TERRAIN_TYPE_COUNT>> terrainCount(provinces.size());
    Vector<UnsignedInteger32> provinceAt(SizeT(W) * H, UINT32_MAX);
    for (SizeT s = 0; s < states.size(); ++s) {
        for (SizeT i = 0; i < states[s].pixels.size(); ++i) {
            const SizeT index = offset[s] + provinceIds[s][i];
            provinceAt[SizeT(states[s].pixels[i].y) * W + states[s].pixels[i].x] = UnsignedInteger32(index);
            if (i < states[s].terrains.size() && states[s].terrains[i] < TERRAIN_TYPE_COUNT) ++terrainCount[index][states[s].terrains[i]];
        }
    }
    Vector<Boolean> coastal(provinces.size(), false);
    auto checkPair = [&](UnsignedInteger32 a, UnsignedInteger32 b) {
        if (a == b || a == UINT32_MAX || b == UINT32_MAX) return;
        const ProvinceType ta = states[provinces[a].state].type, tb = states[provinces[b].state].type;
        if ((ta == LAND_PROVINCE && tb == SEA_PROVINCE) || (ta == SEA_PROVINCE && tb == LAND_PROVINCE)) coastal[a] = coastal[b] = true;
    };
    for (SignedInteger32 y = 0; y < H; ++y) {
        for (SignedInteger32 x = 0; x < W; ++x) {
            const SizeT i = SizeT(y) * W + x;
            if (x + 1 < W) checkPair(provinceAt[i], provinceAt[i + 1]);
            if (y + 1 < H) checkPair(provinceAt[i], provinceAt[i + W]);
        }
    }
    Vector<UnsignedInteger32>().swap(provinceAt);

    // ---- definition.csv ----
    fs::create_directories("out");
    {
        std::ofstream file("out/definition.csv");
        if (!file) FatalError("ERROR: couldn't write out/definition.csv");
        file << "0;0;0;0;land;false;unknown;0\n";
        for (const SizeT index : order) {
            const ProvinceInfo& info = provinces[index];
            const ProvinceType type = states[info.state].type;
            const ColourRGB& c = provinceColours[info.state][info.local];
            const char* terrain = type == SEA_PROVINCE ? "ocean" : type == LAKE_PROVINCE ? "lakes" : "plains";
            if (type == LAND_PROVINCE) {
                UnsignedInteger32 best = 0;
                // Ocean isn't a valid terrain for land, so ignore any ocean-coloured terrain.png pixels on it
                for (SizeT t = 0; t < TERRAIN_TYPE_COUNT; ++t)
                    if (t != OCEAN && terrainCount[index][t] > best) { best = terrainCount[index][t]; terrain = TERRAIN_NAMES[t]; }
            }
            file << gameId[index] << ';' << SignedInteger32(c.r) << ';' << SignedInteger32(c.g) << ';' << SignedInteger32(c.b) << ';'
                 << TYPE_NAMES[type] << ';' << (coastal[index] ? "true" : "false") << ';' << terrain << ';'
                 << (type == SEA_PROVINCE ? 0 : CONTINENT) << '\n';
        }
    }

    // Empty a folder of old .txt files (IDs change between runs, so old files would otherwise linger)
    auto prepareFolder = [](const fs::path& folder) {
        fs::create_directories(folder);
        for (const auto& entry : fs::directory_iterator(folder))
            if (entry.is_regular_file() && entry.path().extension() == ".txt") fs::remove(entry.path());
    };
    auto provinceList = [](Vector<UnsignedInteger32> ids) {
        std::sort(ids.begin(), ids.end());
        String text;
        for (const UnsignedInteger32 id : ids) text += std::to_string(id) + ' ';
        return text;
    };

    // A state's land and lake provinces are made separately (as two States with the same statemap colour),
    // but are one state in game. Lakes go in the same strategic region as their state's land
    HashMap<UnsignedInteger32, SizeT> landStateOf; // statemap colour -> land State
    for (SizeT s = 0; s < states.size(); ++s)
        if (states[s].type == LAND_PROVINCE && !provinceColours[s].empty()) landStateOf[states[s].colour.ToInteger()] = s;
    auto regionOf = [&](SizeT s) {
        if (states[s].type == LAKE_PROVINCE) {
            const auto land = landStateOf.find(states[s].colour.ToInteger());
            if (land != landStateOf.end()) return states[land->second].regionColour;
        }
        return states[s].regionColour;
    };

    // ---- States: one per statemap colour (its land and lake provinces), numbered by their first province ----
    {
        HashMap<UnsignedInteger32, SizeT> stateIndex; // statemap colour -> index into ids
        Vector<Vector<UnsignedInteger32>> ids;
        for (SizeT s = 0; s < states.size(); ++s) {
            if (states[s].type == SEA_PROVINCE || provinceColours[s].empty()) continue;
            const auto [it, added] = stateIndex.try_emplace(states[s].colour.ToInteger(), ids.size());
            if (added) ids.emplace_back();
            for (SizeT p = 0; p < provinceColours[s].size(); ++p) ids[it->second].push_back(gameId[offset[s] + p]);
        }
        Vector<std::pair<UnsignedInteger32, SizeT>> landStates; // (lowest province ID, index into ids)
        for (SizeT k = 0; k < ids.size(); ++k) landStates.emplace_back(*std::min_element(ids[k].begin(), ids[k].end()), k);
        std::sort(landStates.begin(), landStates.end());
        prepareFolder("out/states");
        for (SizeT k = 0; k < landStates.size(); ++k) {
            const SizeT id = k + 1;
            std::ofstream file("out/states/" + std::to_string(id) + "-State_" + std::to_string(id) + ".txt");
            if (!file) FatalError("ERROR: couldn't write to out/states");
            file << "state={\n"
                 << "\tid=" << id << "\n"
                 << "\tname=\"STATE_" << id << "\"\n"
                 << "\tmanpower=" << STATE_MANPOWER << "\n"
                 << "\tstate_category=" << STATE_CATEGORY << "\n"
                 << "\thistory={\n\t}\n"
                 << "\tprovinces={\n\t\t" << provinceList(ids[landStates[k].second]) << "\n\t}\n"
                 << "}\n";
        }
    }

    // ---- Strategic regions: every province goes in its state's region ----
    {
        HashMap<UnsignedInteger32, SizeT> regionIndex;
        Vector<Vector<UnsignedInteger32>> regionProvinces;
        for (SizeT s = 0; s < states.size(); ++s) {
            if (provinceColours[s].empty()) continue;
            const auto [it, added] = regionIndex.try_emplace(regionOf(s), regionProvinces.size());
            if (added) regionProvinces.emplace_back();
            for (SizeT p = 0; p < provinceColours[s].size(); ++p) regionProvinces[it->second].push_back(gameId[offset[s] + p]);
        }
        Vector<std::pair<UnsignedInteger32, SizeT>> regionOrder;
        for (SizeT r = 0; r < regionProvinces.size(); ++r)
            regionOrder.emplace_back(*std::min_element(regionProvinces[r].begin(), regionProvinces[r].end()), r);
        std::sort(regionOrder.begin(), regionOrder.end());
        prepareFolder("out/strategicregions");
        for (SizeT k = 0; k < regionOrder.size(); ++k) {
            const SizeT id = k + 1;
            std::ofstream file("out/strategicregions/" + std::to_string(id) + "-StrategicRegion_" + std::to_string(id) + ".txt");
            if (!file) FatalError("ERROR: couldn't write to out/strategicregions");
            file << "strategic_region={\n"
                 << "\tid=" << id << "\n"
                 << "\tname=\"STRATEGICREGION_" << id << "\"\n"
                 << "\tprovinces={\n\t\t" << provinceList(regionProvinces[regionOrder[k].second]) << "\n\t}\n"
                 // Placeholder weather: mild and dry all year - tune per region in game files
                 << "\tweather={\n"
                 << "\t\tperiod={\n"
                 << "\t\t\tbetween={ 0.0 30.11 }\n"
                 << "\t\t\ttemperature={ 5.0 20.0 }\n"
                 << "\t\t\tno_phenomenon=1.000\n"
                 << "\t\t\train_light=0.000\n"
                 << "\t\t\train_heavy=0.000\n"
                 << "\t\t\tsnow=0.000\n"
                 << "\t\t\tblizzard=0.000\n"
                 << "\t\t\tarctic_water=0.000\n"
                 << "\t\t\tmud=0.000\n"
                 << "\t\t\tsandstorm=0.000\n"
                 << "\t\t\tmin_snow_level=0.000\n"
                 << "\t\t}\n"
                 << "\t}\n"
                 << "}\n";
        }
    }
}

// Everything for one state, start to finish: seeds, starting layout and balancing. Returns each pixel's
// province (0 to provincesCount - 1, in state.pixels order); colouring happens afterwards, in main().
Vector<UnsignedInteger16> ProcessState(const State& state, std::mt19937& rng, UnsignedInteger16& provincesCount) {
    // Work out which pixels are barriers (rivers), then pick random points in each state - per
    // separate land area, more of them where it's denser - to be our province centres
    const Vector<PixelType> pixelTypes = ClassifyPixels(state);
    Vector<Pixel> randomPixels = SelectSeeds(state, pixelTypes, rng);
    provincesCount = randomPixels.size();

    // Given our semi-random points on the map, assign each pixel in the state to it's nearest point
    Vector<UnsignedInteger16> pixelProvinceIds = AssignProvinces(state, pixelTypes, randomPixels);

    // Balance province sizes and shapes by flipping border pixels between neighbouring provinces
    CleanProvinces(state, pixelTypes, pixelProvinceIds, provincesCount, rng);
    return pixelProvinceIds;
}

int main() {
    Timestamp startTime = std::chrono::steady_clock::now();
    LoadSettings("settings.txt");

    SignedInteger32 mapWidth{}, mapHeight{};
    Vector<State> statesVector = LoadStates(mapWidth, mapHeight);

    const SizeT provincesMapDataSize = mapWidth * mapHeight * 3;
    UnsignedInteger8 *provincesMapData = new UnsignedInteger8[provincesMapDataSize];
    std::fill_n(provincesMapData, provincesMapDataSize, 20);

    std::mt19937 rng(std::random_device{}());

    // One seed per state, drawn up front, so the map doesn't depend on which thread runs which state
    Vector<UnsignedInteger32> stateSeeds(statesVector.size());
    for (auto& seed : stateSeeds) seed = UnsignedInteger32(rng());
    std::mt19937 colourRng{ UnsignedInteger32(rng()) };

    // Order the states based on size and divide them into threadCount vectors
    const SizeT threadCount = (THREAD_COUNT == 0) ?
        SizeT(std::llround(Float64(std::thread::hardware_concurrency()) * 0.75)):
        std::min<SizeT>(THREAD_COUNT, std::thread::hardware_concurrency());
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

    // Compute the states concurrently - each thread works through its own list of states
    Vector<Vector<UnsignedInteger16>> stateProvinceIds(statesVector.size());
    Vector<UnsignedInteger16> stateProvinceCounts(statesVector.size(), 0);
    Vector<std::thread> threads;
    for (SizeT t = 0; t < threadCount; ++t) {
        threads.emplace_back([&, t]() {
            for (const SizeT s : stateGroups[t]) {
                if (statesVector[s].pixels.empty()) continue; // made entirely of preset provinces
                std::mt19937 stateRng(stateSeeds[s]);
                stateProvinceIds[s] = ProcessState(statesVector[s], stateRng, stateProvinceCounts[s]);
            }
        });
    }
    for (auto& thread : threads) thread.join();

    // Add each state's preset provinces to its pixels, after the generated ones
    Vector<SizeT> generatedPixelCounts(statesVector.size());
    for (SizeT s = 0; s < statesVector.size(); ++s) {
        generatedPixelCounts[s] = statesVector[s].pixels.size();
        statesVector[s].MergePresetProvinces(stateProvinceIds[s], stateProvinceCounts[s]);
    }

    // No four provinces may meet at one point
    const Vector<Pixel> unfixableJunctions = FixFourWayJunctions(statesVector, stateProvinceIds, stateProvinceCounts, generatedPixelCounts, mapWidth, mapHeight);
    if (!unfixableJunctions.empty()) {
        std::cerr << "WARNING: " << unfixableJunctions.size() << " point(s) where four provinces meet couldn't be fixed, as the "
                     "four pixels are in different states/strategic regions (fix in statemap.png / strategicregionmap.png):";
        for (SizeT i = 0; i < unfixableJunctions.size() && i < 20; ++i)
            std::cerr << (i ? ", " : " ") << "(" << unfixableJunctions[i].x << ", " << unfixableJunctions[i].y << ")";
        std::cerr << (unfixableJunctions.size() > 20 ? ", ...\n" : "\n");
    }

    SizeT provinceCount = 0;
    for (const auto count : stateProvinceCounts) { provinceCount += count; }

    // Colour the map in
    Vector<Vector<ColourRGB>> stateProvinceColours(statesVector.size());
    {
        Set<UnsignedInteger32> usedColours;
        for (SizeT s = 0; s < statesVector.size(); ++s) {
            const State& state = statesVector[s];
            SignedInteger32 rMin = SEA_RED_MIN,   rMax = SEA_RED_MAX;
            SignedInteger32 gMin = SEA_GREEN_MIN, gMax = SEA_GREEN_MAX;
            SignedInteger32 bMin = SEA_BLUE_MIN,  bMax = SEA_BLUE_MAX;
            if (state.type != SEA_PROVINCE) {
                const auto low  = [](const UnsignedInteger8 v) { return std::max(0,   SignedInteger32(v) - STATE_COLOUR_VARIATION); };
                const auto high = [](const UnsignedInteger8 v) { return std::min(255, SignedInteger32(v) + STATE_COLOUR_VARIATION); };
                rMin = low(state.colour.r); rMax = high(state.colour.r);
                gMin = low(state.colour.g); gMax = high(state.colour.g);
                bMin = low(state.colour.b); bMax = high(state.colour.b);
            }
            std::uniform_int_distribution<SignedInteger32> red(rMin, rMax), green(gMin, gMax), blue(bMin, bMax);

            Vector<ColourRGB> colours(stateProvinceCounts[s]);
            for (auto& colour : colours) {
                UnsignedInteger32 c;
                do {
                    c = (UnsignedInteger32(red(colourRng)) << 16) | (UnsignedInteger32(green(colourRng)) << 8) | UnsignedInteger32(blue(colourRng));
                } while (c == 0x141414 || c == 0x000000 || !usedColours.insert(c).second); // 0x000000 is province 0 in definition.csv
                colour = ColourRGB(UnsignedInteger8(c >> 16), UnsignedInteger8(c >> 8), UnsignedInteger8(c));
            }
            for (SizeT i = 0; i < state.pixels.size(); i++) {
                const ColourRGB& colour = colours[stateProvinceIds[s][i]];
                const SizeT index = ((SizeT(state.pixels[i].y) * mapWidth) + state.pixels[i].x) * 3;
                provincesMapData[index + 0] = colour.r;
                provincesMapData[index + 1] = colour.g;
                provincesMapData[index + 2] = colour.b;
            }
            stateProvinceColours[s] = std::move(colours);
        }
    }

    // 24-bit BMP, as the game wants
    std::filesystem::create_directories("out");
    if (!stbi_write_bmp("out/provinces.bmp", mapWidth, mapHeight, 3, provincesMapData)) FatalError("ERROR: couldn't write out/provinces.bmp");
    delete[] provincesMapData;
    WriteGameFiles(statesVector, stateProvinceIds, stateProvinceColours, mapWidth, mapHeight);

    std::cout << std::format("Processed statemap in {0}.\nGenerated {1} provinces.", GetTimeElapsedFromStart(startTime), provinceCount);
    return 0;
}
