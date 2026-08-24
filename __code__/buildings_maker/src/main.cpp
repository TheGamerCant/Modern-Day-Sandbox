// g++ src/*.cpp -std=c++20 -static -O3 -o buildings_maker.exe
// g++ src/*.cpp -std=c++20 -O3 -o buildings_maker_mac

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "data_types.hpp"
#include "functions.hpp"

#include "json.hpp"
using json = nlohmann::json;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void LoadDataFromJson(const String& jsonPath, Path& vanillaDirectory, Vector<UnsignedInteger16>& statesToRunOn, Boolean& sortBuildingsFile) {
    std::ifstream file(jsonPath);
    json inputsJson = json::parse(file);

	if (!inputsJson.contains("game_directory") || !inputsJson["game_directory"].is_string()) {
	    FatalError("Missing or invalid 'game_directory' in " + jsonPath);
	}

	vanillaDirectory = Path(inputsJson.at("game_directory"));

	if (!std::filesystem::exists(vanillaDirectory) || !std::filesystem::is_directory(vanillaDirectory)) {
	    FatalError("The given game directory " + vanillaDirectory.string() + " does not exist");
	}

	if (inputsJson.contains("states") && inputsJson["states"].is_array()) {
	    statesToRunOn = inputsJson["states"].get<Vector<UnsignedInteger16>>();
	}

	if (inputsJson.contains("sort_buildings_file") && inputsJson["sort_buildings_file"].is_boolean()) {
	    sortBuildingsFile = inputsJson["sort_buildings_file"].get<Boolean>();
	}
}

Set<String> LoadReplacePathsFromModFile(const Path& modDirectory) {
    UnsignedInteger32 modFileCount = 0;
    Path modFilePath;

    for (const auto& entry : std::filesystem::directory_iterator(modDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mod") {
            ++modFileCount;
            modFilePath = entry.path();
        }
    }

    if (modFileCount == 0) {
        FatalError("No .mod files found in " + modDirectory.string());
    }
    else if (modFileCount > 1) {
        FatalError("More than one .mod files found in " + modDirectory.string());
    }

    Set<String> modReplaceDirectories;
    PdxJson modFile = ParseFileToPdxJson(modFilePath.string());

    if (!modFile.contains("replace_path") || !modFile["replace_path"].isList()) { return modReplaceDirectories; }

    for (const auto& replace_path : modFile.at("replace_path").asList()) {
        if (!replace_path.isString()) { continue; }

        modReplaceDirectories.insert(RemoveQuotes(replace_path.as<String>()));
    }

    return modReplaceDirectories;
}

struct SpawnPoint {
public:
    SignedInteger32 count;
    Boolean provincial;
    Boolean onlyCoastal;
    String name;

    SpawnPoint() : count(0), provincial(false), onlyCoastal(false), name() {}
    SpawnPoint(const SignedInteger32 count, const Boolean provincial, const Boolean onlyCoastal, const String& name)
        : count(count), provincial(provincial), onlyCoastal(onlyCoastal), name(name) {}
};

void LoadBuildings(
    const Vector<Path>& buildingFiles,
    Vector<SpawnPoint>& spawnPointsVector,
    Set<String>& autoGenerationDisabledSet
) {
    // Used to validate 'spawn_point = ' args in buildings
    Set<String> spawnPointNamesSet;

    for (const auto& filePath : buildingFiles) {
        PdxJson buildingFile = ParseFileToPdxJson(filePath.string());

        // Spawn points come first
        if (buildingFile.contains("spawn_points")) {
            for (const auto& spawnPointsDict: buildingFile.at("spawn_points").asList()) {
                for (const auto& [spawnPointName, spawnPointData] : spawnPointsDict.asDict()) {
                    String name = std::get<String>(spawnPointName);

                    const PdxJson& data = spawnPointData[0];
                    SignedInteger32 count = 1;
                    Boolean provincial = false;
                    Boolean onlyCoastal = false;

                    if (data.contains("disable_auto_nudging") && data["disable_auto_nudging"][0].isBool() && data["disable_auto_nudging"][0].as<Boolean>()) {
                        autoGenerationDisabledSet.insert(name);
                        spawnPointNamesSet.insert(name);
                        continue;
                    }

                    if (!data.contains("max") || !data["max"][0].isInt()) {
                        std::cout << std::format("Spawn point {0} doesn't contain a 'max' value, skipping entry\n", name);
                        continue;
                    }
                    count = data["max"][0].as<SignedInteger32>();

                    if (!data.contains("type") || !data["type"][0].isString()) {
                        std::cout << std::format("Spawn point {0} doesn't contain a 'type' value, skipping entry\n", name);
                        continue;
                    }
                    String type = data["type"][0].as<String>();
                    if (type == "province") {
                        provincial = true;
                    }
                    else if (type != "state") {
                        std::cout << std::format("Spawn point {0}'s 'type' value must be either 'state' or 'province' got {1}, skipping entry\n", name, type);
                        continue;
                    }

                    if (data.contains("only_costal") && data["only_costal"][0].isBool()) {
                        onlyCoastal = data["only_costal"][0].as<Boolean>();
                    }

                    spawnPointsVector.emplace_back(count, provincial, onlyCoastal, name);
                    spawnPointNamesSet.insert(name);
                }
            }
        }

        // Now look at buildings
        if (buildingFile.contains("buildings")) {
            for (const auto& buildingsDict: buildingFile.at("buildings").asList()) {
                for (const auto& [buildingName, buildingData] : buildingsDict.asDict()) {
                    String name = std::get<String>(buildingName);

                    const PdxJson& data = buildingData[0];

                    // If this building uses a spawn point ignore it
                    if (data.contains("spawn_point") && data["spawn_point"][0].isString() && spawnPointNamesSet.contains(data["spawn_point"][0].as<String>())) {
                        continue;
                    }

                    SignedInteger32 count = 1;
                    Boolean provincial = false;
                    Boolean onlyCoastal = false;

                    // If show_on_map does not exist just ignore it
                    if (!data.contains("show_on_map") || !data["show_on_map"][0].isInt()) {
                        continue;
                    }
                    count = data["show_on_map"][0].as<SignedInteger32>();

                    if (data.contains("only_costal") && data["only_costal"][0].isBool()) {
                        onlyCoastal = data["only_costal"][0].as<Boolean>();
                    }

                    if (data.contains("level_cap") && data.at("level_cap")[0].isDict() &&
                        data["level_cap"][0].contains("province_max") && data["level_cap"][0]["province_max"][0].isInt() &&
                        data["level_cap"][0]["province_max"][0].as<SignedInteger64>() > 0) {
                        provincial = true;
                    }

                    spawnPointsVector.emplace_back(count, provincial, onlyCoastal, name);
                    spawnPointNamesSet.insert(name);
                }
            }
        }
    }
}

enum TypeEnum : UnsignedInteger8 { Land, Lake, Sea };

struct Province {
    ColourRGB colour;
    TypeEnum type;

