// g++ src/*.cpp -std=c++20 -static -O3 -o name_writer.exe
// g++ src/*.cpp -std=c++20 -O3 -o name_writer_mac

#include <fstream>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <regex>
#include <sstream>

#include "data_types.hpp"
#include "functions.hpp"

#include "json.hpp"
using json = nlohmann::json;

void LoadStateProvinceList(const Path& modDirectory, HashMap<SignedInteger64, State>& states) {
	Vector<Path> stateFiles = GetGameFiles(modDirectory, modDirectory, {"history/states"}, "history/states", ".txt");

	for (const auto& file : stateFiles) {
	    PdxJson stateFile = ParseFileToPdxJson(file.string());

        if (!stateFile.contains("state")) { continue; }

        for (const auto& state : stateFile.at("state").asList()) {
            if (
                state.contains("id") &&
                state.contains("provinces") &&
                state.at("id")[0].isInt() &&
                state.at("provinces")[0].isList() &&
                state["provinces"][0][0].isInt()
                ) {

                //Use ID as key and use id & vector as constructor
                SignedInteger64 stateId = state["id"][0].as<SignedInteger64>();
                states[stateId] = State(stateId, state["provinces"][0].asVector<SignedInteger64>());
            }
        }
	}
}

void LoadNamesFromJson(const String& jsonPath, HashMap<SignedInteger64, Province>& provinces, HashMap<SignedInteger64, State>& states, String& modPrefix) {
    std::ifstream file(jsonPath);
    json namesJson = json::parse(file);

    if (namesJson.contains("prefix") || namesJson["prefix"].is_string()) {
	    modPrefix = namesJson.at("prefix");

	    if (!modPrefix.ends_with("_")) {
	        modPrefix = modPrefix + "_";
	    }
	}

	if (!namesJson.contains("state_names") || !namesJson["state_names"].is_array()) {
	    FatalError("Missing or invalid 'state_names' in " + jsonPath);
	}
	else if (!namesJson.contains("victory_point_names") || !namesJson["victory_point_names"].is_array()) {
	    FatalError("Missing or invalid 'victory_point_names' in " + jsonPath);
	}
	
	const json& stateNames = namesJson["state_names"];
	const json& vpNames = namesJson["victory_point_names"];

    for (const auto& stateEntry : stateNames) {
        SignedInteger64 objectId = 0;
        String objectDefaultName = "";
        Vector<NameEntry> objectCustomNames {};

        if (!stateEntry.is_object()) {
            std::cout << "Invalid object in state_names, skipping entry.\n";
            continue;
        }

        if (!stateEntry.contains("id") || !stateEntry.at("id").is_number_integer()) {
            std::cout << "State does not have a valid state ID, skipping entry.\n";
            continue;
        }

        objectId = stateEntry.at("id");

        if (!states.contains(objectId)) {
            std::cout << objectId << " is not a valid state, skipping entry.\n";
            continue;
        }

        if (!stateEntry.contains("default_name") || !stateEntry.at("default_name").is_string()) {
            std::cout << "State " << stateEntry.at("id") << " does not have a valid default name, skipping entry.\n";
        }

        objectDefaultName = stateEntry.at("default_name");

        if (stateEntry.contains("custom_names") && stateEntry.at("custom_names").is_array()) {
            for (const auto& nameEntry : stateEntry.at("custom_names")) {
                if (!nameEntry.is_object() ||
                    !nameEntry.contains("requirements") || !nameEntry.at("requirements").is_string() ||
                    !nameEntry.contains("name") || !nameEntry.at("name").is_string()) {
                    std::cout << "State " << stateEntry.at("id") << " has a bad custom name entry, skipping this name.\n";
                    continue;
                }

                objectCustomNames.emplace_back(nameEntry.at("name"), nameEntry.at("requirements"));
            }
        }

        states[objectId].defaultName = objectDefaultName;
        states[objectId].customNames = objectCustomNames;
    }

    for (const auto& victoryPointEntry : vpNames) {
        SignedInteger64 objectId = 0;
        String objectDefaultName = "";
        Vector<NameEntry> objectCustomNames {};

        if (!victoryPointEntry.is_object()) {
            std::cout << "Invalid object in victory_point_names, skipping entry.\n";
            continue;
        }

        if (!victoryPointEntry.contains("id") || !victoryPointEntry.at("id").is_number_integer()) {
            std::cout << "Province does not have a valid provinces ID, skipping entry.\n";
            continue;
        }

        objectId = victoryPointEntry.at("id");

        if (!victoryPointEntry.contains("default_name") || !victoryPointEntry.at("default_name").is_string()) {
            std::cout << "Province " << victoryPointEntry.at("id") << " does not have a valid default name, skipping entry.\n";
        }

        objectDefaultName = victoryPointEntry.at("default_name");

        if (victoryPointEntry.contains("custom_names") && victoryPointEntry.at("custom_names").is_array()) {
            for (const auto& nameEntry : victoryPointEntry.at("custom_names")) {
                if (!nameEntry.is_object() ||
                    !nameEntry.contains("requirements") || !nameEntry.at("requirements").is_string() ||
                    !nameEntry.contains("name") || !nameEntry.at("name").is_string()) {
                    std::cout << "Province " << victoryPointEntry.at("id") << " has a bad custom name entry, skipping this name.\n";
                    continue;
                }

                objectCustomNames.emplace_back(nameEntry.at("name"), nameEntry.at("requirements"));
            }
        }

        Province province(objectId, objectDefaultName, objectCustomNames);

        // Explicit victory-point count (e.g. airports), if present.
        if (victoryPointEntry.contains("victory_points") && victoryPointEntry.at("victory_points").is_number_integer()) {
            province.victoryPoints = victoryPointEntry.at("victory_points").get<SignedInteger64>();
            province.hasVictoryPoints = true;
        }

        // Population object { tag, population, year } used to compute VPs when not set.
        if (victoryPointEntry.contains("population") && victoryPointEntry.at("population").is_object()) {
            const auto& populationObject = victoryPointEntry.at("population");
            if (populationObject.contains("population") && populationObject.at("population").is_number_integer() &&
                populationObject.contains("year") && populationObject.at("year").is_number_integer()) {
                province.population = populationObject.at("population").get<SignedInteger64>();
                province.populationYear = populationObject.at("year").get<SignedInteger32>();
                if (populationObject.contains("tag") && populationObject.at("tag").is_string()) {
                    province.populationTag = populationObject.at("tag").get<String>();
                }
                province.hasPopulation = true;
            }
        }

        provinces[objectId] = std::move(province);
    }
}

