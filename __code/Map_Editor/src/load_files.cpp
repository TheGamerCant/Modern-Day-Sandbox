#include "load_files.hpp"
#include "data_types.hpp"
#include <fstream>
#include <iostream>

void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories) {
    HashMap<String, String> directoriesMap = ParseStringForPairsMapUnique(LoadFileToString("file_directories.txt"));

    //Remove any trailing quotation marks we may have
    for (auto& [key, value] : directoriesMap) {
        if (value.size() >= 2) {
            Char first = value.front();
            Char last = value.back();

            if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) { value = value.substr(1, value.size() - 2); }
        }
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

    if (modFileCount == 0) { FatalError("Could not find any .mod files in the mod directory " + modDirectory.string()); }
    else if (modFileCount == 0) { FatalError("More than one .mod files in the mod directory " + modDirectory.string()); }

    else {
        modReplaceDirectories.reserve(64);
        HashMap<String, Vector<String>> modFolderMap = ParseStringForPairsMapRepeat(LoadFileToString(modFilePath.string()));

        if (modFolderMap.find("replace_path") != modFolderMap.end()) {
            for (auto& pathToReplace : modFolderMap.at("replace_path")) {
                if (pathToReplace.size() >= 2) {
                    Char first = pathToReplace.front();
                    Char last = pathToReplace.back();

                    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) { pathToReplace = pathToReplace.substr(1, pathToReplace.size() - 2); }
                    for (char& c : pathToReplace) { if (c == '/') { c = '\\'; } }
                    modReplaceDirectories.push_back(pathToReplace);
                }
            }
        }
    }

    std::sort(modReplaceDirectories.begin(), modReplaceDirectories.end());
}

void LoadCountryFiles(Vector<Country>& countriesArray, const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, Vector<String>& errorsLog) {
    Vector<Path> countryTagFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\country_tags", ".txt", 4);

    UnsignedInteger16 countryIndex = 0;
    for (const Path& file : countryTagFiles) {
        Vector<DoubleString> countryTagAndDefinitionFiles = ParseStringForPairsArray(LoadFileToString(file.string()), 128);
        countriesArray.reserve(countriesArray.capacity() + countryTagAndDefinitionFiles.size());

        for (const auto& [tag, countryFile] : countryTagAndDefinitionFiles) {
            if (tag == "dynamic_tags" && countryFile == "yes") break;

            if (!TagIsValid(tag)) { 
                errorsLog.push_back("Error in " + file.string() + ", tag " + tag + " is not a valid tag"); 
                continue; 
            }
            
            countriesArray.emplace_back(countryIndex++, tag);
        }
    }
    countriesArray.shrink_to_fit();
}

void LoadTerrainFiles() {

}