    // Worked out by looking at TypeEnum / provinces.bmp -> not definition.csv
    Boolean isCoastal = false;
    Set<UnsignedInteger16> coastalNeighbours{};

    // Pixel indexes belonging to this province
    Vector<UnsignedInteger32> pixelIndexes{};

    // Find the interior pixel of the province instead of just the width/height
    UnsignedInteger32 interiorPixel = 0;
    UnsignedInteger16 interiorPixelDepth = 0;

    UnsignedInteger16 stateId = 0;

    Province() : colour(), type(Land) { pixelIndexes.reserve(512); }
    Province(const ColourRGB colour, const TypeEnum type) : colour(colour), type(type) { pixelIndexes.reserve(512); }
};

struct State {
    UnsignedInteger16 id;
    Boolean isCoastal = false;
    Vector<UnsignedInteger16> provinces;

    State() : id(0), provinces() {}
    State(const UnsignedInteger16 id, const Vector<UnsignedInteger16>& provinces) : id(id), provinces(provinces) {}
};

Vector<Province> LoadProvinces(const Path& definitionsCsvPath, const HashMap<String, TypeEnum>& typeMap) {
	String provinceDefinitions = RemoveStringWhitespace(LoadFileToString(definitionsCsvPath.string()));

	// Initialise vector and reserve space
	Vector<Province> provincesVector;
	UnsignedInteger64 newLinesCount = 1;
    for (const auto& c : provinceDefinitions) if (c == ' ') { ++newLinesCount; }
    provincesVector.reserve(newLinesCount + 1);

	// There shouldn't be any line greater than 256 characters - use the stack instead of heap for optimisation purposes
	Char csvEntryArray[256]{};
    UnsignedInteger8 currentStringLength = 0;
    String entry = "";

	UnsignedInteger16 column = 0;
    UnsignedInteger32 currentLine = 0;

	//Store the current province's data in these
	ColourRGB colour;
	TypeEnum type = Land;

	// Go char-by-char
	for (const auto& c : provinceDefinitions) {
		//New line
        switch (c) {
			case ' ':
				if (column == 7) {
					csvEntryArray[currentStringLength++] = 0;
					entry = String(csvEntryArray);
				}
				else if (column < 7) { FatalError("Column count in definition.csv at line " + std::to_string(currentLine) + " is too short"); }

				provincesVector.emplace_back(colour, type);

				currentLine++; column = 0; currentStringLength = 0;

				break;

			case ';':
				csvEntryArray[currentStringLength++] = 0;
				entry = String(csvEntryArray);

				switch (column) {
					// Red
					case 1:
						if (!StringCanBecomeInteger(entry)) { FatalError("Bad colour definition in definition.csv at line " + std::to_string(currentLine)); }
						colour.r = std::stoi(entry);
						break;

					// Green
					case 2:
						if (!StringCanBecomeInteger(entry)) { FatalError("Bad colour definition in definition.csv at line " + std::to_string(currentLine)); }
						colour.g = std::stoi(entry);
						break;

					// Blue
					case 3:
						if (!StringCanBecomeInteger(entry)) { FatalError("Bad colour definition in definition.csv at line " + std::to_string(currentLine)); }
						colour.b = std::stoi(entry);
						break;

                    // Type
					case 4:
						if (!typeMap.contains(entry)) { FatalError("Bad province type definition in definition.csv at line " + std::to_string(currentLine)); }
						type = typeMap.at(entry);
						break;
				}

				column++;
				currentStringLength = 0;

				break;

			default:
				if (currentStringLength > 254) { FatalError("Entry too long in definition.csv at line " + std::to_string(currentLine)); }
				csvEntryArray[currentStringLength++] = c;
		}

	}

	if (column == 7) {
		csvEntryArray[currentStringLength++] = 0;
		entry = String(csvEntryArray);
	}
	else if (column < 7) { FatalError("Column count in definition.csv at line " + std::to_string(currentLine) + " is too short"); }

	provincesVector.emplace_back(colour, type);

	currentLine++; column = 0; currentStringLength = 0;

	return provincesVector;
}

struct PixelMap {
    UnsignedInteger32 width, height, area;
    Vector<UnsignedInteger8> heightmap;
    Vector<UnsignedInteger16> provinceIdMap;

    // How far each pixel is from the nearest pixel of a different province. One means
    // the pixel is on the border. FindProvinceInteriors fills this in; placement needs
    // it to keep models off province edges, which is why it is kept rather than
    // discarded once the interior points have been found.
    Vector<UnsignedInteger16> borderDistance;

