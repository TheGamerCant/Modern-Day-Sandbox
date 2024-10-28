#include "mapTypes.hpp"
#include "loadMap.hpp"
#include "func.hpp"

#include <filesystem>
#include <regex>
#include <fstream>
#include <sstream>
#include <thread>

#include <mutex>
#include <future>

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

static void loadBuildings(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, PDX::vectorStringIndexMap<PDX::building>& stateBuildingsArray, PDX::vectorStringIndexMap<PDX::building>& provinceBuildingsArray) {

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

				if (T_building.provincial) provinceBuildingsArray.vector.push_back(T_building);
				else stateBuildingsArray.vector.push_back(T_building);

				str = removeStringBetweenBrackets(str, name);
			}
		}
	}

	stateBuildingsArray.mapFromVector();
	provinceBuildingsArray.mapFromVector();
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

static void loadProvinces(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, std::vector<PDX::province>& provincesArray,
	PDX::vectorStringIndexMap<PDX::terrain>& terrainArray, PDX::vectorStringIndexMap<PDX::building>& provinceBuildingsArray) {
	std::filesystem::path definitionsCSV = modPath;
	definitionsCSV += "\\map\\definition.csv";

	if (!std::filesystem::exists(definitionsCSV)) { definitionsCSV = vanillaGamePath; definitionsCSV += "\\map\\definition.csv"; }

	std::ifstream file1(definitionsCSV);
	std::string line1;
	int number_of_lines = 0;
	while (std::getline(file1, line1)) ++number_of_lines;

	provincesArray.reserve(number_of_lines);

	std::string line2;
	std::ifstream file2(definitionsCSV);
	while (getline(file2, line2)) {

		size_t hashPos = line2.find('#');
		if (hashPos != std::string::npos) line2 = line2.substr(0, hashPos);

		std::istringstream ss(line2);
		std::string token;
		std::vector<std::string> tokens(8, "");

		int i = 0;
		while (getline(ss, token, ';')) {
			if ( i < 8) tokens[i] = token;
			++i;
		}
		uint16_t id = stoi(tokens[0]);
		uint8_t red = stoi(tokens[1]);
		uint8_t green = stoi(tokens[2]);
		uint8_t blue = stoi(tokens[3]);
		std::string typeStr = tokens[4];
		uint8_t type = PDX::provinceTypeLand;
		if (typeStr == "lake") type = PDX::provinceTypeLake;
		else if (typeStr == "sea") type = PDX::provinceTypeSea;
		bool coastal = (tokens[5] == "true");
		std::string terrainStr = tokens[6];
		int continent = stoi(tokens[7]);

		std::vector<uint8_t> buildings(provinceBuildingsArray.vector.size(), 0);

		PDX::terrain* terrainPointer = nullptr;
		auto terrainIndex = terrainArray.hashMap.find(terrainStr);
		if (terrainIndex != terrainArray.hashMap.end()) terrainPointer = &terrainArray.vector[terrainIndex->second];

		provincesArray.emplace_back(id, red, green, blue, type, coastal, continent, terrainPointer, buildings);
	} 
}

static uint16_t returnStateID(std::string& fileContent) {
	uint16_t id = 0;

	std::regex idRegex("id\\s*=\\s*(\\d+)");
	std::smatch match;
	if (regex_search(fileContent, match, idRegex)) {
		id = stoi(match[1]);
	}
	fileContent = regex_replace(fileContent, idRegex, "");

	return id;
}

static std::vector <std::string> returnStateDates(std::string& historyContent) {
	std::vector<std::string> dates;

	std::regex dateInfoRegex("(\\d{4})\\.(\\d{1,2})\\.(\\d{1,2})");
	while (regex_search(historyContent, dateInfoRegex)) {
		std::smatch match;
		regex_search(historyContent, match, dateInfoRegex);
		std::string date = match[0].str();
		std::string dateInfoString = returnStringBetweenBrackets(historyContent, date);

		dates.emplace_back(date + " = {" + dateInfoString + "}");

		historyContent = removeStringBetweenBrackets(historyContent, date);
	}

	return dates;
}

