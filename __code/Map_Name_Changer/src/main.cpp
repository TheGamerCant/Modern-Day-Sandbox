#include "functions.hpp"
#include "data_types.hpp"

#include <iostream>

void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories) {
    HashMap<String, String> directoriesMap = ParseStringForPairsMapUnique(LoadFileToString("in\\file_directories.txt"));

    //Remove any trailing quotation marks we may have
    for (auto& [key, value] : directoriesMap) {
        if (value.size() >= 2) { value = RemoveQuotes(value); }
    }

    if (directoriesMap.find("mod_directory") == directoriesMap.end()) FatalError("Mod directory is not defined in file_directories.txt");
    if (directoriesMap.find("vanilla_directory") == directoriesMap.end()) FatalError("Base game directory is not defined in file_directories.txt");


    if (std::filesystem::exists(directoriesMap.at("mod_directory")) && std::filesystem::is_directory(directoriesMap.at("mod_directory"))) modDirectory = directoriesMap.at("mod_directory");
    else FatalError(directoriesMap.at("mod_directory") + " is not a valid path");

    if (std::filesystem::exists(directoriesMap.at("vanilla_directory")) && std::filesystem::is_directory(directoriesMap.at("vanilla_directory"))) vanillaDirectory = directoriesMap.at("vanilla_directory");
    else FatalError(directoriesMap.at("vanilla_directory") + " is not a valid path");


    //Now get all replace_path entries in the .mod folder
    UnsignedInteger32 modFileCount = 0;
    Path modFilePath;

    for (const auto& entry : std::filesystem::directory_iterator(modDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mod") {
            ++modFileCount;
            modFilePath = entry.path();
        }
    }

    //If zero we can load vanilla
    if (modFileCount == 1) {
        modReplaceDirectories.reserve(64);
        HashMap<String, Vector<String>> modFolderMap = ParseStringForPairsMapRepeat(LoadFileToString(modFilePath.string()));

        if (modFolderMap.find("replace_path") != modFolderMap.end()) {
            for (auto& pathToReplace : modFolderMap.at("replace_path")) {
                if (pathToReplace.size() >= 2) {
                    pathToReplace = RemoveQuotes(pathToReplace);
                    pathToReplace = ForwardToBackslashes(pathToReplace);

                    modReplaceDirectories.push_back(pathToReplace);
                }
            }
        }
    }

    std::sort(modReplaceDirectories.begin(), modReplaceDirectories.end());
}

void LoadStateFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<State>& statesArray, Vector<Province>& provincesArray) {
    Vector<Path> stateFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "history\\states", ".txt", 1800);

    statesArray.reserve(stateFiles.size() + 1);
    statesArray.emplace_back(0);

    UnsignedInteger32 provinceCount = 1;

    for (const auto& file : stateFiles) {
        HashMap<String, Vector<String>> states = ParseStringForPairsMapRepeat(LoadFileToString(file.string()));

        if (states.find("state") != states.end()) {
            Vector<String> statesDataWhole = states.at("state");

            for (const auto& stateDataWhole : statesDataWhole) {
                HashMap<String, Vector<String>> stateData = ParseStringForPairsMapRepeat(stateDataWhole);

                if (stateData.find("id") == stateData.end() || !StringCanBecomeInteger(stateData.at("id")[0])) { FatalError("No/invalid ID defined in " + file.string()); }
                UnsignedInteger32 id = std::stoi(stateData.at("id")[stateData.at("id").size() - 1]);
                String stringId = std::to_string(id);

                if (stateData.find("provinces") == stateData.end() || stateData.at("provinces").size() != 1) { FatalError("No provinces defined for state " + stringId); }
                Vector<UnsignedInteger16> provinces = ParseStringAsUnsignedInteger16Array(stateData.at("provinces")[0]);

                statesArray.emplace_back(id, provinces);
                provinceCount += provinces.size();
            }
        }
    }

    provinces.reserve(provinceCount);
    for (Size_T i = 0; i < provinceCount; i++) {
        provinces.emplace_back(i);
    }
}