    PixelMap(
        const Path& provincesBmpPath,
        const Path& heightmapBmpPath,
        Vector<Province>& provincesVector,
        const HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColourToIndexMap
    ) {
        UnsignedInteger32 provincesBmpWidth = 0;
        UnsignedInteger32 provincesBmpHeight = 0;
        UnsignedInteger32 provincesBmpChannels = 0;
        UnsignedInteger64 provincesBmpDataOffset = 0;
		
		// Load provinces.bmp to a UnsignedInteger8*
		std::ifstream provincesBmpFile(provincesBmpPath.string(), std::ios::binary | std::ios::ate);
		if (!provincesBmpFile) {
			FatalError("Cannot open file " + provincesBmpPath.string());
		}
		SizeT provincesBmpFileSize = provincesBmpFile.tellg();
		provincesBmpFile.seekg(0, std::ios::beg);
		Char* provincesBmpRawData = new Char[provincesBmpFileSize];
		provincesBmpFile.read(provincesBmpRawData, provincesBmpFileSize);
		
		provincesBmpWidth = provincesBmpRawData[21] << 24 | provincesBmpRawData[20] << 16 | provincesBmpRawData[19] << 8 | provincesBmpRawData[18];
		provincesBmpHeight = provincesBmpRawData[25] << 24 | provincesBmpRawData[24] << 16 | provincesBmpRawData[23] << 8 | provincesBmpRawData[22];
		provincesBmpChannels = provincesBmpRawData[29] << 8 | provincesBmpRawData[28];
		provincesBmpDataOffset = provincesBmpRawData[11] << 8 | provincesBmpRawData[10];
		
		if (provincesBmpChannels != 24) {
			delete[] provincesBmpRawData;
            FatalError("provinces.bmp should have 24 bpp, instead has " + std::to_string(provincesBmpChannels));
        }
		
		// Set the map width/height/area
		width = provincesBmpWidth;
        height = provincesBmpHeight;
        area = width * height;
		
		// Assign province IDs based on the map
        UnsignedInteger32 rgbColour = 0;
        provinceIdMap = Vector<UnsignedInteger16>(area, 0);
		for (SizeT yInv = 0; yInv < height; yInv++) {
			SizeT realY = height - 1 - yInv;
			SizeT rowStart = realY * width;
			
			for (SizeT x = 0; x < width; x++) {
				SizeT index = rowStart + x;
				
				// Stored as BGR
				rgbColour =
					(static_cast<UnsignedInteger8>(provincesBmpRawData[provincesBmpDataOffset + 2]) << 16) |
					(static_cast<UnsignedInteger8>(provincesBmpRawData[provincesBmpDataOffset + 1]) << 8) |
					(static_cast<UnsignedInteger8>(provincesBmpRawData[provincesBmpDataOffset + 0]));

				if (!provinceColourToIndexMap.contains(rgbColour)) {
					FatalError(std::format(
						"No province has the colour {0}, {1}, {2} in provinces.bmp ({3}, {4})", 
						(UnsignedInteger32)provincesBmpRawData[provincesBmpDataOffset + 2], 
						(UnsignedInteger32)provincesBmpRawData[provincesBmpDataOffset + 1], 
						(UnsignedInteger32)provincesBmpRawData[provincesBmpDataOffset + 0], 
						index % width, 
						index / width
					));
				}

				// Set both the province ID map and the province's pixel vector
				UnsignedInteger16 provinceId = provinceColourToIndexMap.at(rgbColour);
				provinceIdMap[index] = provinceId;
				provincesVector[provinceId].pixelIndexes.push_back(index);

				provincesBmpDataOffset += 3;
			}
        }
		
		delete[] provincesBmpRawData;


		// Repeat for heightmap.bmp
		
		UnsignedInteger32 heightmapBmpWidth = 0;
        UnsignedInteger32 heightmapBmpHeight = 0;
        UnsignedInteger32 heightmapBmpChannels = 0;
        UnsignedInteger64 heightmapBmpDataOffset = 0;
		
		// Load heightmap.bmp to a UnsignedInteger8*
		std::ifstream heightmapBmpFile(heightmapBmpPath.string(), std::ios::binary | std::ios::ate);
		if (!heightmapBmpFile) {
			FatalError("Cannot open file " + heightmapBmpPath.string());
		}
		SizeT heightmapBmpFileSize = heightmapBmpFile.tellg();
		heightmapBmpFile.seekg(0, std::ios::beg);
		Char* heightmapBmpRawData = new Char[heightmapBmpFileSize];
		heightmapBmpFile.read(heightmapBmpRawData, heightmapBmpFileSize);
		
		heightmapBmpWidth = heightmapBmpRawData[21] << 24 | heightmapBmpRawData[20] << 16 | heightmapBmpRawData[19] << 8 | heightmapBmpRawData[18];
		heightmapBmpHeight = heightmapBmpRawData[25] << 24 | heightmapBmpRawData[24] << 16 | heightmapBmpRawData[23] << 8 | heightmapBmpRawData[22];
		heightmapBmpChannels = heightmapBmpRawData[29] << 8 | heightmapBmpRawData[28];
		heightmapBmpDataOffset = heightmapBmpRawData[11] << 8 | heightmapBmpRawData[10];
		
		if (heightmapBmpChannels != 8) {
			delete[] heightmapBmpRawData;
            FatalError("heightmap.bmp should have 8 bpp, instead has " + std::to_string(provincesBmpChannels));
        }
		
		// Verify w/h are the same
		if (heightmapBmpWidth != width || heightmapBmpHeight != height) {
			delete[] heightmapBmpRawData;
            FatalError("heightmap.bmp does not have the same dimensions as provinces.bmp");
        }
		
		// Assign heightmap values
		heightmap = Vector<UnsignedInteger8> (area);
		for (SizeT y = 0; y < height; y++) {
			SizeT srcRow = height - 1 - y; 
			UnsignedInteger8* srcRowStart = reinterpret_cast<UnsignedInteger8*>(heightmapBmpRawData) + heightmapBmpDataOffset + srcRow * width;
			UnsignedInteger8* dstRowStart = heightmap.data() + y * width;
			std::copy(srcRowStart, srcRowStart + width, dstRowStart);
		}
		
		delete[] heightmapBmpRawData;

    }
};

Boolean CoastalPixelFound(TypeEnum typeOne, TypeEnum type2){
    if (typeOne == Sea && (type2 == Land || type2 == Lake)) {
        return true;
    }
    else if ((typeOne == Land || typeOne == Lake) && type2 == Sea) {
        return true;
    }

    return false;
}

// Given our x and y coordinates, get the neighbour indexes
Array<SignedInteger64, 4> GetNeighbourIndexes(const PixelMap& pixelMap, const SizeT x, const SizeT y, const SignedInteger64 index) {
    Array<SignedInteger64, 4> indexesToCompare = {
        index - 1,
        index + 1,
        index - pixelMap.width,
        index + pixelMap.width
    };

    if (x == 0) { indexesToCompare[0] = index + pixelMap.width - 1; }
    else if (x == pixelMap.width - 1) { indexesToCompare[1] = index - pixelMap.width + 1; }

    if (y == 0) { indexesToCompare[2] = -1; }
    else if (y == pixelMap.height - 1) { indexesToCompare[3] = -1; }

    return indexesToCompare;
};

void UpdateCoastalStatuses(Vector<Province>& provincesVector, const PixelMap& pixelMap) {
    SignedInteger64 index = 0;
    for (SizeT y = 0; y < pixelMap.height; y++) {
        for (SizeT x = 0; x < pixelMap.width; x++) {
            const UnsignedInteger16 thisProvince = pixelMap.provinceIdMap[index];

            // Check left/right/up/down for a province with a different type
            Array<SignedInteger64, 4> indexesToCompare = GetNeighbourIndexes(pixelMap, x, y, index);

            // Compare this pixel's type to it's neighbour's types
            const TypeEnum thisProvinceType = provincesVector[thisProvince].type;

            for (const auto& indexToCompare : indexesToCompare) {
                if (indexToCompare == -1) { continue; }
                const UnsignedInteger16 neighbourProvince = pixelMap.provinceIdMap[indexToCompare];

                if (CoastalPixelFound(thisProvinceType, provincesVector[neighbourProvince].type)) {
                    provincesVector[thisProvince].isCoastal = true;
                    provincesVector[thisProvince].coastalNeighbours.insert(neighbourProvince);

                    provincesVector[neighbourProvince].isCoastal = true;
                    provincesVector[neighbourProvince].coastalNeighbours.insert(thisProvince);
                }
            }

            index++;
        }
    }
}

// Placement scoring. A candidate pixel is judged on how far it is from everything
// already placed, capped because past a dozen pixels the models already read as
// separate and chasing more just drives them into the corners.
constexpr SignedInteger32 placementSpreadCap = 12;

// How much room to insist on between a model and the province border, when the
// province is big enough to offer it. Raise it and models hug the middle; lower it
// and they drift towards the edges
constexpr SignedInteger32 placementClearanceTarget = 3;

// A province larger than this is sampled by stride rather than pixel by pixel, which
// keeps the cost flat on continent sized provinces without biasing where models land
constexpr SizeT placementCandidateCap = 1024;

