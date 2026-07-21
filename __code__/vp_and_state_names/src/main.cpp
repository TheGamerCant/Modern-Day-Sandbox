// g++ src/*.cpp -std=c++20 -static -O3 -o name_writer.exe
// g++ src/*.cpp -std=c++20 -O3 -o name_writer_mac

#include <fstream>
#include <iostream>
#include <algorithm>

#include "data_types.hpp"
#include "functions.hpp"

#include "json.hpp"
using json = nlohmann::json;

struct NameEntry {
public:
    String name;
    String nameRequirements;

    NameEntry() : name(""), nameRequirements("") {};
    NameEntry(const String& name, const String& nameRequirements) :
        name(name), nameRequirements(nameRequirements) {};
};

struct Province {
public:
    SignedInteger64 id;
    String defaultName;
    Vector<NameEntry> customNames;

    Province() : id(0), defaultName(""), customNames() {};
    Province(const SignedInteger64 id, const String& defaultName, const Vector<NameEntry>& customNames) :
        id(id), defaultName(defaultName), customNames(customNames) {};
};

struct State {
public:
    SignedInteger64 id;
    String defaultName;
    Vector<NameEntry> customNames;
    Vector<SignedInteger64> provinces;

    State() : id(0), defaultName(""), customNames(), provinces() {};
    State(const SignedInteger64 id, const Vector<SignedInteger64>& provinces) :
        id(id), defaultName(""), customNames(), provinces(provinces) {};
};

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

        provinces[objectId] = Province(objectId, objectDefaultName, objectCustomNames);
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

int main() {
	//Get the mod directory
	Path modDirectory = std::filesystem::current_path().parent_path().parent_path();

    HashMap<SignedInteger64, Province> provinces;
    HashMap<SignedInteger64, State> states;
    String modPrefix;

    LoadStateProvinceList(modDirectory, states);

	LoadNamesFromJson("names.json", provinces, states, modPrefix);

    Vector<State> statesVector {}; statesVector.reserve(states.size());
	for (auto& [stateId, stateData] : states) {
	    std::sort(stateData.provinces.begin(), stateData.provinces.end());
	    statesVector.push_back(stateData);
	}

	std::sort(statesVector.begin(), statesVector.end(), [](const State& a, const State& b) { return a.id < b.id; });

	WriteNames(modDirectory, provinces, statesVector, modPrefix);

	return 0;
}