void LoadNames(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<State>& statesArray, Vector<Province>& provincesArray) {
    Vector<DoubleString> stateNames = ParseStringForPairsArray(LoadFileToString("in\\names_states.txt"));

    HashMap<UnsignedInteger16, HashMap<String, Vector<String>>> stateNameEntries;
    for (const auto& [stateId, data] : stateNames) {
        if (StringCanBecomeInteger(stateId)) {
            stateNameEntries[std::stoi(stateId)] = ParseStringForPairsMapRepeat(data);
        }
        else {
            String outString = "Bad state ID defined in in\\names_states.txt\n";
            std::cout << outString;
        }
    }

    for (const auto& [stateId, dataHashMap] : stateNameEntries) {
        if (dataHashMap.find("default") == dataHashMap.end()) {
            String outString = "No default name defined for state " + std::to_string(stateId) + "\n";
            std::cout << outString;
            continue;
        }

        if (dataHashMap.at("default").size() > 1) {
            String outString = "More than one default name defined for " + std::to_string(stateId) + ", the last one will be used\n";
            std::cout << outString;
        }

        String defaultName = RemoveQuotes(dataHashMap.at("default")[dataHashMap.at("default").size() - 1]);
        Vector<ChangeableName> nameEntries;

        if (dataHashMap.find("entry") != dataHashMap.end() && dataHashMap.at("entry").size() > 0) {
            Vector<String> entriesDataVector = dataHashMap.at("entry");
            for (const auto& entryDataWhole : entriesDataVector) {
                HashMap<String, String> entryData = ParseStringForPairsMapUnique(entryDataWhole);

                if (entryData.find("name") == entryData.end()) {
                    String outString = "Name entry for state " + std::to_string(stateId) + " does not have a name defined.\n";
                    std::cout << outString;
                    continue;
                }
                String changedName = RemoveQuotes(entryData.at("name"));

                if (entryData.find("requirements") == entryData.end()) {
                    String outString = "Name entry for state " + std::to_string(stateId) + " does not have a requirements defined.\n";
                    std::cout << outString;
                    continue;
                }
                Vector<String> nameRequirements = ParseStringAsStringArray(entryData.at("requirements"));

                nameEntries.emplace_back(changedName, nameRequirements);
            }
        }

        if (statesArray.size() >= stateId) {
            statesArray[stateId].SetDefaultName(defaultName);
            statesArray[stateId].SetNameEntries(nameEntries);
        }
    }

    Vector<DoubleString> vpNames = ParseStringForPairsArray(LoadFileToString("in\\names_victory_points.txt"));

    HashMap<UnsignedInteger16, HashMap<String, Vector<String>>> vpNameEntries;
    for (const auto& [vp, data] : vpNames) {
        if (StringCanBecomeInteger(vp)) {
            vpNameEntries[std::stoi(vp)] = ParseStringForPairsMapRepeat(data);
		}
        else {
			String outString = "Bad victory point ID defined in in\\names_victory_points.txt\n";
            std::cout << outString;
        }
    }

    for (const auto& [vp, dataHashMap] : vpNameEntries) {
        if (dataHashMap.find("default") == dataHashMap.end()) {
            String outString = "No default name defined for victory point " + std::to_string(vp) + "\n";
			std::cout << outString;
            continue;
        }

        if (dataHashMap.at("default").size() > 1) {
            String outString = "More than one default name defined for " + std::to_string(vp) + ", the last one will be used\n";
            std::cout << outString;
        }

		String defaultName = RemoveQuotes(dataHashMap.at("default")[dataHashMap.at("default").size() - 1]);
        Vector<ChangeableName> nameEntries;

        if (dataHashMap.find("entry") != dataHashMap.end() && dataHashMap.at("entry").size() > 0) {
			Vector<String> entriesDataVector = dataHashMap.at("entry");
            for (const auto& entryDataWhole : entriesDataVector) {
				HashMap<String, String> entryData = ParseStringForPairsMapUnique(entryDataWhole);

                if (entryData.find("name") == entryData.end()) {
                    String outString = "Name entry for victory point " + std::to_string(vp) + " does not have a name defined.\n";
                    std::cout << outString;
                    continue;
                }
                String changedName = RemoveQuotes(entryData.at("name"));

                if (entryData.find("requirements") == entryData.end()) {
                    String outString = "Name entry for victory point " + std::to_string(vp) + " does not have a requirements defined.\n";
                    std::cout << outString;
                    continue;
                }
                Vector<String> nameRequirements = ParseStringAsStringArray(entryData.at("requirements"));

                nameEntries.emplace_back(changedName, nameRequirements);
            }
        }

        if (provincesArray.size() > vp) {
            provincesArray[vp].SetDefaultName(defaultName);
            provincesArray[vp].SetNameEntries(nameEntries);
        }
    }
}