static PDX::country* returnStateOwner(std::string& historyContent, PDX::vectorStringIndexMap<PDX::country>& countriesArray) {
	std::string owner = "ZZZ";

	std::regex ownerRegex("owner\\s*=\\s*(\\w+)");
	std::smatch match;
	if (regex_search(historyContent, match, ownerRegex)) owner = match[1];
	historyContent = regex_replace(historyContent, ownerRegex, "");
	std::transform(owner.begin(), owner.end(), owner.begin(), [](unsigned char c) { return std::toupper(c); });

	PDX::country* countryPointer = nullptr;

	auto vecIndex = countriesArray.hashMap.find(owner);
	if (vecIndex != countriesArray.hashMap.end()) countryPointer = &countriesArray.vector[vecIndex->second];

	return countryPointer;
}

static PDX::state_category* returnStateCategory(std::string& fileContent, PDX::vectorStringIndexMap<PDX::state_category>& stateCategoriesArray) {
	std::string state_category = "NULL";

	std::regex stateCategoryRegex("state_category\\s*=\\s*(\\w+)");
	std::smatch match;
	if (regex_search(fileContent, match, stateCategoryRegex)) state_category = match[1];
	fileContent = regex_replace(fileContent, stateCategoryRegex, "");


	PDX::state_category* stateCategoryPointer = nullptr;

	auto vecIndex = stateCategoriesArray.hashMap.find(state_category);
	if (vecIndex != stateCategoriesArray.hashMap.end()) stateCategoryPointer = &stateCategoriesArray.vector[vecIndex->second];

	return stateCategoryPointer;
}

static int32_t returnStateManpower(std::string& fileContent) {
	int32_t manpower = 0;

	std::regex manpowerRegex("manpower\\s*=\\s*(\\d+)");
	std::smatch match;
	if (regex_search(fileContent, match, manpowerRegex)) manpower = stoi(match[1]);
	fileContent = regex_replace(fileContent, manpowerRegex, "");

	return manpower;
}

static bool returnStateImpassable(std::string& fileContent) {
	bool impassable = false;

	std::regex impassableRegex("impassable\\s*=\\s*yes");
	impassable = regex_search(fileContent, impassableRegex);
	fileContent = regex_replace(fileContent, impassableRegex, "");

	return impassable;
}

static std::vector <PDX::province*> returnStateProvinces(std::string& provinceContent, std::vector<PDX::province>& provincesArray) {
	std::vector<uint16_t> provinces;
	std::vector<PDX::province*> provincesPtr;

	std::stringstream iss(provinceContent);
	int provinceInt;
	while (iss >> provinceInt) {
		provinces.push_back(provinceInt);
	}

	provincesPtr.reserve(provinces.size());

	for (auto& province : provinces) {
		provincesPtr.emplace_back(&provincesArray[province]);
	}

	return provincesPtr;
}

static std::vector <uint16_t> returnStateResources(std::string& fileContent, PDX::vectorStringIndexMap<PDX::resource>& resourcesArray) {
	std::vector<uint16_t> resources(resourcesArray.vector.size(), 0);

	std::regex nameEqualsValueRegex("(\\w+)\\s*=\\s*(\\d+)");

	std::string resourcesString = returnStringBetweenBrackets(fileContent, "resources");
	auto BEGIN = std::sregex_iterator(resourcesString.begin(), resourcesString.end(), nameEqualsValueRegex);
	auto END = std::sregex_iterator();
	for (std::sregex_iterator i = BEGIN; i != END; ++i) {
		std::smatch resoursesMatch = *i;
		std::string resourceName = resoursesMatch[1].str();

		auto vecIndex = resourcesArray.hashMap.find(resourceName);
		if (vecIndex != resourcesArray.hashMap.end()) resources[vecIndex->second] = stoi(resoursesMatch[2]);
	}
	fileContent = removeStringBetweenBrackets(fileContent, "resources");
	return resources;
}

static std::vector <PDX::country*> returnStateCores(std::string& historyContent, PDX::vectorStringIndexMap<PDX::country>& countriesArray) {
	std::vector<PDX::country*> cores;

	std::regex coreRegex("add_core_of\\s*=\\s*(\\w{3})");

	auto BEGIN = std::sregex_iterator(historyContent.begin(), historyContent.end(), coreRegex);
	auto END = std::sregex_iterator();
	for (std::sregex_iterator i = BEGIN; i != END; ++i) {
		std::smatch match = *i;
		std::string countryTagStr = match[1].str();

		auto vecIndex = countriesArray.hashMap.find(countryTagStr);
		if (vecIndex != countriesArray.hashMap.end()) cores.push_back(&countriesArray.vector[vecIndex->second]);
	}
	historyContent = regex_replace(historyContent, coreRegex, "");
	return cores;
}