void FindProvinceInteriors(Vector<Province>& provincesVector, PixelMap& pixelMap) {
    // Each pixels' distance to a nearest province
    Vector<UnsignedInteger16>& distances = pixelMap.borderDistance;
    distances.assign(pixelMap.area, 0);

    // Every pixel enters this exactly once, so works as both the queue and the visit order
    Vector<UnsignedInteger32> frontier;
    frontier.reserve(pixelMap.area);

    {
        SignedInteger64 index = 0;
        for (SizeT y = 0; y < pixelMap.height; y++) {
            for (SizeT x = 0; x < pixelMap.width; x++) {
                const UnsignedInteger16 thisProvince = pixelMap.provinceIdMap[index];
                Array<SignedInteger64, 4> indexesToCompare = GetNeighbourIndexes(pixelMap, x, y, index);

                for (const auto& indexToCompare : indexesToCompare) {
                    if (indexToCompare == -1 || pixelMap.provinceIdMap[indexToCompare] != thisProvince) {
                        distances[index] = 1;
                        frontier.push_back(static_cast<UnsignedInteger32>(index));
                        break;
                    }
                }

                index++;
            }
        }
    }

    {
        for (SizeT head = 0; head < frontier.size(); head++) {
            const SignedInteger64 index = static_cast<SignedInteger64>(frontier[head]);
            const UnsignedInteger16 thisProvince = pixelMap.provinceIdMap[index];

            const SizeT x = static_cast<SizeT>(index) % pixelMap.width;
            const SizeT y = static_cast<SizeT>(index) / pixelMap.width;

            Array<SignedInteger64, 4> indexesToCompare = GetNeighbourIndexes(pixelMap, x, y, index);

            for (const auto& indexToCompare : indexesToCompare) {
                if (indexToCompare == -1) { continue; }
                if (distances[indexToCompare] != 0) { continue; }                           // already reached
                if (pixelMap.provinceIdMap[indexToCompare] != thisProvince) { continue; }   // never leave the province

                distances[indexToCompare] = static_cast<UnsignedInteger16>(distances[index] + 1);
                frontier.push_back(static_cast<UnsignedInteger32>(indexToCompare));
            }
        }
    }

    for (auto& province : provincesVector) {
        province.interiorPixel = 0;
        province.interiorPixelDepth = 0;

        for (const auto& pixelIndex : province.pixelIndexes) {
            if (distances[pixelIndex] > province.interiorPixelDepth) {
                province.interiorPixelDepth = distances[pixelIndex];
                province.interiorPixel = pixelIndex;
            }
        }
    }
}

// Chebyshev distance between two pixels, taking the shorter way round the seam
SignedInteger32 PixelDistance(const PixelMap& pixelMap, const UnsignedInteger32 a, const UnsignedInteger32 b) {
    const SignedInteger32 ax = static_cast<SignedInteger32>(a % pixelMap.width);
    const SignedInteger32 ay = static_cast<SignedInteger32>(a / pixelMap.width);
    const SignedInteger32 bx = static_cast<SignedInteger32>(b % pixelMap.width);
    const SignedInteger32 by = static_cast<SignedInteger32>(b / pixelMap.width);

    SignedInteger32 dx = std::abs(ax - bx);
    if (dx > static_cast<SignedInteger32>(pixelMap.width) / 2) { dx = static_cast<SignedInteger32>(pixelMap.width) - dx; }

    return std::max(dx, std::abs(ay - by));
}

// Holds the candidate pixels of every province in one state, together with how far
// each of them currently sits from the nearest model already placed anywhere in the
// state. Keeping that distance and updating it as models land turns the search from
// "every candidate against every placed model" into one pass per placement, which is
// what makes this affordable on a map with fourteen thousand provinces.
//
// It replaces sampling a box around each province's interior point. A box big enough
// to hold every model overflows a small province, so the models that fell outside it
// used to be dropped onto whatever pixel came next in the province's raster ordered
// pixel list. That packed them into horizontal runs along whichever row the scan
// happened to start on, which in a province ten pixels across meant its border.
struct StatePlacements {
    struct Candidates {
        UnsignedInteger16 provinceIndex = 0;
        Boolean coastal = false;
        SignedInteger32 clearanceTarget = 1;
        Vector<UnsignedInteger32> pixels;
        Vector<SignedInteger32> spread;   // distance to the nearest model already placed
    };

    Vector<Candidates> provinces;
    Vector<UnsignedInteger32> taken;

    void Build(const PixelMap& pixelMap, const Vector<Province>& provincesVector, const Vector<UnsignedInteger16>& stateProvinces) {
        (void)pixelMap;
        provinces.clear();
        taken.clear();

        for (const auto& provinceIndex : stateProvinces) {
            if (provinceIndex >= provincesVector.size()) { continue; }

            const Province& province = provincesVector[provinceIndex];
            if (province.type != Land || province.pixelIndexes.empty()) { continue; }

            Candidates c;
            c.provinceIndex = provinceIndex;
            c.coastal = province.isCoastal;
            c.clearanceTarget = std::max(1, std::min<SignedInteger32>(
                placementClearanceTarget, static_cast<SignedInteger32>(province.interiorPixelDepth)));

            // Every pixel of a small province is a candidate. A large one is sampled by
            // stride, which caps the cost without biasing towards any part of it
            const SizeT size = province.pixelIndexes.size();
            const SizeT step = (size + placementCandidateCap - 1) / placementCandidateCap;
            c.pixels.reserve((size + step - 1) / step);
            for (SizeT i = 0; i < size; i += step) { c.pixels.push_back(province.pixelIndexes[i]); }
            c.spread.assign(c.pixels.size(), placementSpreadCap);

            provinces.push_back(std::move(c));
        }
    }

    void Add(const PixelMap& pixelMap, const UnsignedInteger32 pixel) {
        taken.push_back(pixel);
        for (auto& c : provinces) {
            for (SizeT i = 0; i < c.pixels.size(); i++) {
                c.spread[i] = std::min(c.spread[i], PixelDistance(pixelMap, c.pixels[i], pixel));
            }
        }
    }

    // Takes the best free spot in the state, or in just its coastal provinces. Clearance
    // is a filter rather than part of a score, because scoring it loses: pushing a model
    // as far as it can get from its neighbours pushes it into a corner, and a few points
    // of clearance never outweigh that.
    // onlyProvince restricts the search to one province, which is what a province
    // scoped type needs; anyProvince lets a state scoped type use the whole state.
    static constexpr UnsignedInteger16 anyProvince = 0xFFFF;

    Boolean Take(const PixelMap& pixelMap, const Boolean coastalOnly, const UnsignedInteger16 onlyProvince, UnsignedInteger32& chosen) {
        for (SignedInteger32 relax = 0; relax < placementClearanceTarget; relax++) {
            Boolean found = false;
            SignedInteger32 bestSpread = 0;
            UnsignedInteger32 bestPixel = 0;

            for (const auto& c : provinces) {
                if (coastalOnly && !c.coastal) { continue; }
                if (onlyProvince != anyProvince && c.provinceIndex != onlyProvince) { continue; }

                const SignedInteger32 minDepth = std::max(1, c.clearanceTarget - relax);

                for (SizeT i = 0; i < c.pixels.size(); i++) {
                    if (c.spread[i] <= 0) { continue; }   // already occupied
                    if (static_cast<SignedInteger32>(pixelMap.borderDistance[c.pixels[i]]) < minDepth) { continue; }

                    // Ties go to the lower pixel index, so two runs over one map agree
                    if (!found || c.spread[i] > bestSpread) { found = true; bestSpread = c.spread[i]; bestPixel = c.pixels[i]; }
                }
            }

            if (found) { chosen = bestPixel; Add(pixelMap, bestPixel); return true; }
        }

        return false;
    }
};