void WriteNames(const Path& modDirectory, const HashMap<SignedInteger64, Province>& provinces, const Vector<State>& statesVector, const String& modPrefix) {
	String scriptedEffectsOutFilePath = std::format("{0}/common/scripted_effects/{1}name_changes_scripted_effects.txt", modDirectory.string(), modPrefix);
	std::ofstream scriptedEffectsOutFile(scriptedEffectsOutFilePath, std::ios::binary);

	String stateIdString{};
	String provinceIdString{};
	String stateNameChangesString{};
	String changeAllCityNamesString{};

	for (const auto& state : statesVector) {
		stateIdString = std::to_string(state.id);

		stateNameChangesString = std::format("#{0}\n{1}update_state_{2}_names = {{\n", state.defaultName, modPrefix, stateIdString);

		if (state.customNames.size() != 0) {
			String prefix = "";
			stateNameChangesString += std::format("\t{0} = {{\n", stateIdString);

			SizeT i = 0;
			for (const auto& nameChange : state.customNames) {
			    stateNameChangesString += std::format(
			        "\t\t#{0}\n\t\t{1}if = {{\n\t\t\tlimit = {{ CONTROLLER = {{ {2} }} }}\n\t\t\tset_state_name = STATE_{3}_{4}\n\t\t}}\n",
			        nameChange.name, prefix, nameChange.nameRequirements, stateIdString, std::to_string(i)
			    );

				prefix = "else_";
				++i;
			}

			stateNameChangesString += "\t\telse = {\n\t\t\treset_state_name = yes\n\t\t}\n\t}\n";
		}

		for (const auto& provinceId : state.provinces) {
		    if (!provinces.contains(provinceId)) {
		        continue;
		    }

			const Province& province = provinces.at(provinceId);

			if (province.customNames.size() == 0) { continue; }

			provinceIdString = std::to_string(provinceId);
			String prefix = "";

			SizeT i = 0;
			for (const auto& nameChange : province.customNames) {
			    stateNameChangesString += std::format(
			        "\t#{0}\n\t{1}if = {{\n\t\tlimit = {{ any_country = {{ controls_province = {2} {3}"
			        " }} }}\n\t\tset_province_name = {{ id = {2} name = VICTORY_POINTS_{2}_{4} }}\n\t}}\n",
			        nameChange.name, prefix, provinceIdString, nameChange.nameRequirements, std::to_string(i)
			    );

				prefix = "else_";
				++i;
			}
            stateNameChangesString += std::format("\t#{0}\n\telse = {{\n\t\treset_province_name = {1}\n\t}}\n", province.defaultName, provinceIdString);
		}

		stateNameChangesString += "}\n\n";

		scriptedEffectsOutFile << stateNameChangesString;

		changeAllCityNamesString += std::format("\t{0}update_state_{1}_names = yes\n", modPrefix, stateIdString);
	}

	scriptedEffectsOutFile << std::format(
	    "\n\n{0}change_all_city_names = {{\n{1}}}\n\n{0}toggle_change_city_names = {{"
	    "\n\tif = {{\n\t\tlimit = {{ has_global_flag = {0}city_name_changes_active_flag }}"
	    "\n\t\tclr_global_flag = {0}city_name_changes_active_flag\n\t}}\n\telse = {{\n\t\t"
	    "set_global_flag = {0}city_name_changes_active_flag\n\t}}\n\t{0}change_all_city_names = yes\n}}",
	    modPrefix, changeAllCityNamesString
	);

	scriptedEffectsOutFile.close();

	stateNameChangesString = "";
	changeAllCityNamesString = "";

	String StatesOutFilePath = modDirectory.string() + "/localisation/english/state_names_l_english.yml";
	String VictoryPointsOutFilePath = modDirectory.string() + "/localisation/english/victory_points_l_english.yml" ;

	std::ofstream stateNamesYmlOutFile(StatesOutFilePath, std::ios::binary);
	std::ofstream victoryPointNamesYmlOutFile(VictoryPointsOutFilePath, std::ios::binary);

	const UnsignedChar bom_l_english[13] = {
		0xEF, 0xBB, 0xBF, 0x6C, 0x5F, 0x65, 0x6E, 0x67, 0x6C, 0x69, 0x73, 0x68, 0x3A
	};
	stateNamesYmlOutFile.write(reinterpret_cast<const Char*>(bom_l_english), 13);

	const UnsignedChar victory_points_tooltip[73] = {
		0x0A, 0x20, 0x56, 0x49, 0x43, 0x54, 0x4F, 0x52, 0x59, 0x5F, 0x50, 0x4F, 0x49, 0x4E, 0x54, 0x53, 0x5F, 0x54, 0x4F, 0x4F,
		0x4C, 0x54, 0x49, 0x50, 0x3A, 0x30, 0x20, 0x22, 0xC2, 0xA7, 0x47, 0x24, 0x4E, 0x41, 0x4D, 0x45, 0x24, 0xC2, 0xA7, 0x21,
		0x20, 0x76, 0x69, 0x63, 0x74, 0x6F, 0x72, 0x79, 0x20, 0x70, 0x6F, 0x69, 0x6E, 0x74, 0x73, 0x20, 0x3D, 0x20, 0xC2, 0xA7,
		0x59, 0x24, 0x50, 0x4F, 0x49, 0x4E, 0x54, 0x53, 0x24, 0xC2, 0xA7, 0x21, 0x22 };
	victoryPointNamesYmlOutFile.write(reinterpret_cast<const Char*>(bom_l_english), 13);
	victoryPointNamesYmlOutFile.write(reinterpret_cast<const Char*>(victory_points_tooltip), 73);

	String ymlOutString = "";
	SignedInteger64 id = 0;
	String idString = "";
	SizeT customNameIndex = 0;

	for (const auto& state : statesVector) {
		idString = std::to_string(state.id);

        ymlOutString += std::format("\n STATE_{0}: \"{1}\"", idString, state.defaultName);

		customNameIndex = 0;
		for (const auto& nameChange : state.customNames) {
		    ymlOutString += std::format("\n STATE_{0}_{1}: \"{2}\"", idString, customNameIndex, nameChange.name);
			++customNameIndex;
		}
	}

	stateNamesYmlOutFile << ymlOutString;
	stateNamesYmlOutFile.close();

	ymlOutString = "";
	for (const auto& state : statesVector) {
        for (const auto& provinceId : state.provinces) {
		    if (!provinces.contains(provinceId)) {
		        continue;
		    }

			const Province& province = provinces.at(provinceId);
			idString = std::to_string(province.id);

			ymlOutString += std::format("\n VICTORY_POINTS_{0}: \"{1}\"", idString, province.defaultName);

            customNameIndex = 0;
            for (const auto& nameChange : province.customNames) {
                ymlOutString += std::format("\n VICTORY_POINTS_{0}_{1}: \"{2}\"", idString, customNameIndex, nameChange.name);
                ++customNameIndex;
            }
        }
	}


	victoryPointNamesYmlOutFile << ymlOutString;
	victoryPointNamesYmlOutFile.close();
}