static std::vector <PDX::country*> returnStateClaims(std::string& historyContent, PDX::vectorStringIndexMap<PDX::country>& countriesArray) {
	std::vector<PDX::country*> claims;

	std::regex claimRegex("add_claim_by\\s*=\\s*(\\w{3})");

	auto BEGIN = std::sregex_iterator(historyContent.begin(), historyContent.end(), claimRegex);
	auto END = std::sregex_iterator();
	for (std::sregex_iterator i = BEGIN; i != END; ++i) {
		std::smatch match = *i;
		std::string countryTagStr = match[1].str();

		auto vecIndex = countriesArray.hashMap.find(countryTagStr);
		if (vecIndex != countriesArray.hashMap.end()) claims.push_back(&countriesArray.vector[vecIndex->second]);
	}
	historyContent = regex_replace(historyContent, claimRegex, "");
	return claims;
}

static std::vector <PDX::flag> returnStateFlags(std::string& historyContent) {
	std::vector<PDX::flag> flagsArray;

	std::regex basicFlagRegex("set_(\\w+)_flag\\s*=\\s*(\\w+)");
	auto BEGIN = std::sregex_iterator(historyContent.begin(), historyContent.end(), basicFlagRegex);
	auto END = std::sregex_iterator();
	for (std::sregex_iterator i = BEGIN; i != END; ++i) {
		std::smatch match = *i;

		std::string name = match[2].str();
		int days = -1;
		int value = 1;
		uint8_t type = PDX::flagTypeGlobal;

		std::string flagType = match[1].str();
		if (flagType == "country") type = PDX::flagTypeCountry;
		else if (flagType == "state") type = PDX::flagTypeState;
		else if (flagType == "character") type = PDX::flagTypeCharacter;
		else if (flagType == "mio") type = PDX::flagTypeMIO;

		flagsArray.emplace_back(name, value, days, type);
	}
	historyContent = regex_replace(historyContent, basicFlagRegex, "");



	std::regex flagBracketRegex(R"(set_(\w+)_flag\s*=\s*\{)");
	std::smatch match2;

	while (regex_search(historyContent, match2, flagBracketRegex)) {
		std::string flagType = match2[1].str();
		std::string flagStr = "set_" + flagType + "_flag";

		std::string name = "";
		int days = -1;
		int value = 1;
		uint8_t type = PDX::flagTypeGlobal;

		if (flagType == "country") type = PDX::flagTypeCountry;
		else if (flagType == "state") type = PDX::flagTypeState;
		else if (flagType == "character") type = PDX::flagTypeCharacter;
		else if (flagType == "mio") type = PDX::flagTypeMIO;

		std::string flagData = returnStringBetweenBrackets(historyContent, flagStr);

		std::regex flagRegex("flag\\s*=\\s*(\\w+)");
		std::regex daysRegex("days\\s*=\\s*(\\d+)");
		std::regex valueRegex("value\\s*=\\s*(\\d+)");

		std::smatch matchFlag;
		std::smatch matchDays;
		std::smatch matchValue;


		if (regex_search(flagData, matchFlag, flagRegex)) name = matchFlag[1].str();
		if (regex_search(flagData, matchDays, daysRegex)) days = stoi(matchDays[1].str());
		if (regex_search(flagData, matchValue, valueRegex)) value = stoi(matchValue[1].str());

		flagsArray.emplace_back(name, value, days, type);
		historyContent = removeStringBetweenBrackets(historyContent, flagStr);
	}

	return flagsArray;
}

static std::vector <PDX::variable> returnStateVariables(std::string& historyContent) {
	std::vector<PDX::variable> variablesArray;

	std::regex basicVariableRegex(R"(set_variable\s*=\s*\{\s*(.+)\s*=\s*(.+)\s*\})");
	auto BEGIN = std::sregex_iterator(historyContent.begin(), historyContent.end(), basicVariableRegex);
	auto END = std::sregex_iterator();
	for (std::sregex_iterator i = BEGIN; i != END; ++i) {
		std::smatch match = *i;

		std::string name = match[1].str();
		std::string value = match[2].str();
		
		variablesArray.emplace_back(name, value);
	}
	historyContent = regex_replace(historyContent, basicVariableRegex, "");



	std::regex variableRegex(R"(set_variable\s*=\s*\{)");
	std::smatch match2;

	while (regex_search(historyContent, match2, variableRegex)) {
		std::string variableData = returnStringBetweenBrackets(historyContent, "set_variable");

		std::string name, value, tooltip = "";

		std::regex nameRegex("var\\s*=\\s*(\\w+)");
		std::regex valueRegex("value\\s*=\\s*(\\w+)");
		std::regex tooltipRegex("tooltip\\s*=\\s*(\\w+)");

		std::smatch matchName;
		std::smatch matchValue;
		std::smatch matchTooltip;


		if (regex_search(variableData, matchName, nameRegex)) value = matchName[1].str();
		if (regex_search(variableData, matchValue, valueRegex)) value = matchValue[1].str();
		if (regex_search(variableData, matchTooltip, tooltipRegex)) tooltip = matchTooltip[1].str();

		variablesArray.emplace_back(name, value, tooltip);
		historyContent = removeStringBetweenBrackets(historyContent, "set_variable");
	}

	return variablesArray;
}