void LoadStateFiles(const Vector<Path>& stateFiles, Vector<Province>& provincesVector, Vector<State>& statesVector) {
    statesVector.reserve(stateFiles.size());

    for (const auto& stateFile : stateFiles) {
        PdxJson stateFileData = ParseFileToPdxJson(stateFile.string());

        if (!stateFileData.contains("state")) { continue; }

        for (const auto& stateData : stateFileData["state"].asList()) {
            if (!stateData.contains("id") || !stateData["id"][0].isInt()) {
                FatalError("State in " + stateFile.string() + " does not have a valid ID");
            }
            if (!stateData.contains("provinces") || !stateData["provinces"][0].isList() || !stateData["provinces"][0][0].isInt()) {
                FatalError("State in " + stateFile.string() + " does not have a valid provinces list");
            }

            statesVector.emplace_back(
                stateData["id"][0].as<UnsignedInteger16>(),
                stateData["provinces"][0].asVector<UnsignedInteger16>()
            );
        }
    }

    std::sort(statesVector.begin(), statesVector.end(), [](const State& a, const State& b) { return a.id < b.id; });

    for (auto& state : statesVector) {
        for (const auto& provinceId : state.provinces) {
            provincesVector[provinceId].stateId = state.id;

            if (provincesVector[provinceId].isCoastal) {
                state.isCoastal = true;
            }
        }
    }
}

Vector<String> LoadBuildingsToKeep(
    const Path& buildingsFile,
    const Set<UnsignedInteger16>& statesToRunOnSet,
    const Set<String>& autoGenerationDisabledSet
) {
    std::ifstream file(buildingsFile.string());

    const Boolean runOnEveryState = statesToRunOnSet.empty();

    String line;
    UnsignedInteger32 lineNumber = 0;
    Vector<String> keptLines;
    keptLines.reserve(96000);

    while (std::getline(file, line)) {
        lineNumber++;

        // Handle windows/mac differences and empty/comment lines
        if (!line.empty() && line.back() == '\r') { line.pop_back(); }
        const String trimmedLine = RemoveStringWhitespace(line);
        if (trimmedLine.empty() || trimmedLine[0] == '#') { continue; }

        // Only the first two fields decide anything of value for us, only look at themp
        const SizeT firstSemicolon = line.find(';');
        const SizeT secondSemicolon = (firstSemicolon == String::npos) ? String::npos : line.find(';', firstSemicolon + 1);

        if (secondSemicolon == String::npos) {
            FatalError(std::format("{0} line {1} does not have a state and a building type", buildingsFile.string(), lineNumber));
        }

        const String stateStr = RemoveStringWhitespace(line.substr(0, firstSemicolon));
        const String building  = RemoveStringWhitespace(line.substr(firstSemicolon + 1, secondSemicolon - firstSemicolon - 1));

        if (building.empty()) {
            FatalError(std::format("{0} line {1} has no building type", buildingsFile.string(), lineNumber));
        }
        if (!StringCanBecomeInteger(stateStr) || stateStr.size() > 5 || stateStr[0] == '-' || stateStr[0] == '+') {
            FatalError(std::format("{0} line {1} does not start with a state id, got \"{2}\"", buildingsFile.string(), lineNumber, stateStr));
        }

        const UnsignedInteger16 stateId = std::stoi(stateStr);

        const Boolean keepForState = !runOnEveryState && !statesToRunOnSet.contains(stateId);

        if (keepForState || autoGenerationDisabledSet.contains(building)) {
            keptLines.push_back(line);
        }
    }

    return keptLines;
}


constexpr Float64 seaLevelHeight = 9.50;
const String floatingHarbourName = "floating_harbor";
constexpr SignedInteger32 portFacingRadius = 8;
constexpr SignedInteger32 floatingHarbourOffshoreDistance = 7;
constexpr Float64 quarterTurn = 1.57079632679;

// Only these two types name the sea they open onto. Verified against the existing
// map/buildings.txt: all 2508 naval_base_spawn and all 2349 floating_harbor lines
// carry a non zero last column, and every other type carries zero. A port with no
// sea province crashes the game while it loads the map.
Boolean TypeNeedsSeaProvince(const String& name) {
    return name == "naval_base_spawn" || name == "floating_harbor";
}

// The sea a province's ports open onto: the lowest numbered sea province it
// touches, so the choice is identical on every run. Zero when there is none.
UnsignedInteger16 GetPortSeaProvince(const Vector<Province>& provincesVector, const Province& province) {
    UnsignedInteger16 seaProvince = 0;

    for (const auto& neighbour : province.coastalNeighbours) {
        if (provincesVector[neighbour].type != Sea) { continue; }
        if (seaProvince == 0 || neighbour < seaProvince) { seaProvince = neighbour; }
    }

    return seaProvince;
}

struct PortSite {
    UnsignedInteger32 landPixel = 0;
    UnsignedInteger32 seaPixel = 0;
    Boolean found = false;
};

// Where on the shoreline a port should stand. Preferring the spot with the most
// open water in front of it keeps ports out of one pixel inlets, where the model
// clips through the land behind it.
PortSite FindPortSite(const PixelMap& pixelMap, const Province& province, const UnsignedInteger16 seaProvince) {
    PortSite site;
    UnsignedInteger32 bestOpenness = 0;

    for (const auto& pixel : province.pixelIndexes) {
        const SizeT x = pixel % pixelMap.width;
        const SizeT y = pixel / pixelMap.width;

        const Array<SignedInteger64, 4> indexesToCompare = GetNeighbourIndexes(pixelMap, x, y, static_cast<SignedInteger64>(pixel));

        for (const auto& indexToCompare : indexesToCompare) {
            if (indexToCompare == -1) { continue; }
            if (pixelMap.provinceIdMap[indexToCompare] != seaProvince) { continue; }

            const SignedInteger32 seaX = static_cast<SignedInteger32>(indexToCompare % pixelMap.width);
            const SignedInteger32 seaY = static_cast<SignedInteger32>(indexToCompare / pixelMap.width);

            UnsignedInteger32 openness = 0;
            for (SignedInteger32 dy = -4; dy <= 4; dy++) {
                const SignedInteger32 sampleY = seaY + dy;
                if (sampleY < 0 || sampleY >= pixelMap.height) { continue; }

                for (SignedInteger32 dx = -4; dx <= 4; dx++) {
                    // The map is a cylinder, so the sample window wraps sideways
                    SignedInteger32 sampleX = seaX + dx;
                    while (sampleX < 0) { sampleX += pixelMap.width; }
                    while (sampleX >= pixelMap.width) { sampleX -= pixelMap.width; }

                    const UnsignedInteger32 sample = static_cast<UnsignedInteger32>(sampleY) * pixelMap.width + static_cast<UnsignedInteger32>(sampleX);
                    if (pixelMap.provinceIdMap[sample] == seaProvince) { openness++; }
                }
            }

            if (!site.found || openness > bestOpenness) {
                site.found = true;
                bestOpenness = openness;
                site.landPixel = pixel;
                site.seaPixel = static_cast<UnsignedInteger32>(indexToCompare);
            }
        }
    }

    return site;
}