static Terrain ParseTerrainCategory(const PdxJson& data) {
	Terrain terrain{};

	for (const auto& [fieldKey, fieldValues] : data.asDict()) {
		if (!std::holds_alternative<String>(fieldKey) || fieldValues.empty()) { continue; }
		const String& field = std::get<String>(fieldKey);
		const PdxJson& value = fieldValues[0];

		if (field == "color") {
			if (value.isList() && value.size() >= 3) {
				terrain.colour = ColourRGB(
					static_cast<UnsignedInteger8>(value[0].getInt()),
					static_cast<UnsignedInteger8>(value[1].getInt()),
					static_cast<UnsignedInteger8>(value[2].getInt()));
			}
		}
		else if (field == "is_water")                     { terrain.isWater = value.getBool(); }
		else if (field == "naval_terrain")                { terrain.navalTerrain = value.getBool(); }
		else if (field == "sound_type")                   { terrain.soundType = value.getString(); }
		else if (field == "movement_cost")                { terrain.movementCost = value.getDouble(); }
		else if (field == "ai_terrain_importance_factor") { terrain.aiTerrainImportanceFactor = value.getDouble(); }
		else if (field == "combat_width")                 { terrain.combatWidth = static_cast<UnsignedInteger32>(value.getInt()); }
		else if (field == "combat_support_width")         { terrain.combatSupportWidth = static_cast<UnsignedInteger32>(value.getInt()); }
		else if (field == "match_value")                  { terrain.matchValue = static_cast<Float64>(value.getInt()); }
		else if (field == "buildings_max_level") {
			if (value.isDict()) {
				for (const auto& [buildingKey, buildingValues] : value.asDict()) {
					if (std::holds_alternative<String>(buildingKey) && !buildingValues.empty()) {
						terrain.buildingsMaxLevel[std::get<String>(buildingKey)] =
							static_cast<UnsignedInteger32>(buildingValues[0].getInt());
					}
				}
			}
		}
		else if (field == "units") {
			if (value.isDict()) {
				for (const auto& [unitKey, unitValues] : value.asDict()) {
					if (std::holds_alternative<String>(unitKey) && !unitValues.empty()) {
						terrain.units[std::get<String>(unitKey)] = unitValues[0].getDouble();
					}
				}
			}
		}
		else if (value.isDict()) {
			// Subunit modifiers
			TerrainSubunitModifier subUnit{};
			for (const auto& [subKey, subValues] : value.asDict()) {
				if (!std::holds_alternative<String>(subKey) || subValues.empty()) { continue; }
				const String& subField = std::get<String>(subKey);
				const PdxJson& subValue = subValues[0];

				if (subField == "units" && subValue.isDict()) {
					for (const auto& [unitKey, unitValues] : subValue.asDict()) {
						if (std::holds_alternative<String>(unitKey) && !unitValues.empty()) {
							subUnit.units[std::get<String>(unitKey)] = unitValues[0].getDouble();
						}
					}
				}
				else if (subValue.isNumber() || subValue.isBool()) {
					subUnit.modifiers[subField] = subValue.getDouble();
				}
			}
			terrain.subUnits[field] = std::move(subUnit);
		}
		else if (value.isNumber() || value.isBool()) {
			// Any other value is added to modifiers
			terrain.modifiers[field] = value.getDouble();
		}
	}

	return terrain;
}