static std::vector <uint8_t> loadStateBuildings(std::string& buildingsContent, std::vector <std::vector <uint16_t> >& provinceBuildings,
	PDX::vectorStringIndexMap<PDX::building>& stateBuildingsArray, PDX::vectorStringIndexMap<PDX::building>& provinceBuildingsArray) {

	std::regex provinceBuildingRegex(R"((\d+)\s*=\s*\{)");
	std::regex buildingValueRegex(R"((\w+)\s*=\s*(\d+))");

	std::smatch match;
	while (regex_search(buildingsContent, match, provinceBuildingRegex)) {
		std::string provID = match[1].str();

		std::string provinceBuildingData = returnStringBetweenBrackets(buildingsContent, provID);

		std::vector<uint16_t> v(provinceBuildingsArray.vector.size() + 1, 0);
		v[0] = stoi(provID);

		auto BEGIN = std::sregex_iterator(provinceBuildingData.begin(), provinceBuildingData.end(), buildingValueRegex);
		auto END = std::sregex_iterator();
		for (std::sregex_iterator i = BEGIN; i != END; ++i) {
			std::smatch match2 = *i;

			std::string buildingName = match2[1].str();
			uint8_t buildingAmount = stoi(match2[2].str());

			auto vecIndex = provinceBuildingsArray.hashMap.find(buildingName);
			if (vecIndex != provinceBuildingsArray.hashMap.end()) {			
				unsigned int index = (vecIndex->second) + 1;
				v[index] = buildingAmount;
			}
		}
		provinceBuildings.push_back(v);
		buildingsContent = removeStringBetweenBrackets(buildingsContent, provID);
	}
	
//	std::cout << buildingsContent << "\n";

	std::vector <uint8_t> buildingsVector(stateBuildingsArray.vector.size(), 0);

	auto BEGIN = std::sregex_iterator(buildingsContent.begin(), buildingsContent.end(), buildingValueRegex);
	auto END = std::sregex_iterator();
	for (std::sregex_iterator i = BEGIN; i != END; ++i) {
		std::smatch match2 = *i;

		std::string buildingName = match2[1].str();
		uint8_t buildingAmount = stoi(match2[2].str());

		auto vecIndex = stateBuildingsArray.hashMap.find(buildingName);
		if (vecIndex != stateBuildingsArray.hashMap.end()) buildingsVector[vecIndex->second] = buildingAmount;
		
	}

	return buildingsVector;
}