// Deterministic hash used everywhere a placement decision needs to look random but
// stay identical between runs, so two runs over an unchanged map produce the same
// file and a diff shows only what really moved.
UnsignedInteger64 PlacementHash(const UnsignedInteger64 a, const String& name, const UnsignedInteger64 b, const UnsignedInteger64 c) {
    UnsignedInteger64 hash = 0xCBF29CE484222325ULL;

    for (const auto& value : { a, b, c }) {
        hash = (hash ^ value) * 0x100000001B3ULL;
        hash ^= hash >> 29;
    }
    for (const auto& character : name) {
        hash = (hash ^ static_cast<UnsignedInteger8>(character)) * 0x100000001B3ULL;
    }

    hash ^= hash >> 33;
    return hash;
}

// A stable pseudo random facing, so models do not all point the same way while the
// file stays byte identical between runs. The name is folded in as well, or two
// different types sharing one pixel would sit at the same angle and read as a
// single object.
Float64 RotationForPixel(const UnsignedInteger32 pixel, const String& name) {
    return static_cast<Float64>(PlacementHash(pixel, name, 0, 0) % 6283) / 1000.0;
}

// The angle from a pixel towards the bulk of a province around it, found by
// averaging that province's pixels inside a window. Aiming at a single shore pixel
// instead would only ever give the four cardinal directions, because the step from
// land to sea is one 4-neighbour hop, so every harbour on a diagonal or curved coast
// would sit visibly askew to the land it serves.
Float64 RotationTowardsProvince(
    const PixelMap& pixelMap,
    const UnsignedInteger32 fromPixel,
    const UnsignedInteger16 provinceIndex,
    const SignedInteger32 radius,
    Boolean& found
) {
    found = false;

    const SignedInteger32 fromX = static_cast<SignedInteger32>(fromPixel % pixelMap.width);
    const SignedInteger32 fromY = static_cast<SignedInteger32>(fromPixel / pixelMap.width);

    Float64 sumX = 0.0;
    Float64 sumY = 0.0;
    UnsignedInteger32 samples = 0;

    for (SignedInteger32 dy = -radius; dy <= radius; dy++) {
        const SignedInteger32 y = fromY + dy;
        if (y < 0 || y >= pixelMap.height) { continue; }

        for (SignedInteger32 dx = -radius; dx <= radius; dx++) {
            // The map is a cylinder, so x wraps. dx itself is already the signed
            // offset, so nothing has to be unwrapped when it is accumulated
            SignedInteger32 x = fromX + dx;
            while (x < 0) { x += pixelMap.width; }
            while (x >= pixelMap.width) { x -= pixelMap.width; }

            const UnsignedInteger32 pixel = static_cast<UnsignedInteger32>(y) * pixelMap.width + static_cast<UnsignedInteger32>(x);
            if (pixelMap.provinceIdMap[pixel] != provinceIndex) { continue; }

            sumX += static_cast<Float64>(dx);
            sumY += static_cast<Float64>(dy);
            samples++;
        }
    }

    // Nothing of the province in range, so the caller has to fall back. Reporting it
    // rather than returning zero matters because zero is a real angle: due east
    if (samples == 0) { return 0.0; }
    found = true;

    // World z runs south to north while pixel rows run north to south
    return std::atan2(-sumY / static_cast<Float64>(samples), sumX / static_cast<Float64>(samples));
}

// The angle a model at fromPixel needs in order to point at toPixel. World z runs
// south to north while pixel rows run north to south, so the vertical delta is
// negated on the way into world space.
Float64 RotationFacing(const PixelMap& pixelMap, const UnsignedInteger32 fromPixel, const UnsignedInteger32 toPixel) {
    const Float64 fromX = static_cast<Float64>(fromPixel % pixelMap.width);
    const Float64 fromY = static_cast<Float64>(fromPixel / pixelMap.width);
    const Float64 toX   = static_cast<Float64>(toPixel % pixelMap.width);
    const Float64 toY   = static_cast<Float64>(toPixel / pixelMap.width);

    return std::atan2(fromY - toY, toX - fromX);
}

// Walks out from the shore into open water, staying inside the same sea province, so
// a floating harbour sits clear of the coastline instead of clipping it.
UnsignedInteger32 PushOffshore(
    const PixelMap& pixelMap,
    const UnsignedInteger32 landPixel,
    const UnsignedInteger32 seaPixel,
    const UnsignedInteger16 seaProvince,
    const SignedInteger32 distance
) {
    const SignedInteger32 landX = static_cast<SignedInteger32>(landPixel % pixelMap.width);
    const SignedInteger32 landY = static_cast<SignedInteger32>(landPixel / pixelMap.width);
    const SignedInteger32 seaX  = static_cast<SignedInteger32>(seaPixel % pixelMap.width);
    const SignedInteger32 seaY  = static_cast<SignedInteger32>(seaPixel / pixelMap.width);

    // The two are 4-neighbours, so each step is -1, 0 or 1. Anything larger means the
    // pair straddles the seam where the map wraps, and the direction is inverted
    SignedInteger32 stepX = seaX - landX;
    if (stepX > 1) { stepX = -1; }
    else if (stepX < -1) { stepX = 1; }

    const SignedInteger32 stepY = seaY - landY;

    UnsignedInteger32 offshore = seaPixel;

    for (SignedInteger32 step = 1; step <= distance; step++) {
        const SignedInteger32 y = seaY + stepY * step;
        if (y < 0 || y >= pixelMap.height) { break; }

        SignedInteger32 x = seaX + stepX * step;
        while (x < 0) { x += pixelMap.width; }
        while (x >= pixelMap.width) { x -= pixelMap.width; }

        const UnsignedInteger32 pixel = static_cast<UnsignedInteger32>(y) * pixelMap.width + static_cast<UnsignedInteger32>(x);
        if (pixelMap.provinceIdMap[pixel] != seaProvince) { break; }

        offshore = pixel;
    }

    return offshore;
}

// Scatters a state's models across the whole state, keeping each one clear of every
// other state level model already placed there. `taken` is shared by every state
// scoped type in the state, which is what stops two different buildings landing on
// one pixel; it is appended to as models are accepted.
Vector<UnsignedInteger32> PickStatePlacements(
    const PixelMap& pixelMap,
    StatePlacements& placements,
    const Boolean coastalOnly,
    const SizeT count,
    const UnsignedInteger16 onlyProvince = StatePlacements::anyProvince
) {
    Vector<UnsignedInteger32> pixels;
    pixels.reserve(count);

    for (SizeT modelIndex = 0; modelIndex < count; modelIndex++) {
        UnsignedInteger32 chosen = 0;
        if (!placements.Take(pixelMap, coastalOnly, onlyProvince, chosen)) { break; }
        pixels.push_back(chosen);
    }

    return pixels;
}


