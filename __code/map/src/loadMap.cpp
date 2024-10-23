#include "mapTypes.hpp"
#include "loadMap.hpp"
#include "func.hpp"

#include <filesystem>
#include <regex>
#include <fstream>
#include <thread>

static void loadTerrain(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, PDX::vectorStringIndexMap<PDX::terrain>& terrainArray) {
	std::vector<std::filesystem::path> filesVector = findFilesToLoad("common/terrain", vanillaGamePath, modPath);

	for (const auto& path : filesVector) {
		std::string str = returnTXTFileAsStringNoHashes(path);
		str = returnStringBetweenBrackets(str, "categories");

		std::regex contentEntryRegex(R"((\w+)\s*=\s*\{)");
		while (regex_search(str, contentEntryRegex)) {
			std::smatch match;
			if (regex_search(str, match, contentEntryRegex)) {
				std::string name = match[1];
				std::string data = returnStringBetweenBrackets(str, name);

				PDX::terrain T_terrain;
				T_terrain.name = name;


				std::regex isNavalRegex(R"(naval_terrain\s*=\s*yes)");
				T_terrain.naval = regex_search(data, isNavalRegex);

				std::regex isWaterRegex(R"(is_water\s*=\s*yes)");
				T_terrain.is_water = regex_search(data, isWaterRegex);

				std::string rgb = returnStringBetweenBrackets(data, "color");

				std::regex colourRegex(R"((\d{1,3})\s+(\d{1,3})\s+(\d{1,3}))");
				std::smatch match2;
				if (regex_search(rgb, match2, colourRegex)) {
					T_terrain.red = stoi(match2[1]);
					T_terrain.green = stoi(match2[2]);
					T_terrain.blue = stoi(match2[3]);
				}

				terrainArray.vector.push_back(T_terrain);

				str = removeStringBetweenBrackets(str, name);
			}
		}
	}

	terrainArray.mapFromVector();
}

static void loadBuildings(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, PDX::vectorStringIndexMap<PDX::building>& buildingArray) {

	std::vector<std::filesystem::path> filesVector = findFilesToLoad("common/buildings", vanillaGamePath, modPath);

	for (const auto& path : filesVector) {
		std::string str = returnTXTFileAsStringNoHashes(path);
		str = returnStringBetweenBrackets(str, "buildings");

		std::regex contentEntryRegex(R"((\w+)\s*=\s*\{)");
		while (regex_search(str, contentEntryRegex)) {
			std::smatch match;
			if (regex_search(str, match, contentEntryRegex)) {
				std::string name = match[1];
				std::string data = returnStringBetweenBrackets(str, name);

				PDX::building T_building;
				T_building.name = name;

				std::regex isProvincialRegex(R"(provincial\s*=\s*yes)");
				T_building.provincial = regex_search(data, isProvincialRegex);

				std::regex isCoastalRegex(R"(only_costal\s*=\s*yes)");
				T_building.only_coastal = regex_search(data, isCoastalRegex);

				std::regex maxLevelRegex(R"(max_level\s*=\s*(\d+))");
				std::smatch match2;
				if (regex_search(data, match2, maxLevelRegex)) {
					T_building.max_level = stoi(match2[1]);
				}

				buildingArray.vector.push_back(T_building);
				str = removeStringBetweenBrackets(str, name);
			}
		}
	}

	buildingArray.mapFromVector();
}

static void loadStateCategories(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, PDX::vectorStringIndexMap<PDX::state_category>& stateCategoryArray) {
	std::vector<std::filesystem::path> filesVector = findFilesToLoad("common/state_category", vanillaGamePath, modPath);

	for (const auto& path : filesVector) {
		std::string str = returnTXTFileAsStringNoHashes(path);
		str = returnStringBetweenBrackets(str, "state_categories");

		std::regex contentEntryRegex(R"((\w+)\s*=\s*\{)");
		while (regex_search(str, contentEntryRegex)) {
			std::smatch match;
			if (regex_search(str, match, contentEntryRegex)) {
				std::string name = match[1];
				std::string data = returnStringBetweenBrackets(str, name);

				PDX::state_category T_stateCategory;
				T_stateCategory.name = name;

				uint8_t building_slots = 0;
				std::regex buildingSlotRegex(R"(local_building_slots\s*=\s*(\d+))");
				std::smatch match2;
				if (regex_search(data, match2, buildingSlotRegex)) {
					building_slots = stoi(match2[1]);
				}
				T_stateCategory.building_slots = building_slots;

				uint8_t red = 0; uint8_t green = 0; uint8_t blue = 0;
				std::string rgb = returnStringBetweenBrackets(data, "color");

				std::regex colourRegex(R"((\d{1,3})\s+(\d{1,3})\s+(\d{1,3}))");
				std::smatch match3;
				if (regex_search(rgb, match3, colourRegex)) {
					red = stoi(match3[1]);
					green = stoi(match3[2]);
					blue = stoi(match3[3]);
				}

				T_stateCategory.red = red;
				T_stateCategory.green = green;
				T_stateCategory.blue = blue;

				stateCategoryArray.vector.push_back(T_stateCategory);

				str = removeStringBetweenBrackets(str, name);
			}
		}
	}

	stateCategoryArray.mapFromVector();
}