static void loadState(
	const std::filesystem::path& stateFile,

	PDX::vectorStringIndexMap<PDX::terrain>& terrainArray,
	PDX::vectorStringIndexMap<PDX::building>& stateBuildingsArray,
	PDX::vectorStringIndexMap<PDX::building>& provinceBuildingsArray,
	PDX::vectorStringIndexMap<PDX::resource>& resourcesArray,
	PDX::vectorStringIndexMap<PDX::state_category>& stateCategoryArray,
	PDX::vectorStringIndexMap<PDX::country>& countriesArray,
	std::vector<PDX::province>& provincesArray,
	std::vector<PDX::state>& statesArray,

	std::mutex& mtx
) {
	std::string fileContent = returnTXTFileAsStringNoHashes(stateFile);

	std::string historyContent = returnStringBetweenBrackets(fileContent, "history");
	fileContent = removeStringBetweenBrackets(fileContent, "history");
	std::string buildingContent = returnStringBetweenBrackets(historyContent, "buildings");
	historyContent = removeStringBetweenBrackets(historyContent, "buildings");
	std::string provinceContent = returnStringBetweenBrackets(fileContent, "provinces");
	fileContent = removeStringBetweenBrackets(fileContent, "provinces");

	uint16_t id = returnStateID(fileContent);
	std::vector <std::string> dates = returnStateDates(historyContent);
	PDX::country* owner = returnStateOwner(historyContent, countriesArray);
	PDX::state_category* state_category = returnStateCategory(fileContent, stateCategoryArray);
	int32_t manpower = returnStateManpower(fileContent);
	bool impassable = returnStateImpassable(fileContent);
	std::vector <PDX::province*> provinces = returnStateProvinces(provinceContent, provincesArray);
	std::vector <uint16_t> resources = returnStateResources(fileContent, resourcesArray);
	std::vector <PDX::country*> cores = returnStateCores(historyContent, countriesArray);
	std::vector <PDX::country*> claims = returnStateClaims(historyContent, countriesArray);
	std::vector <PDX::flag> flags = returnStateFlags(historyContent);
	std::vector <PDX::variable> variables = returnStateVariables(historyContent);
	std::vector <std::vector <uint16_t> > provinceBuildings; provinceBuildings.reserve(provinces.size() / 2);
	std::vector <uint8_t> buildings = loadStateBuildings(buildingContent, provinceBuildings, stateBuildingsArray, provinceBuildingsArray);

	std::lock_guard<std::mutex> lock(mtx);
	statesArray.emplace_back(id, impassable, manpower, owner, state_category, provinces, resources, dates, cores, claims, flags, variables, buildings);
	for (const auto& buildingsAndIdVec : provinceBuildings) {
		uint16_t id = buildingsAndIdVec[0];
		for (size_t i = 0; i < provincesArray[id].buildings.size(); ++i) provincesArray[id].buildings[i] += buildingsAndIdVec[i + 1];
	}
}