// Hands out up to count pixels of the province, walking outwards ring by ring from
// a centre, so repeated models of one type do not stack on a single pixel. Used for
// ports, which have to start from the shoreline rather than the middle.
Vector<UnsignedInteger32> SpreadInProvince(
    const PixelMap& pixelMap,
    const UnsignedInteger16 provinceIndex,
    const UnsignedInteger32 centrePixel,
    const SizeT count,
    const SignedInteger32 maxRadius
) {
    Vector<UnsignedInteger32> pixels;
    if (count == 0) { return pixels; }
    pixels.reserve(count);

    const SignedInteger32 centreX = static_cast<SignedInteger32>(centrePixel % pixelMap.width);
    const SignedInteger32 centreY = static_cast<SignedInteger32>(centrePixel / pixelMap.width);

    // Past half the map width the horizontal wrap brings the walk back around to
    // pixels it has already visited, so never let the radius reach that far
    const SignedInteger32 radiusLimit = std::min(maxRadius, static_cast<SignedInteger32>(pixelMap.width / 2));

    // Try to keep models a few pixels apart, but relax rather than fail in a
    // province too small to honour it
    for (SignedInteger32 spacing = 3; spacing >= 0; spacing--) {
        pixels.clear();

        for (SignedInteger32 radius = 0; radius <= radiusLimit && pixels.size() < count; radius++) {
            for (SignedInteger32 dy = -radius; dy <= radius && pixels.size() < count; dy++) {
                for (SignedInteger32 dx = -radius; dx <= radius && pixels.size() < count; dx++) {
                    // Only the outermost ring is new on each pass
                    if (radius > 0 && std::abs(dx) != radius && std::abs(dy) != radius) { continue; }

                    const SignedInteger32 y = centreY + dy;
                    if (y < 0 || y >= pixelMap.height) { continue; }

                    SignedInteger32 x = centreX + dx;
                    while (x < 0) { x += pixelMap.width; }
                    while (x >= pixelMap.width) { x -= pixelMap.width; }

                    const UnsignedInteger32 pixel = static_cast<UnsignedInteger32>(y) * pixelMap.width + static_cast<UnsignedInteger32>(x);
                    if (pixelMap.provinceIdMap[pixel] != provinceIndex) { continue; }

                    Boolean tooClose = false;
                    for (const auto& chosen : pixels) {
                        // One model per spot: the same pixel can be reachable at more
                        // than one offset once the walk wraps
                        if (chosen == pixel) { tooClose = true; break; }

                        const SignedInteger32 chosenX = static_cast<SignedInteger32>(chosen % pixelMap.width);
                        const SignedInteger32 chosenY = static_cast<SignedInteger32>(chosen / pixelMap.width);
                        if (std::abs(chosenX - x) < spacing && std::abs(chosenY - y) < spacing) { tooClose = true; break; }
                    }
                    if (tooClose) { continue; }

                    pixels.push_back(pixel);
                }
            }
        }

        if (pixels.size() >= count) { break; }
    }

    return pixels;
}

String FormatBuildingLine(
    const PixelMap& pixelMap,
    const UnsignedInteger16 stateId,
    const String& name,
    const UnsignedInteger32 pixel,
    const Float64 rotation,
    const UnsignedInteger16 seaProvince,
    const Boolean floats = false
) {
    const Float64 x = static_cast<Float64>(pixel % pixelMap.width);
    const Float64 pixelY = static_cast<Float64>(pixel / pixelMap.width);
    const Float64 y = floats ? seaLevelHeight : std::max(seaLevelHeight, static_cast<Float64>(pixelMap.heightmap[pixel]) / 10.0);
    const Float64 z = static_cast<Float64>(pixelMap.height - 1) - pixelY;

    Char lineBuffer[256];
    const int written = std::snprintf(lineBuffer, sizeof(lineBuffer), "%u;%s;%.2f;%.2f;%.2f;%.2f;%u",
        static_cast<unsigned>(stateId), name.c_str(), x, y, z, rotation, static_cast<unsigned>(seaProvince));

    if (written <= 0 || written >= static_cast<int>(sizeof(lineBuffer))) {
        FatalError("Building line too long to write for " + name);
    }

    return String(lineBuffer, static_cast<SizeT>(written));
}