static void loadResources(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, PDX::vectorStringIndexMap<PDX::resource>& resourcesArray) {
	std::vector<std::filesystem::path> filesVector = findFilesToLoad("common/resources", vanillaGamePath, modPath);

	for (const auto& path : filesVector) {
		std::string str = returnTXTFileAsStringNoHashes(path);
		str = returnStringBetweenBrackets(str, "resources");

		std::regex contentEntryRegex(R"((\w+)\s*=\s*\{)");
		while (regex_search(str, contentEntryRegex)) {
			std::smatch match;
			if (regex_search(str, match, contentEntryRegex)) {
				std::string name = match[1];

				resourcesArray.vector.emplace_back(name);

				str = removeStringBetweenBrackets(str, name);
			}
		}
	}

	resourcesArray.mapFromVector();
}

static void loadCountryColours(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, std::filesystem::path& colourFilePath, PDX::vectorStringIndexMap<PDX::country>& countriesArray) {
	for (auto& country : countriesArray.vector) {
		std::filesystem::path modColour = modPath / "common" / country.file;
		std::filesystem::path vanillaColour = vanillaGamePath / "common" / country.file;

		std::string data;
		if (std::filesystem::exists(modColour)) data = returnTXTFileAsStringNoHashes(modColour);
		else data = returnTXTFileAsStringNoHashes(vanillaColour);

		std::regex colourRegex(R"(color\s*=\s*(\w*)\s*\{\s*([a-zA-Z0-9_.]+) ([a-zA-Z0-9_.]+) ([a-zA-Z0-9_.]+)\s*\})");
		std::smatch match;
		if (regex_search(data, match, colourRegex)) {
			std::string colourType = match[1];
			std::string v1 = match[2];
			std::string v2 = match[3];
			std::string v3 = match[4];

			if (colourType == "hsv" || colourType == "HSV") {
				if (isFloat(v1) && isFloat(v2) && isFloat(v3)) country.HSVToRGB(std::stod(v1), std::stod(v2), std::stod(v3));

			}
			else {
				if (isInt(v1)) country.red = std::stoi(v1);
				else if (isFloat(v1)) country.red = doubleToIntEightBit(std::stod(v1));

				if (isInt(v2)) country.green = std::stoi(v2);
				else if (isFloat(v2)) country.green = doubleToIntEightBit(std::stod(v2));

				if (isInt(v3)) country.blue = std::stoi(v3);
				else if (isFloat(v3)) country.blue = doubleToIntEightBit(std::stod(v3));
			}
		}
	}

	//now load from colors.txt
	std::string str = returnTXTFileAsStringNoHashes(colourFilePath);

	std::regex contentEntryRegex(R"((\w+)\s*=\s*\{)");
	while (regex_search(str, contentEntryRegex)) {
		std::smatch match;
		if (regex_search(str, match, contentEntryRegex)) {
			std::string name = match[1];

			auto vecIndex = countriesArray.hashMap.find(name);
			if (vecIndex != countriesArray.hashMap.end()) {

				std::string colourData = returnStringBetweenBrackets(str, name);

				std::regex colourRegex(R"(color\s*=\s*(\w*)\s*\{\s*([a-zA-Z0-9_.]+) ([a-zA-Z0-9_.]+) ([a-zA-Z0-9_.]+)\s*\})");
				std::smatch match2;
				if (regex_search(str, match2, colourRegex)) {
					std::string colourType = match[1];
					std::string v1 = match2[2];
					std::string v2 = match2[3];
					std::string v3 = match2[4];

					if (colourType == "hsv" || colourType == "HSV") {
						if (isFloat(v1) && isFloat(v2) && isFloat(v3)) countriesArray.vector[vecIndex->second].HSVToRGB(std::stod(v1), std::stod(v2), std::stod(v3));

					}
					else {
						if (isInt(v1)) countriesArray.vector[vecIndex->second].red = std::stoi(v1);
						else if (isFloat(v1)) countriesArray.vector[vecIndex->second].red = doubleToIntEightBit(std::stod(v1));

						if (isInt(v2)) countriesArray.vector[vecIndex->second].green = std::stoi(v2);
						else if (isFloat(v2)) countriesArray.vector[vecIndex->second].green = doubleToIntEightBit(std::stod(v2));

						if (isInt(v3)) countriesArray.vector[vecIndex->second].blue = std::stoi(v3);
						else if (isFloat(v3)) countriesArray.vector[vecIndex->second].blue = doubleToIntEightBit(std::stod(v3));
					}
				}

				//std::cout << "Key: " << key << ", Value: " << it->second << std::endl;
			}

			str = removeStringBetweenBrackets(str, name);
		}
	}
}