static void loadStrategicRegion(
	const std::filesystem::path& strategicRegionFile,

	std::vector<PDX::province>& provincesArray,
	std::vector<PDX::strategic_region>& strategicRegionsArray,

	std::mutex& mtx2
) {
	std::string fileContent = returnTXTFileAsStringNoHashes(strategicRegionFile);

	std::string provinceContent = returnStringBetweenBrackets(fileContent, "provinces");
	fileContent = removeStringBetweenBrackets(fileContent, "provinces");
	std::string weatherContent = returnStringBetweenBrackets(fileContent, "weather");
	fileContent = removeStringBetweenBrackets(fileContent, "weather");

	uint16_t id = returnStateID(fileContent);
	std::vector <PDX::province*> provinces = returnStateProvinces(provinceContent, provincesArray);

	std::vector<PDX::weather_period> weather;
	weather.reserve(12);

	std::regex weatherPeriodRegex(R"(period\s*=\s*\{)");

	std::smatch match;
	while (regex_search(weatherContent, match, weatherPeriodRegex)){
		std::string periodStr = returnStringBetweenBrackets(weatherContent, "period");

		std::regex betweenRegex(R"(between\s*=\s*\{\s*([0-9_+-\.]+) ([0-9_+-\.]+)\s*\})");
		std::regex temperatureRegex(R"(temperature\s*=\s*\{\s*([0-9_+-\.]+) ([0-9_+-\.]+)\s*\})");
		std::regex noPhenomenonRegex(R"(no_phenomenon\s*=\s*([0-9_+-\.]+))");
		std::regex rainLightRegex(R"(rain_light\s*=\s*([0-9_+-\.]+))");
		std::regex rainHeavyRegex(R"(rain_heavy\s*=\s*([0-9_+-\.]+))");
		std::regex snowRegex(R"(snow\s*=\s*([0-9_+-\.]+))");
		std::regex blizzardRegex(R"(blizzard\s*=\s*([0-9_+-\.]+))");
		std::regex arcticWaterRegex(R"(arctic_Water\s*=\s*([0-9_+-\.]+))");
		std::regex mudRegex(R"(mud\s*=\s*([0-9_+-\.]+))");
		std::regex sandstormRegex(R"(sandstorm\s*=\s*([0-9_+-\.]+))");
		std::regex minSnowLevelRegex(R"(min_snow_level\s*=\s*([0-9_+-\.]+))");

		std::smatch matchBetween, matchTemperature, matchNoPhenomenon, matchRainLight, matchRainHeavy, matchSnow, matchBlizzard, matchArcticWater, matchMud, matchSandstorm, matchMinSnowLevel;

		double betweenL{}, betweenR{}, temperatureL{}, temperatureR{}, no_phenomenon{}, rain_light{}, rain_heavy{}, snow{},
			blizzard{}, arctic_water{}, mud{}, sandstorm{}, min_snow_level{};

		if (regex_search(periodStr, matchBetween, betweenRegex)) {
			betweenL = stod(matchBetween[1].str());
			betweenR = stod(matchBetween[2].str());
//			regex_replace(periodStr, betweenRegex, "");
		}
		if (regex_search(periodStr, matchTemperature, temperatureRegex)) {
			temperatureL = stod(matchTemperature[1].str());
			temperatureR = stod(matchTemperature[2].str());
//			regex_replace(periodStr, temperatureRegex, "");
		}
		if (regex_search(periodStr, matchNoPhenomenon, noPhenomenonRegex)) {
			no_phenomenon = stod(matchNoPhenomenon[1].str());
//			regex_replace(periodStr, noPhenomenonRegex, "");
		}
		if (regex_search(periodStr, matchRainLight, rainLightRegex)) {
			rain_light = stod(matchRainLight[1].str());
//			regex_replace(periodStr, rainLightRegex, "");
		}
		if (regex_search(periodStr, matchRainHeavy, rainHeavyRegex)) {
			rain_heavy = stod(matchRainHeavy[1].str());
//			regex_replace(periodStr, rainHeavyRegex, "");
		}
		if (regex_search(periodStr, matchSnow, snowRegex)) {
			snow = stod(matchSnow[1].str());
//			regex_replace(periodStr, snowRegex, "");
		}
		if (regex_search(periodStr, matchBlizzard, blizzardRegex)) {
			blizzard = stod(matchBlizzard[1].str());
//			regex_replace(periodStr, blizzardRegex, "");
		}
		if (regex_search(periodStr, matchArcticWater, arcticWaterRegex)) {
			arctic_water = stod(matchArcticWater[1].str());
//			regex_replace(periodStr, arcticWaterRegex, "");
		}
		if (regex_search(periodStr, matchMud, mudRegex)) {
			mud = stod(matchMud[1].str());
//			regex_replace(periodStr, mudRegex, "");
		}
		if (regex_search(periodStr, matchSandstorm, sandstormRegex)) {
			sandstorm = stod(matchSandstorm[1].str());
//			regex_replace(periodStr, sandstormRegex, "");
		}
		if (regex_search(periodStr, matchMinSnowLevel, minSnowLevelRegex)) { 
			min_snow_level = stod(matchMinSnowLevel[1].str()); 
//			regex_replace(periodStr, minSnowLevelRegex, "");
		}

		weather.emplace_back(betweenL, betweenR, temperatureL, temperatureR, no_phenomenon, rain_light, rain_heavy, snow, blizzard, arctic_water, mud, sandstorm, min_snow_level);
		weatherContent = removeStringBetweenBrackets(weatherContent, "period");
	}

	std::lock_guard<std::mutex> lock(mtx2);
	strategicRegionsArray.emplace_back(id, provinces, weather);
}

static void returnLocalisationFiles(std::string folder, std::filesystem::path vanillaGamePath, std::filesystem::path modPath, std::vector<std::filesystem::path>& locFiles, std::vector<std::filesystem::path>& replaceFiles) {
	locFiles.reserve(256);
	bool replacePathBool = modFileReplacesFolder(modPath, folder);

	std::regex forwardSlashRegex("/");
	folder = regex_replace(folder, forwardSlashRegex, "\\");

	vanillaGamePath = vanillaGamePath / folder;
	modPath = modPath / folder;

	//    std::cout << vanillaGamePath << "\n" << modPath << "\n";


	if (replacePathBool && std::filesystem::exists(modPath) && std::filesystem::is_directory(modPath)) {
		for (const auto& file : std::filesystem::recursive_directory_iterator(modPath)) {
			std::filesystem::path relativePath = std::filesystem::relative(file.path(), modPath);
			std::string checkReplace = relativePath.string().substr(0, 8);
			if (file.is_regular_file() && file.path().extension() == ".yml" && checkReplace != "replace\\") {
				locFiles.emplace_back(file.path());
			}
			else if(checkReplace == "replace\\") replaceFiles.emplace_back(file.path());
		}
	}
	else if (!replacePathBool && std::filesystem::exists(modPath) && std::filesystem::is_directory(modPath)) {
		for (const auto& file : std::filesystem::recursive_directory_iterator(vanillaGamePath)) {
			if (file.is_regular_file() && file.path().extension() == ".yml") {

				std::string pathStr = file.path().string();
				std::istringstream ss(pathStr);
				std::string token;
				std::vector<std::string> tokens;

				while (getline(ss, token, '\\')) { tokens.push_back(token); }

				int tokenSize = tokens.size();
				tokenSize--;
				std::filesystem::path fileToCheckFor = modPath / tokens[tokenSize];
				if (!std::filesystem::exists(fileToCheckFor)) {
					locFiles.emplace_back(file.path());
				}
			}
		}

		for (const auto& file : std::filesystem::recursive_directory_iterator(modPath)) {
			std::filesystem::path relativePath = std::filesystem::relative(file.path(), modPath);
			std::string checkReplace = relativePath.string().substr(0, 8);
			if (file.is_regular_file() && file.path().extension() == ".yml" && checkReplace != "replace\\") {
				locFiles.emplace_back(file.path());
			}
			else if (checkReplace == "replace\\") replaceFiles.emplace_back(file.path());
		}
	}
	else {
		for (const auto& file : std::filesystem::recursive_directory_iterator(vanillaGamePath)) {
			if (file.is_regular_file() && file.path().extension() == ".yml") {
				locFiles.emplace_back(file.path());
			}
		}
	}
}

