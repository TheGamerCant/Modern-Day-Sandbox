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

static void loadProvinces(const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath, std::vector<PDX::province>& provincesArray, PDX::vectorStringIndexMap<PDX::terrain>& terrainArray) {
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

		PDX::terrain* terrainPointer = nullptr;
		auto terrainIndex = terrainArray.hashMap.find(terrainStr);
		if (terrainIndex != terrainArray.hashMap.end()) terrainPointer = &terrainArray.vector[terrainIndex->second];

		provincesArray.emplace_back(id, red, green, blue, type, coastal, continent, terrainPointer);
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

	std::regex basicVariableRegex(R"(set_variable\s*=\s*\{\s*(.+)\s*=\s*(.+))");
	auto BEGIN = std::sregex_iterator(historyContent.begin(), historyContent.end(), basicVariableRegex);
	auto END = std::sregex_iterator();
	for (std::sregex_iterator i = BEGIN; i != END; ++i) {
		std::smatch match = *i;

		std::string name = match[1].str();
		std::string value = match[2].str();
		
		variablesArray.emplace_back(name, value, "");
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

static void loadState(
	const std::filesystem::path& stateFile,

	PDX::vectorStringIndexMap<PDX::terrain>& terrainArray,
	PDX::vectorStringIndexMap<PDX::building>& buildingArray,
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

	PDX::state T_state;
	T_state.id = id;
	T_state.dates = dates;
	T_state.owner = owner;
	T_state.state_category = state_category;
	T_state.manpower = manpower;
	T_state.impassable = impassable;
	T_state.provinces = provinces;
	T_state.resources = resources;
	T_state.cores = cores;
	T_state.claims = claims;
	T_state.flags = flags;
	T_state.variables = variables;
	

	std::lock_guard<std::mutex> lock(mtx);
	statesArray.push_back(T_state);
}


#include <chrono>
void loadMap(
	const std::filesystem::path& vanillaGamePath, const std::filesystem::path& modPath,

	PDX::vectorStringIndexMap<PDX::terrain>& terrainArray,
	PDX::vectorStringIndexMap<PDX::building>& buildingArray,
	PDX::vectorStringIndexMap<PDX::resource>& resourcesArray,
	PDX::vectorStringIndexMap<PDX::state_category>& stateCategoryArray,
	PDX::vectorStringIndexMap<PDX::country>& countriesArray,
	std::vector<PDX::province>& provincesArray,
	std::vector<PDX::state>& statesArray
) {
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::milliseconds ms;
	typedef std::chrono::duration<float> fsec;
	auto timeStart = Time::now();

	std::thread t1(loadTerrain, std::cref(vanillaGamePath), std::cref(modPath), std::ref(terrainArray));
	std::thread t2(loadBuildings, std::cref(vanillaGamePath), std::cref(modPath), std::ref(buildingArray));
	std::thread t3(loadResources, std::cref(vanillaGamePath), std::cref(modPath), std::ref(resourcesArray));
	std::thread t4(loadStateCategories, std::cref(vanillaGamePath), std::cref(modPath), std::ref(stateCategoryArray));
	std::thread t5(loadCountries, std::cref(vanillaGamePath), std::cref(modPath), std::ref(countriesArray));

	t1.join();
	std::thread t6(loadProvinces, std::cref(vanillaGamePath), std::cref(modPath), std::ref(provincesArray), std::ref(terrainArray));

	t2.join();
	t3.join();
	t4.join();
	t5.join();
	t6.join();

	auto timeMiddle = Time::now();
	fsec fs = timeMiddle - timeStart;
	ms middleMS = std::chrono::duration_cast<ms>(fs);
	std::cout << middleMS.count() << "ms\n";

	std::vector<std::filesystem::path> stateFiles = findFilesToLoad("history/states", vanillaGamePath, modPath);
	statesArray.reserve(stateFiles.size() + 1);
	statesArray.emplace_back();
	std::mutex mtx;
	std::vector<std::future<void>> futures;

	for (const auto& state : stateFiles) {
		futures.push_back(std::async(std::launch::async, loadState, state, std::ref(terrainArray), std::ref(buildingArray), std::ref(resourcesArray),
			std::ref(stateCategoryArray), std::ref(countriesArray), std::ref(provincesArray), std::ref(statesArray), std::ref(mtx)));
	}

	for (auto& fut : futures) fut.get();

	std::sort(provincesArray.begin(), provincesArray.end(), [](const PDX::province& a, const PDX::province& b) { return a.id < b.id; });
	std::sort(statesArray.begin(), statesArray.end(), [](const PDX::state& a, const PDX::state& b) { return a.id < b.id; });

	for (auto& state : statesArray) {
		if (state.id != 0) {
			state.owner->states.emplace_back(&state);

			for (auto& province : state.provinces) province->state = &state;
		}
	}

	auto timeEnd = Time::now();
	fsec fs2 = timeEnd - timeMiddle;
	ms endMS = std::chrono::duration_cast<ms>(fs2);
	std::cout << endMS.count() << "ms\n";
}