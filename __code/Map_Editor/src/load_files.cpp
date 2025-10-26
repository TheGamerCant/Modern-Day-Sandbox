#include "load_files.hpp"
#include "data_types.hpp"
#include <fstream>
#include <iostream>

void LoadFileDirectories(Path& vanillaDirectory, Path& modDirectory, Vector<String>& modReplaceDirectories) {
    HashMap<String, String> directoriesMap = ParseStringForPairsMapUnique(LoadFileToString("file_directories.txt"));

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

    if (modFileCount == 0) { FatalError("Could not find any .mod files in the mod directory " + modDirectory.string()); }
    else if (modFileCount == 0) { FatalError("More than one .mod files in the mod directory " + modDirectory.string()); }

    else {
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

VectorMap<GraphicalCulture> LoadGraphicalCultureFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories) {
    VectorMap<GraphicalCulture> culturesReturnArray;
    Vector<String> culturesArray = ParseStringAsArray(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\graphicalculturetype.txt").string()));
    culturesReturnArray.Reserve(culturesArray.size() / 2);

    for (auto& culture : culturesArray) {
        if (!culturesReturnArray.NameInArray(culture)) culturesReturnArray.EmplaceBack(culture);
    }

    return culturesReturnArray;
}

static void BadColourDefinition(const String& countryFile) { FatalError("Bad colour definition at " + countryFile); }

VectorMap<Country> LoadCountryFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, const VectorMap<GraphicalCulture>& graphicalCulturesArray) {
    Vector<Path> countryTagFiles = GetGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\country_tags", ".txt", 4);
    VectorMap<Country> countriesReturnArray;

    for (const Path& file : countryTagFiles) {
        Vector<DoubleString> countryTagAndDefinitionFiles = ParseStringForPairsArray(LoadFileToString(file.string()), 128);
        countriesReturnArray.Reserve(countriesReturnArray.Capacity() + countryTagAndDefinitionFiles.size());

        for (auto& [tag, countryFile] : countryTagAndDefinitionFiles) {
            if (tag == "dynamic_tags" && countryFile == "yes") break;

            if (!TagIsValid(tag)) { 
                FatalError("Error in " + file.string() + ", tag " + tag + " is not a valid tag"); 
            }
        
            countryFile = RemoveQuotes(countryFile);
            countryFile = ForwardToBackslashes(countryFile);

            HashMap<String, String> countryCosmeticData = ParseStringForPairsMapUnique(LoadFileToString(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, "common\\" + countryFile).string()));
            UnsignedInteger16 cultureIndex = 0, cultureIndex2D = 0;

            if (countryCosmeticData.find("graphical_culture") == countryCosmeticData.end() || countryCosmeticData.find("graphical_culture_2d") == countryCosmeticData.end() || 
                !graphicalCulturesArray.NameInArray(countryCosmeticData.at("graphical_culture")) || !graphicalCulturesArray.NameInArray(countryCosmeticData.at("graphical_culture"))) {
                
                FatalError("Incorrect graphical cultures definition at " + countryFile);
            }
            cultureIndex = graphicalCulturesArray[countryCosmeticData.at("graphical_culture")].GetId();
            cultureIndex2D = graphicalCulturesArray[countryCosmeticData.at("graphical_culture_2d")].GetId();

            UnsignedInteger8 rgbArray[3] = { 0, 0, 0 };
            Float64 hsvArray[3] = { 0.0f, 0.0f, 0.0f };

            if (countryCosmeticData.find("color") == countryCosmeticData.end()) {
                FatalError("Color not defined at " + countryFile);
            }

            String colourData = ToLower(countryCosmeticData.at("color"));
            
            if (colourData.starts_with("rgb")) {
                colourData = colourData.substr(3);
            }
            else if (colourData.starts_with("hsv")) {
                colourData = colourData.substr(3);
            }

            Vector<String> colours = ParseStringAsArray(colourData);
            if (colours.size() != 3) { BadColourDefinition(countryFile); }
            
            //Must all be the same type (all ints or all floats)
            if (!((StringCanBecomeInteger(colours[0]) && StringCanBecomeInteger(colours[1]) && StringCanBecomeInteger(colours[2])) ||
                ((!StringCanBecomeInteger(colours[0]) && StringCanBecomeFloat(colours[0])) && (!StringCanBecomeInteger(colours[1])
                    && StringCanBecomeFloat(colours[1])) && (!StringCanBecomeInteger(colours[2]) && StringCanBecomeFloat(colours[2]))))) {
                BadColourDefinition(countryFile);
            }

            Boolean HSVbool = false;
            UnsignedInteger8 i = 0;
            for (const auto& colour : colours) {
                if (StringCanBecomeInteger(colour)) {
                    SignedInteger64 colourInt = std::stoi(colour);
                    if (colourInt > 255 || colourInt < 0) { BadColourDefinition(countryFile); }
                    rgbArray[i++] = static_cast<UnsignedInteger8>(colourInt);
                }
                else if (StringCanBecomeFloat(colour)) {
                    Float64 colourFloat = std::stod(colour);
                    if (colourFloat > 1.0f || colourFloat < 0.0f) { BadColourDefinition(countryFile); }
                    hsvArray[i++] = colourFloat;
                    HSVbool = true;
                }
                else { BadColourDefinition(countryFile); }
            }

            if (HSVbool) { HSVToRGB(rgbArray[0], rgbArray[1], rgbArray[2], hsvArray[0], hsvArray[1], hsvArray[2]); }

            countriesReturnArray.EmplaceBack(tag, rgbArray[0], rgbArray[1], rgbArray[2], cultureIndex, cultureIndex2D);
        }
    }
    countriesReturnArray.ShrinkToFit();
    return countriesReturnArray;
}

void LoadTerrainFiles() {

}