static void loadLocalisation(const std::filesystem::path& file, std::vector<PDX::province>& provincesArray, std::vector<PDX::state>& statesArray, std::vector<PDX::strategic_region>& strategicRegionsArray) {
	std::regex whiteSpaceRegex("(\\s+)");
	std::regex provinceNameRegex("VICTORY_POINTS_(\\d+)");
	std::regex stateNameRegex("STATE_(\\d+)");
	std::regex StrategicRegionNameRegex("STRATEGIC_REGION_(\\d+)");

	std::smatch provinceMatch;
	std::smatch stateMatch;
	std::smatch strategicRegionMatch;

	std::ifstream fileFStream(file);
	std::string line;
	int number_of_lines = 0;
	while (std::getline(fileFStream, line)) {
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos) {
			std::string locKey = line.substr(0, colonPos);
			std::string locValue = line.substr(colonPos + 1);

			if (!locValue.empty()) {
				size_t firstPos = locValue.find('\"');
				size_t lastPos = locValue.rfind('\"');
				if (firstPos != lastPos && firstPos != std::string::npos && lastPos != std::string::npos) {
					locValue = locValue.substr(firstPos + 1, lastPos - 1);
					locKey = regex_replace(locKey, whiteSpaceRegex, "");
					uint16_t id = -1;

					if (regex_search(locKey, provinceMatch, provinceNameRegex)) {
						id = stoi(provinceMatch[1].str());
						provincesArray[id].name = locValue;
					}
					else if (regex_search(locKey, stateMatch, stateNameRegex)) {
						id = stoi(stateMatch[1].str());
						statesArray[id].name = locValue;
					}
					else if (regex_search(locKey, strategicRegionMatch, StrategicRegionNameRegex)) {
						id = stoi(strategicRegionMatch[1].str());
						strategicRegionsArray[id].name = locValue;
					}
				}
			}
		}
	}
}