HashMap<String, Terrain> LoadTerrain(const Path& modDirectory) {
	HashMap<String, Terrain> terrains;

	Vector<Path> terrainFiles = GetGameFiles(modDirectory, modDirectory, {"common/terrain"}, "common/terrain", ".txt");

	std::erase_if(terrainFiles, [](const Path& file) {
		return file.filename() == "TDA_unique_province_terrains.txt";
	});

	for (const auto& file : terrainFiles) {
		PdxJson terrainFile = ParseFileToPdxJson(file.string());

		if (!terrainFile.contains("categories")) { continue; }

		const PdxJson& categories = terrainFile.at("categories")[0];
		if (!categories.isDict()) { continue; }

		for (const auto& [nameKey, categoryValues] : categories.asDict()) {
			if (!std::holds_alternative<String>(nameKey) || categoryValues.empty()) { continue; }
			if (!categoryValues[0].isDict()) { continue; }

			terrains[std::get<String>(nameKey)] = ParseTerrainCategory(categoryValues[0]);
		}
	}

	return terrains;
}

// Split one CSV line into fields, honouring double-quoted fields that may
// contain commas (e.g. "Korea, Rep.") and "" as an escaped quote.
static Vector<String> SplitCsvLine(const String& line) {
	Vector<String> fields;
	String field;
	Boolean inQuotes = false;

	for (SizeT i = 0; i < line.size(); ++i) {
		const Char c = line[i];
		if (inQuotes) {
			if (c == '"') {
				if (i + 1 < line.size() && line[i + 1] == '"') { field += '"'; ++i; }
				else { inQuotes = false; }
			}
			else { field += c; }
		}
		else if (c == '"')  { inQuotes = true; }
		else if (c == ',')  { fields.push_back(field); field.clear(); }
		else if (c != '\r') { field += c; }
	}
	fields.push_back(field);
	return fields;
}