void WriteBuildings(
    const Path& buildingsFile,
    const Vector<String>& keptLines,
    const Vector<SpawnPoint>& spawnPointsVector,
    const Set<UnsignedInteger16>& statesToRunOnSet,
    const Vector<Province>& provincesVector,
    const Vector<State>& statesVector,
    const PixelMap& pixelMap,
    const Boolean sortBuildingsFile
) {
    Timestamp generateBuildingsStartTime = std::chrono::steady_clock::now();
    const Boolean runOnEveryState = statesToRunOnSet.empty();

    // Prepare the out file; state id - line
    Vector<std::pair<UnsignedInteger16, String>> lines;
    lines.reserve(keptLines.size() + 96000);
    for (const auto& line : keptLines) {
		lines.emplace_back(static_cast<UnsignedInteger16>(std::stoi(line.substr(0, line.find(';')))), line);
    }

	// Loop over every state, skip any we don't need to make buildings for
    for (const auto& state : statesVector) {
        if (!runOnEveryState && !statesToRunOnSet.contains(state.id)) { continue; }

        Boolean hasLand = false;
        for (const auto& provinceIndex : state.provinces) {
            if (provinceIndex >= provincesVector.size()) { continue; }

            const Province& province = provincesVector[provinceIndex];
            if (province.type == Land && !province.pixelIndexes.empty()) { hasLand = true; break; }
        }

        // Nothing of this state is drawn on the map, so there is nowhere to build
        if (!hasLand) { continue; }

        // Every state level model in this state is placed against this one structure,
        // so an arms factory knows where the radar station went and will not stand on
        // top of it. Province level models keep to their own province and are not
        // part of it. Built once per state, because the cost of building it is the
        // same whether one model or forty are placed against it.
        StatePlacements statePlacements;
        statePlacements.Build(pixelMap, provincesVector, state.provinces);

        for (const auto& spawnPoint : spawnPointsVector) {
            if (spawnPoint.count <= 0) { continue; }

            const SizeT count = static_cast<SizeT>(spawnPoint.count);
            const Boolean needsSea = TypeNeedsSeaProvince(spawnPoint.name);

            // State level models scatter across the whole state rather than crowding
            // into one province. Only the two port types name a sea province and both
            // are province scoped, so nothing on this path needs one.
            if (!spawnPoint.provincial) {
                for (const auto& pixel : PickStatePlacements(pixelMap, statePlacements, spawnPoint.onlyCoastal, count)) {
                    lines.emplace_back(state.id, FormatBuildingLine(
                        pixelMap, state.id, spawnPoint.name, pixel, RotationForPixel(pixel, spawnPoint.name), 0));
                }

                continue;
            }

            Vector<UnsignedInteger16> targets;
            for (const auto& provinceIndex : state.provinces) {
                if (provinceIndex < provincesVector.size()) { targets.push_back(provinceIndex); }
            }

            for (const auto& provinceIndex : targets) {
                const Province& province = provincesVector[provinceIndex];

                if (province.type != Land || province.pixelIndexes.empty()) { continue; }
                if (spawnPoint.onlyCoastal && !province.isCoastal) { continue; }

                UnsignedInteger16 seaProvince = 0;
                PortSite site;
                Vector<UnsignedInteger32> placements;

				// Port handling
                if (needsSea) {
                    // A port with no sea province crashes the game so skip it
                    seaProvince = GetPortSeaProvince(provincesVector, province);
                    if (seaProvince == 0) { continue; }

                    site = FindPortSite(pixelMap, province, seaProvince);
                    if (!site.found) { continue; }

                    // Find where in the province to place the port
                    placements = SpreadInProvince(pixelMap, provinceIndex, site.landPixel, count, 24);
                }
                else {
                    placements = PickStatePlacements(pixelMap, statePlacements, false, count, provinceIndex);
                }

                for (const auto& pixel : placements) {
                    Float64 rotation = 0.0;
					
					if (needsSea) {
						Boolean facingFound = false;
						rotation = RotationTowardsProvince(pixelMap, pixel, seaProvince, portFacingRadius, facingFound);
						if (!facingFound) {
							rotation = RotationFacing(pixelMap, pixel, site.seaPixel);
						}
					}
					else {
						rotation = RotationForPixel(pixel, spawnPoint.name);
					}
					
					rotation += quarterTurn;

                    lines.emplace_back(state.id, FormatBuildingLine(
                        pixelMap, state.id, spawnPoint.name, pixel, rotation, seaProvince));
                }
            }
        }

        // Handle floating harbours
        for (const auto& provinceIndex : state.provinces) {
            if (provinceIndex >= provincesVector.size()) { continue; }

            const Province& province = provincesVector[provinceIndex];
            if (province.type != Land || province.pixelIndexes.empty()) { continue; }
            if (!province.isCoastal) { continue; }

            const UnsignedInteger16 seaProvince = GetPortSeaProvince(provincesVector, province);
            if (seaProvince == 0) { continue; }

            const PortSite site = FindPortSite(pixelMap, province, seaProvince);
            if (!site.found) { continue; }

            const UnsignedInteger32 offshore = PushOffshore(pixelMap, site.landPixel, site.seaPixel, seaProvince, floatingHarbourOffshoreDistance);

            Boolean facingFound = false;
            Float64 rotation = RotationTowardsProvince(pixelMap, offshore, provinceIndex, portFacingRadius, facingFound);
            if (!facingFound) { rotation = RotationFacing(pixelMap, offshore, site.landPixel); }

			rotation += quarterTurn;

            lines.emplace_back(state.id, FormatBuildingLine(
                pixelMap, state.id, floatingHarbourName, offshore, rotation, seaProvince, true));
        }
    }

    if (sortBuildingsFile) { std::sort(lines.begin(), lines.end()); }

    // Write the file
    String output;
    output.reserve(lines.size() * 48);
    for (const auto& [stateId, line] : lines) {
        output += line;
        output += "\r\n";
    }

    std::ofstream file(buildingsFile.string(), std::ios::binary);
    if (!file) { FatalError("Could not write " + buildingsFile.string()); }
    file.write(output.data(), static_cast<std::streamsize>(output.size()));
    if (!file) { FatalError("Failed while writing " + buildingsFile.string()); }

    std::cout << std::format("Generated {0} building models in {1}\n", lines.size() - keptLines.size(), GetTimeElapsedFromStart(generateBuildingsStartTime));
}

int main() {
    Timestamp startTime = std::chrono::steady_clock::now();
	
	// Define type names
	const HashMap<String, TypeEnum> typeMap = {
		{ "land", Land },
		{ "lake", Lake },
		{ "sea", Sea }
	};
	const Vector<String> localisedTypeNames = { "land", "lake", "sea" };

	// Get the mod directory and load the JSON file
	Path modDirectory = std::filesystem::current_path().parent_path().parent_path();
    Path vanillaDirectory;
    Vector<UnsignedInteger16> statesToRunOn;
    Boolean sortBuildingsFile = false;
    LoadDataFromJson("inputs.json", vanillaDirectory, statesToRunOn, sortBuildingsFile);
    const Set<UnsignedInteger16> statesToRunOnSet(statesToRunOn.begin(), statesToRunOn.end());

    // Load the mod's replace directories
    Set<String> modReplaceDirectories = LoadReplacePathsFromModFile(modDirectory);

    // Load all of the spawn points we'll need to write for
    Vector<SpawnPoint> spawnPointsVector;
    Set<String> autoGenerationDisabledSet;
    LoadBuildings(
        GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common/buildings", ".txt", 4),
        spawnPointsVector,
        autoGenerationDisabledSet
    );

    /*
    for (const auto& spawnPoint : spawnPointsVector) {
        std::cout << std::format("{0}:\n - Count: {1}\n - Provincial: {2}\n - Coastal: {3}\n - Disable Auto Nudging: {4}\n\n",
            spawnPoint.name, spawnPoint.count, spawnPoint.provincial, spawnPoint.onlyCoastal, spawnPoint.disableAutoNudging);
    }
    */

    // Load provinces from definition.csv
    Vector<Province> provincesVector = LoadProvinces(
        GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "map/definition.csv"),
        typeMap
    );

    // Create an RGB -> index map for provinces
	HashMap<UnsignedInteger32, UnsignedInteger16> provinceColourToIndexMap;
	{
		UnsignedInteger16 provinceIndex = 0;
		for (const auto& province : provincesVector) {
			provinceColourToIndexMap[province.colour.ToInteger()] = provinceIndex++;
		}
	}

    // Load the map from provinces.bmp and heightmap.bmp
    PixelMap pixelMap = PixelMap(
        GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "map/provinces.bmp"),
        GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "map/heightmap.bmp"),
        provincesVector,
        provinceColourToIndexMap
    );

    // Update province coastal status
    UpdateCoastalStatuses(provincesVector, pixelMap);

    // Get the province interiors
    FindProvinceInteriors(provincesVector, pixelMap);

    // Load state files
    Vector<State> statesVector;
    LoadStateFiles(
        GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "history/states", ".txt", 1500),
        provincesVector,
        statesVector
    );

    Vector<String> keptLines = LoadBuildingsToKeep(
        GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "map/buildings.txt"),
        statesToRunOnSet,
        autoGenerationDisabledSet
    );

    std::cout << "Files took " << GetTimeElapsedFromStart(startTime) << " to load.\n";

    WriteBuildings(
        modDirectory / "map/buildings.txt",
        keptLines,
        spawnPointsVector,
        statesToRunOnSet,
        provincesVector,
        statesVector,
        pixelMap,
        sortBuildingsFile
    );

	return 0;
}