static void loadCountries(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, PDX::vectorStringIndexMap<PDX::country>& countriesArray) {
	std::vector<std::filesystem::path> filesVector = findFilesToLoad("common/country_tags", vanillaGamePath, modPath);

	std::regex whiteSpaceRegex("\\s+");
	std::regex forwardSlashRegex("/");
	std::regex countryRegex(R"(\s*(\w{3})\s*=\s*\"(.+)\")");
	std::regex dynamicRegex(R"(dynamic_tags\s*=\s*yes\")");

	for (const auto& file : filesVector) {
		std::string line;
		std::ifstream fileThis(file);
		bool dynamic = false;

		while (getline(fileThis, line)) {
			std::smatch match;
			std::smatch match2;
			std::string lineNoSpaces = regex_replace(line, whiteSpaceRegex, "");

			size_t hashPos = lineNoSpaces.find('#');
			if (hashPos != std::string::npos) lineNoSpaces = lineNoSpaces.substr(0, hashPos);

			if (regex_search(lineNoSpaces, match, dynamicRegex)) dynamic = true;

			if (regex_search(lineNoSpaces, match2, countryRegex)) {
				PDX::country T_country;



				T_country.tag = match2[1];
				T_country.file = match2[2];
				T_country.file = regex_replace(T_country.file, forwardSlashRegex, "\\");
				T_country.dynamic = dynamic;

				countriesArray.vector.push_back(T_country);
			}
		}
	}

	countriesArray.mapFromVector();
	std::filesystem::path modColours = modPath / "common\\countries\\colors.txt";
	std::filesystem::path vanillaColours = vanillaGamePath / "common\\countries\\colors.txt";

	//No need to check for replaces, that file is the only valid one so if it exists it will always overwrite
	if (std::filesystem::exists(modColours)) loadCountryColours(vanillaGamePath, modPath, modColours, countriesArray);
	else loadCountryColours(vanillaGamePath, modPath, vanillaColours, countriesArray);
}

void loadMap(
	const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath,

	PDX::vectorStringIndexMap<PDX::terrain>& terrainArray,
	PDX::vectorStringIndexMap<PDX::building>& buildingArray,
	PDX::vectorStringIndexMap<PDX::resource>& resourcesArray,
	PDX::vectorStringIndexMap<PDX::state_category>& stateCategoryArray,
	PDX::vectorStringIndexMap<PDX::country>& countriesArray
) {
	std::thread t1(loadTerrain, std::cref(vanillaGamePath), std::cref(modPath), std::ref(terrainArray));
	std::thread t2(loadBuildings, std::cref(vanillaGamePath), std::cref(modPath), std::ref(buildingArray));
	std::thread t3(loadResources, std::cref(vanillaGamePath), std::cref(modPath), std::ref(resourcesArray));
	std::thread t4(loadStateCategories, std::cref(vanillaGamePath), std::cref(modPath), std::ref(stateCategoryArray));
	std::thread t5(loadCountries, std::cref(vanillaGamePath), std::cref(modPath), std::ref(countriesArray));

	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
}