// Load country_pops.csv into tag -> (year -> population). The file is wide:
// "Country Name","Country Tag",1960,1961,...,2022.
HashMap<String, HashMap<SignedInteger32, SignedInteger64>> LoadCountryPopulations(const String& csvPath) {
	HashMap<String, HashMap<SignedInteger32, SignedInteger64>> populations;

	std::ifstream file(csvPath);
	if (!file.is_open()) {
		std::cout << "Could not open " << csvPath << ", skipping population-based victory points.\n";
		return populations;
	}

	String line;
	if (!std::getline(file, line)) { return populations; }

	Vector<String> header = SplitCsvLine(line);
	SizeT tagColumn = 1;                                   // "Country Tag"
	Vector<std::pair<SizeT, SignedInteger32>> yearColumns; // (column index, year)
	for (SizeT i = 0; i < header.size(); ++i) {
		if (header[i] == "Country Tag") { tagColumn = i; }
		else if (header[i].size() == 4 &&
			std::all_of(header[i].begin(), header[i].end(), [](unsigned char ch) { return std::isdigit(ch); })) {
			yearColumns.emplace_back(i, static_cast<SignedInteger32>(std::stoi(header[i])));
		}
	}

	while (std::getline(file, line)) {
		if (line.empty()) { continue; }
		Vector<String> fields = SplitCsvLine(line);
		if (tagColumn >= fields.size() || fields[tagColumn].empty()) { continue; }

		HashMap<SignedInteger32, SignedInteger64>& byYear = populations[fields[tagColumn]];
		for (const auto& [columnIndex, year] : yearColumns) {
			if (columnIndex >= fields.size() || fields[columnIndex].empty()) { continue; }
			try { byYear[year] = static_cast<SignedInteger64>(std::stod(fields[columnIndex])); }
			catch (...) { /* non-numeric cell, skip */ }
		}
	}
	return populations;
}

// Country population for a year, clamping to the nearest available year when the
// exact one is missing (the table only runs 1960-2022, but VP years can be 2025).
static SignedInteger64 CountryPopulationForYear(const HashMap<SignedInteger32, SignedInteger64>& byYear, SignedInteger32 year) {
	if (byYear.empty()) { return 0; }
	if (auto it = byYear.find(year); it != byYear.end()) { return it->second; }

	SignedInteger32 minYear = INT32_MAX, maxYear = INT32_MIN;
	for (const auto& [y, _] : byYear) { minYear = std::min(minYear, y); maxYear = std::max(maxYear, y); }

	SignedInteger32 clamped = std::clamp(year, minYear, maxYear);
	auto it = byYear.find(clamped);
	return it != byYear.end() ? it->second : 0;
}

