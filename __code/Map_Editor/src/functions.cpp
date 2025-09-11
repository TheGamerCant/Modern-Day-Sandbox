#include "functions.hpp"
#include <iostream>
#include <cstdlib>

//Throw an error - call with "fatalError"
[[noreturn]] void FATALERROR(const std::string& msg, const char* file, int line) {
    std::cerr << "Fatal error at " << file << ":" << line << ": " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}

//Get time elapsed since beginning of program
uint32_t getTimeElapsedFromStart(const std::chrono::steady_clock::time_point& startTime) {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    return duration.count();
}

//Convert HSV values to RGB
void HSVToRGB(uint8_t& red, uint8_t& green, uint8_t& blue, double H, double S, double V) {
    double C = V * S;
    double X = C * (1 - fabs(fmod(H * 6, 2) - 1));
    double m = V - C;

    double rPrime, gPrime, bPrime;

    if (0 <= H && H < 1.0f / 6) {
        rPrime = C; gPrime = X; bPrime = 0;
    }
    else if (1.0f / 6 <= H && H < 2.0f / 6) {
        rPrime = X; gPrime = C; bPrime = 0;
    }
    else if (2.0f / 6 <= H && H < 3.0f / 6) {
        rPrime = 0; gPrime = C; bPrime = X;
    }
    else if (3.0f / 6 <= H && H < 4.0f / 6) {
        rPrime = 0; gPrime = X; bPrime = C;
    }
    else if (4.0f / 6 <= H && H < 5.0f / 6) {
        rPrime = X; gPrime = 0; bPrime = C;
    }
    else {
        rPrime = C; gPrime = 0; bPrime = X;
    }

    //Convert the normalized [0,1] RGB to [0,255] integer RGB values
    red = static_cast<int>((rPrime + m) * 255);
    green = static_cast<int>((gPrime + m) * 255);
    blue = static_cast<int>((bPrime + m) * 255);
}

//Return text between two squiggly brackets
std::string returnStringBetweenBrackets(std::string_view fullStr, std::string_view strToFind) {
    size_t pos = fullStr.find(strToFind);
    if (pos == std::string_view::npos) return {};

    pos = fullStr.find('{', pos);
    if (pos == std::string_view::npos) return {};

    size_t start = pos + 1;
    int bracketBalance = 1;

    for (size_t i = start; i < fullStr.size(); ++i) {
        if (fullStr[i] == '{') {
            ++bracketBalance;
        }
        else if (fullStr[i] == '}') {
            --bracketBalance;
            if (bracketBalance == 0) {
                return std::string (fullStr.substr(start, i - start));
            }
        }
    }

    return "";
}

//Remove text between two squiggly brackets
std::string removeStringBetweenBrackets(std::string_view fullStr, std::string_view strToFind) {
    size_t pos = fullStr.find(strToFind);
    if (pos == std::string_view::npos) return std::string(fullStr);

    size_t open = fullStr.find('{', pos);
    if (open == std::string_view::npos) return std::string(fullStr);

    int bracketBalance = 1;
    size_t close = open + 1;

    for (; close < fullStr.size(); ++close) {
        if (fullStr[close] == '{') {
            ++bracketBalance;
        }
        else if (fullStr[close] == '}') {
            --bracketBalance;
            if (bracketBalance == 0) {
                std::string result;
                result.reserve(fullStr.size());
                result.append(fullStr.substr(0, pos));
                result.append(fullStr.substr(close + 1));
                return result;
            }
        }
    }

    return std::string(fullStr);
}


//Return all files of the specified types in a vector
std::vector<std::filesystem::path> getGameFiles(const std::filesystem::path& vanillaDirectory, const std::filesystem::path& modDirectory,
    const std::vector<std::string>& modReplaceDirectories, std::string path, const std::vector<std::string>& fileTypes) {
    //Create our return vector, reserve some space
    std::vector<std::filesystem::path> filesReturnVector;

    //Ensure the path string is formatted correctly
    for (char& c : path) { if (c == '/') { c = '\\'; } }

    //Does the .mod file have a replace_path argument for this directory
    bool modReplacesDirectory = std::find(modReplaceDirectories.begin(), modReplaceDirectories.end(), path) != modReplaceDirectories.end();

    //Vanilla directory of the files we want to get
    std::filesystem::path vanillaFolder = vanillaDirectory / path;
    //Mod directory of the files we want to get
    std::filesystem::path modFolder = modDirectory / path;

    //Does the mod folder exist
    bool modFolderExists = std::filesystem::exists(modFolder) && std::filesystem::is_directory(modFolder);

    //If the mod tries to replace the directory but the directory does not exist end the program
    if (modReplacesDirectory && !(modFolderExists)) { fatalError("Mod replaces " + path + " but " + modFolder.string() + " does not exist."); }

    //If the mod replaces that directory load from there
    if (modReplacesDirectory && modFolderExists) {
        //Count files in directory and reserve
        size_t fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(modFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            //If the file type is in fileTypes vector add it to our return vector
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //If the mod does not replace the directory and there is no equivilent directory in our mod OR if it does exist but is empty, load solely from vanilla
    else if (!modReplacesDirectory && (!modFolderExists || (modFolderExists && std::filesystem::is_empty(modFolder)))) {
        size_t fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //Otherwise load from vanilla unless the same file exists in our mod, then load all mod files
    else {
        //Get an estimate by counting files in the mod folder then times by 1.5
        size_t fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(modFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount * 1.5f);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
                //Create a hypothetical file to check for in our mod folder - if it exists, don't add it
                std::filesystem::path fileToCheckFor = modDirectory / path / file.path().filename();
                if (!std::filesystem::exists(fileToCheckFor)) {
                    filesReturnVector.emplace_back(file.path());
                }
            }
        }

        //Now load all mod files
        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }

    //Return our files
    return filesReturnVector;
}