#include <chrono>
void loadMap(
	const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath,

	PDX::vectorStringIndexMap<PDX::terrain>& terrainArray,
	PDX::vectorStringIndexMap<PDX::building>& stateBuildingsArray,
	PDX::vectorStringIndexMap<PDX::building>& provinceBuildingsArray,
	PDX::vectorStringIndexMap<PDX::resource>& resourcesArray,
	PDX::vectorStringIndexMap<PDX::state_category>& stateCategoryArray,
	PDX::vectorStringIndexMap<PDX::country>& countriesArray,
	std::vector<PDX::province>& provincesArray,
	std::vector<PDX::state>& statesArray,
	std::vector<PDX::strategic_region>& strategicRegionsArray
) {
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::milliseconds ms;
	typedef std::chrono::duration<float> fsec;
	auto timeStart = Time::now();

	std::vector<std::filesystem::path> replaceFiles;
	std::vector<std::filesystem::path> locFiles;

	std::thread t1(returnLocalisationFiles, "localisation/english", std::cref(vanillaGamePath), std::cref(modPath), std::ref(locFiles), std::ref(replaceFiles));

	std::thread t2(loadTerrain, std::cref(vanillaGamePath), std::cref(modPath), std::ref(terrainArray));
	std::thread t3(loadBuildings, std::cref(vanillaGamePath), std::cref(modPath), std::ref(stateBuildingsArray), std::ref(provinceBuildingsArray));
	std::thread t4(loadResources, std::cref(vanillaGamePath), std::cref(modPath), std::ref(resourcesArray));
	std::thread t5(loadStateCategories, std::cref(vanillaGamePath), std::cref(modPath), std::ref(stateCategoryArray));
	std::thread t6(loadCountries, std::cref(vanillaGamePath), std::cref(modPath), std::ref(countriesArray));

	t2.join();
	t3.join();
	std::thread t7(loadProvinces, std::cref(vanillaGamePath), std::cref(modPath), std::ref(provincesArray), std::ref(terrainArray), std::ref(provinceBuildingsArray));
	
	t1.join();
	t4.join();
	t5.join();
	t6.join();
	t7.join();
	std::sort(provincesArray.begin(), provincesArray.end(), [](const PDX::province& a, const PDX::province& b) { return a.id < b.id; });

	auto timeMiddle1 = Time::now();
	fsec fs = timeMiddle1 - timeStart;
	ms startMS = std::chrono::duration_cast<ms>(fs);
	std::cout << startMS.count() << "ms\n";

	std::vector<std::filesystem::path> stateFiles = findFilesToLoad("history/states", vanillaGamePath, modPath);
	statesArray.reserve(stateFiles.size() + 1);
	statesArray.emplace_back();
	std::mutex mtx;
	std::vector<std::future<void>> futuresStates;

	for (const auto& state : stateFiles) {
		futuresStates.push_back(std::async(std::launch::async, loadState, state, std::ref(terrainArray), std::ref(stateBuildingsArray), std::ref(provinceBuildingsArray), std::ref(resourcesArray),
			std::ref(stateCategoryArray), std::ref(countriesArray), std::ref(provincesArray), std::ref(statesArray), std::ref(mtx)));
	}

	for (auto& fut : futuresStates) fut.get();

	std::vector<std::filesystem::path> strategicregionFiles = findFilesToLoad("map/strategicregions", vanillaGamePath, modPath);
	strategicRegionsArray.reserve(strategicregionFiles.size() + 1);
	strategicRegionsArray.emplace_back();
	std::mutex mtx2;
	std::vector<std::future<void>> futuresStrategicRegions;

	for (const auto& strategicRegion : strategicregionFiles) {
		futuresStrategicRegions.push_back(std::async(std::launch::async, loadStrategicRegion, strategicRegion, std::ref(provincesArray), std::ref(strategicRegionsArray), std::ref(mtx2)));
	}

	for (auto& fut : futuresStrategicRegions) fut.get();

	std::sort(statesArray.begin(), statesArray.end(), [](const PDX::state& a, const PDX::state& b) { return a.id < b.id; });
	std::sort(strategicRegionsArray.begin(), strategicRegionsArray.end(), [](const PDX::strategic_region& a, const PDX::strategic_region& b) { return a.id < b.id; });

	for (auto& state : statesArray) {
		if (state.id != 0) {
			state.owner->states.emplace_back(&state);

			for (auto& province : state.provinces) province->state = &state;
		}
		std::sort(state.provinces.begin(), state.provinces.end(), [](const PDX::province* a, const PDX::province* b) { return a->id < b->id; });
	}

	for (auto& strategicRegion : strategicRegionsArray) {
		if (strategicRegion.id != 0) {

			for (auto& province : strategicRegion.provinces) province->strategic_region = &strategicRegion;
		}
		std::sort(strategicRegion.provinces.begin(), strategicRegion.provinces.end(), [](const PDX::province* a, const PDX::province* b) { return a->id < b->id; });
	}

	auto timeMiddle2 = Time::now();
	fsec fs2 = timeMiddle2 - timeStart;
	ms middleMS = std::chrono::duration_cast<ms>(fs2);
	std::cout << middleMS.count() << "ms\n";

	for (const auto& file : locFiles) loadLocalisation(file, provincesArray, statesArray, strategicRegionsArray);
	for (const auto& file : replaceFiles) loadLocalisation(file, provincesArray, statesArray, strategicRegionsArray);

	auto timeEnd = Time::now();
	fsec fs3 = timeEnd - timeStart;
	ms endMS = std::chrono::duration_cast<ms>(fs3);
	std::cout << endMS.count() << "ms\n";
}