// Level -> people-per-victory-point scale plus per-country level assignments,
// loaded from country_levels.json.
struct CountryLevels {
	HashMap<String, Float64> peoplePerVictoryPointByTag;
	Float64 defaultPeoplePerVictoryPoint = 100000.0;
};

static CountryLevels LoadCountryLevels(const String& jsonPath) {
	CountryLevels result;

	std::ifstream file(jsonPath);
	if (!file.is_open()) {
		std::cout << "Could not open " << jsonPath << "; falling back to "
			<< result.defaultPeoplePerVictoryPoint << " people per victory point.\n";
		return result;
	}

	json levelsJson = json::parse(file);

	// level number -> people per victory point (e.g. 1 -> 50000, 5 -> 125000).
	HashMap<SignedInteger32, Float64> peoplePerVpByLevel;
	if (levelsJson.contains("levels") && levelsJson["levels"].is_object()) {
		for (const auto& [levelKey, value] : levelsJson["levels"].items()) {
			if (value.is_number()) { peoplePerVpByLevel[std::stoi(levelKey)] = value.get<Float64>(); }
		}
	}

	SignedInteger32 defaultLevel = levelsJson.value("default_level", 3);
	if (auto it = peoplePerVpByLevel.find(defaultLevel); it != peoplePerVpByLevel.end()) {
		result.defaultPeoplePerVictoryPoint = it->second;
	}

	// country tag -> people per victory point (resolved through its level).
	if (levelsJson.contains("country_levels") && levelsJson["country_levels"].is_object()) {
		for (const auto& [tag, levelValue] : levelsJson["country_levels"].items()) {
			if (!levelValue.is_number_integer()) { continue; }
			if (auto it = peoplePerVpByLevel.find(levelValue.get<SignedInteger32>()); it != peoplePerVpByLevel.end()) {
				result.peoplePerVictoryPointByTag[tag] = it->second;
			}
		}
	}
	return result;
}

// For every VP province WITHOUT an explicit victory-point count but WITH a
// population, estimate its 2010 population via its owner country's population
// trend and set victory_points at 1 per (people-per-VP for the owner's level;
// minimum 1).
void AssignVictoryPointsFromPopulation(
	HashMap<SignedInteger64, Province>& provinces,
	const HashMap<String, HashMap<SignedInteger32, SignedInteger64>>& countryPopulations,
	const CountryLevels& countryLevels
) {
	constexpr SignedInteger32 targetYear = 2010;

	SizeT assigned = 0;
	for (auto& [id, province] : provinces) {
		if (province.hasVictoryPoints || !province.hasPopulation) { continue; }

		// Default: use the raw figure if we can't scale via the owner country.
		Float64 population2010 = static_cast<Float64>(province.population);

		if (auto tagIt = countryPopulations.find(province.populationTag); tagIt != countryPopulations.end()) {
			const SignedInteger64 countryAtTarget = CountryPopulationForYear(tagIt->second, targetYear);
			const SignedInteger64 countryAtBase = CountryPopulationForYear(tagIt->second, province.populationYear);
			if (countryAtTarget > 0 && countryAtBase > 0) {
				population2010 = static_cast<Float64>(province.population)
					* static_cast<Float64>(countryAtTarget) / static_cast<Float64>(countryAtBase);
			}
		}

		// People-per-victory-point comes from the owner country's level.
		Float64 peoplePerVictoryPoint = countryLevels.defaultPeoplePerVictoryPoint;
		if (auto it = countryLevels.peoplePerVictoryPointByTag.find(province.populationTag);
			it != countryLevels.peoplePerVictoryPointByTag.end()) {
			peoplePerVictoryPoint = it->second;
		}

		SignedInteger64 victoryPoints = static_cast<SignedInteger64>(std::llround(population2010 / peoplePerVictoryPoint));
		if (victoryPoints < 1) { victoryPoints = 1; }  // a named VP with people is worth at least 1

		province.victoryPoints = victoryPoints;
		province.hasVictoryPoints = true;
		++assigned;
	}

	std::cout << "Assigned population-based victory points to " << assigned << " province(s).\n";
}