void WriteNames(const String& modDirectory, const Vector<Province>& provincesArray, const Vector<State>& statesArray) {
	std::ofstream scriptedEffectsOutFile(modDirectory + "\\common\\scripted_effects\\FFTF_name_changes_scripted_effects.txt", std::ios::binary);

	UnsignedInteger16 stateId{};
	String stateIdString{};
	String provinceIdString{};
	String stateNameChangesString{};
	String changeAllCityNamesString{};

	for (const auto& state : statesArray) {
		stateId = state.GetId();
		stateIdString = std::to_string(stateId);

		if (stateId == 0) { continue; }

		stateNameChangesString = "#" + state.GetDefaultName() + "\nFFRF_update_state_" + stateIdString + "_names = {\n";

		if (state.GetChangeableNameCount() != 0) {
			String prefix = "";
			stateNameChangesString += "\t" + stateIdString + " = {\n";

			SizeT i = 0;
			for (const auto& nameChange : state.GetNameEntries()) {
				stateNameChangesString += "\t\t#" + nameChange.name + "\n\t\t" + prefix + "if = {\n\t\t\tlimit = { CONTROLLER = {";
				for (const auto& requirement : nameChange.nameRequirements) {
					stateNameChangesString += " " + requirement;
				}
				stateNameChangesString += " } }\n\t\t\tset_state_name = " + state.GetName() + "_" + std::to_string(i) + "\n\t\t}\n";

				prefix = "else_";
				++i;
			}

			stateNameChangesString += "\t\telse = {\n\t\t\treset_state_name = yes\n\t\t}\n\t}\n";
		}

		for (const auto& provId : state.GetProvinces()) {
			const Province& prov = provincesArray[provId];

			if (prov.GetChangeableNameCount() == 0) { continue; }

			provinceIdString = std::to_string(provId);
			String prefix = "";

			SizeT i = 0;
			for (const auto& nameChange : prov.GetNameEntries()) {
				stateNameChangesString += "\t#" + nameChange.name + "\n\t" + prefix + "if = {\n\t\tlimit = { any_country = { controls_province = " +
					provinceIdString;
				for (const auto& requirement : nameChange.nameRequirements) {
					stateNameChangesString += " " + requirement;
				}
				stateNameChangesString += " } }\n\t\tset_province_name = { id = " + provinceIdString + " name = VICTORY_POINTS_" + provinceIdString +
					"_" + std::to_string(i) + " }\n\t}\n";

				prefix = "else_";
				++i;
			}

			stateNameChangesString += "\t#" + prov.GetDefaultName() + "\n\telse = {\n\t\treset_province_name = " + provinceIdString +"\n\t}\n";
		}

		stateNameChangesString += "}\n\n";

		scriptedEffectsOutFile << stateNameChangesString;

		changeAllCityNamesString += "\tFFF_update_state_" + stateIdString + "_names = yes\n";
	}


	scriptedEffectsOutFile << "\n\nFFTF_change_all_city_names = {\n" << changeAllCityNamesString +
		"}\n\nFFTF_toggle_change_city_names = {\n\tif = {\n\t\tlimit = { has_global_flag = FFTF_city_name_changes_active_flag }\
		\n\t\tclr_global_flag = FFTF_city_name_changes_active_flag\n\t}\n\telse = {\n\t\tset_global_flag = FFTF_city_name_changes_active_flag\n\t}\
		\n\tFFTF_change_all_city_names = yes\n}";
	scriptedEffectsOutFile.close();

	stateNameChangesString = "";
	changeAllCityNamesString = "";

	std::ofstream stateNamesYmlOutFile(modDirectory + "\\localisation\\english\\state_names_l_english.yml", std::ios::binary);
	std::ofstream victoryPointNamesYmlOutFile(modDirectory + "\\localisation\\english\\victory_points_l_english.yml", std::ios::binary);

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
	UnsignedInteger16 id = 0;
	String idString = "";
	SizeT customNameIndex = 0;

	for (const auto& state : statesArray) {
		id = state.GetId();
		if (id == 0) { continue; }


		ymlOutString += "\n " + state.GetName() + ": \"" + state.GetDefaultName() + "\"";
		customNameIndex = 0;
		for (const auto& nameChange : state.GetNameEntries()) {
			ymlOutString += "\n " + state.GetName() + "_" + std::to_string(customNameIndex) + ": \"" + nameChange.name + "\"";
			++customNameIndex;
		}
	}

	stateNamesYmlOutFile << ymlOutString;
	stateNamesYmlOutFile.close();

	ymlOutString = "";
	for (const auto& province : provincesArray) {
		if (province.GetDefaultName() != "") {
			id = province.GetId();
			idString = std::to_string(id);


			ymlOutString += "\n VICTORY_POINTS_" + idString + ": \"" + province.GetDefaultName() + "\"";
			customNameIndex = 0;
			for (const auto& nameChange : province.GetNameEntries()) {
				ymlOutString += "\n VICTORY_POINTS_" + idString + "_" + std::to_string(customNameIndex) + ": \"" + nameChange.name + "\"";
				++customNameIndex;
			}
		}
	}


	victoryPointNamesYmlOutFile << ymlOutString;
	victoryPointNamesYmlOutFile.close();
}

int main() {
    Timestamp startTime = std::chrono::high_resolution_clock::now();

    Path vanillaDirectory, modDirectory; Vector<String> modReplaceDirectories;
    LoadFileDirectories(vanillaDirectory, modDirectory, modReplaceDirectories);

    Vector<Province> provincesArray;
    Vector<State>statesArray;

    LoadStateFiles(vanillaDirectory, modDirectory, modReplaceDirectories, statesArray, provincesArray);

    provincesArray.shrink_to_fit();
    std::sort(provincesArray.begin(), provincesArray.end(), [](const Province& a, const Province& b) { return a.id) < b.id(); });

    LoadNames(vanillaDirectory, modDirectory, modReplaceDirectories, statesArray, provincesArray);

    WriteNames(modDirectory.string(), provincesArray, statesArray);

    std::cout << "Program ran for  " << GetTimeElapsedFromStart(startTime);

    return 0;
}