// Read/write a whole file as raw bytes (binary) so line endings and every other
// byte are preserved exactly — these edits must not reformat anything.
static String ReadFileBytes(const Path& path) {
	std::ifstream in(path, std::ios::binary);
	return String((std::istreambuf_iterator<Char>(in)), std::istreambuf_iterator<Char>());
}
static void WriteFileBytes(const Path& path, const String& content) {
	std::ofstream out(path, std::ios::binary);
	out << content;
}

// Index of the '}' matching the '{' at openBrace (simple depth count).
static SizeT FindMatchingBrace(const String& text, SizeT openBrace) {
	SignedInteger32 depth = 0;
	for (SizeT i = openBrace; i < text.size(); ++i) {
		if (text[i] == '{') { ++depth; }
		else if (text[i] == '}') { if (--depth == 0) { return i; } }
	}
	return String::npos;
}

// Return the state-file text with (a) every active `victory_points = { id n }`
// line removed and (b) fresh victory_points for the state's VP provinces added at
// the end of the history = { } block. Everything else is left byte-for-byte.
static String InjectVictoryPointsIntoState(const String& original, const HashMap<SignedInteger64, Province>& provinces) {
	const String newline = original.find("\r\n") != String::npos ? "\r\n" : "\n";

	// (a) Drop active victory_points lines (commented ones don't match, so survive).
	static const std::regex activeVictoryPoint(
		R"(^[ \t]*victory_points[ \t]*=[ \t]*\{[ \t]*[0-9]+[ \t]+[0-9]+[ \t]*\}[ \t]*$)");
	String content;
	content.reserve(original.size());
	for (SizeT i = 0; i < original.size();) {
		SizeT eol = original.find('\n', i);
		SizeT lineEnd = (eol == String::npos) ? original.size() : eol + 1;
		String body = original.substr(i, ((eol == String::npos) ? original.size() : eol) - i);
		if (!body.empty() && body.back() == '\r') { body.pop_back(); }
		if (!std::regex_match(body, activeVictoryPoint)) {
			content += original.substr(i, lineEnd - i);  // keep line incl. its newline
		}
		i = lineEnd;
	}

	// Collect the state's VP provinces (its provinces = { ... } that carry a VP value).
	Vector<std::pair<SignedInteger64, SignedInteger64>> toAdd;
	{
		static const std::regex provincesBlock(R"(provinces[ \t]*=[ \t]*\{([^}]*)\})");
		std::smatch match;
		if (std::regex_search(content, match, provincesBlock)) {
			std::istringstream ids(match[1].str());
			SignedInteger64 provinceId;
			while (ids >> provinceId) {
				auto it = provinces.find(provinceId);
				if (it != provinces.end() && it->second.hasVictoryPoints) {
					toAdd.emplace_back(provinceId, it->second.victoryPoints);
				}
			}
		}
	}
	std::sort(toAdd.begin(), toAdd.end());

	if (toAdd.empty()) { return content; }

	// (b) Insert at the end of the history block, before its closing brace's line.
	static const std::regex historyOpen(R"(history[ \t]*=[ \t]*\{)");
	std::smatch historyMatch;
	if (!std::regex_search(content, historyMatch, historyOpen)) { return content; }

	SizeT openBrace = static_cast<SizeT>(historyMatch.position(0) + historyMatch.length(0)) - 1;
	SizeT closeBrace = FindMatchingBrace(content, openBrace);
	if (closeBrace == String::npos) { return content; }

	SizeT lineStart = content.rfind('\n', closeBrace);
	lineStart = (lineStart == String::npos) ? 0 : lineStart + 1;
	SizeT firstNonWs = lineStart;
	while (firstNonWs < closeBrace && (content[firstNonWs] == ' ' || content[firstNonWs] == '\t')) { ++firstNonWs; }
	const String indent = content.substr(lineStart, firstNonWs - lineStart) + "\t";  // one deeper than the brace

	String insertion;
	for (const auto& [provinceId, victoryPoints] : toAdd) {
		insertion += indent + "victory_points = { " + std::to_string(provinceId) + " " + std::to_string(victoryPoints) + " }" + newline;
	}
	content.insert(lineStart, insertion);
	return content;
}

void WriteVictoryPointsToStateFiles(const Path& modDirectory, const HashMap<SignedInteger64, Province>& provinces) {
	Vector<Path> stateFiles = GetGameFiles(modDirectory, modDirectory, {"history/states"}, "history/states", ".txt");

	SizeT edited = 0;
	for (const auto& file : stateFiles) {
		String content = ReadFileBytes(file);
		String updated = InjectVictoryPointsIntoState(content, provinces);
		if (updated != content) { WriteFileBytes(file, updated); ++edited; }
	}
	std::cout << "Rewrote victory points in " << edited << " state file(s).\n";
}

// Rewrite map/definition.csv so every VP province's terrain (field index 6) becomes
// "TDA_terrain_<id>". All other fields, lines and line endings are untouched.
static String ReplaceVpTerrainsInDefinition(const String& content, const HashMap<SignedInteger64, Province>& provinces) {
	String result;
	result.reserve(content.size());

	for (SizeT i = 0; i < content.size();) {
		SizeT eol = content.find('\n', i);
		SizeT lineEnd = (eol == String::npos) ? content.size() : eol + 1;
		String line = content.substr(i, lineEnd - i);
		i = lineEnd;

		String body = line;
		String ending;
		while (!body.empty() && (body.back() == '\n' || body.back() == '\r')) { ending.insert(ending.begin(), body.back()); body.pop_back(); }

		Vector<String> fields;
		for (SizeT start = 0, k = 0; k <= body.size(); ++k) {
			if (k == body.size() || body[k] == ';') { fields.push_back(body.substr(start, k - start)); start = k + 1; }
		}

		bool isNumericId = !fields.empty() && !fields[0].empty() &&
			std::all_of(fields[0].begin(), fields[0].end(), [](unsigned char c) { return std::isdigit(c); });

		if (fields.size() > 6 && isNumericId && provinces.count(std::stoll(fields[0]))) {
			fields[6] = "TDA_terrain_" + fields[0];
			String rebuilt;
			for (SizeT f = 0; f < fields.size(); ++f) { if (f) { rebuilt += ';'; } rebuilt += fields[f]; }
			result += rebuilt + ending;
		}
		else {
			result += line;
		}
	}
	return result;
}

void WriteUniqueProvinceTerrains(const Path& modDirectory, const HashMap<SignedInteger64, Province>& provinces) {
	Path definitionPath = modDirectory / "map" / "definition.csv";
	String content = ReadFileBytes(definitionPath);
	if (content.empty()) {
		std::cout << "Could not read " << definitionPath.string() << ", skipping terrain rewrite.\n";
		return;
	}
	WriteFileBytes(definitionPath, ReplaceVpTerrainsInDefinition(content, provinces));
	std::cout << "Rewrote per-VP terrains in definition.csv.\n";
}

int main() {
	//Get the mod directory
	Path modDirectory = std::filesystem::current_path().parent_path().parent_path();

	HashMap<String, Terrain> terrains = LoadTerrain(modDirectory);

	HashMap<String, HashMap<SignedInteger32, SignedInteger64>> countryPopulations = LoadCountryPopulations("country_pops.csv");
	CountryLevels countryLevels = LoadCountryLevels("country_levels.json");

    HashMap<SignedInteger64, Province> provinces;
    HashMap<SignedInteger64, State> states;
    String modPrefix;

    LoadStateProvinceList(modDirectory, states);

	LoadNamesFromJson("names.json", provinces, states, modPrefix);

	// Fill in victory points from population (1 per 100k, scaled to 2010) for
	// any province that doesn't already have an explicit value.
	AssignVictoryPointsFromPopulation(provinces, countryPopulations, countryLevels);

    Vector<State> statesVector {}; statesVector.reserve(states.size());
	for (auto& [stateId, stateData] : states) {
	    std::sort(stateData.provinces.begin(), stateData.provinces.end());
	    statesVector.push_back(stateData);
	}

	std::sort(statesVector.begin(), statesVector.end(), [](const State& a, const State& b) { return a.id < b.id; });

	WriteNames(modDirectory, provinces, statesVector, modPrefix);

	// Write victory points into history/states and per-VP terrains into definition.csv.
	WriteVictoryPointsToStateFiles(modDirectory, provinces);
	//WriteUniqueProvinceTerrains(modDirectory, provinces);

	return 0;
}
