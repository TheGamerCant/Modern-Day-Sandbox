#include "func.hpp"
#include <random>
#include <chrono>
#include <algorithm>
#include <windows.h>
#include <cctype>
#include <regex>
#include <fstream>

std::string returnStringBetweenBrackets(const std::string& fullStr, const std::string& str) {
    size_t startBracketPos = fullStr.find(str);
    if (startBracketPos == std::string::npos) return ""; // Return empty if str not found

    std::string substring = fullStr.substr(startBracketPos);
    startBracketPos = substring.find("{");
    if (startBracketPos == std::string::npos) return ""; // Return empty if '{' not found

    substring = substring.substr(startBracketPos + 1);

    uint8_t bracketBalance = 1;
    size_t i = 0;

    while (bracketBalance != 0 && i < substring.size()) {
        if (substring[i] == '{') {
            bracketBalance++;
        }
        else if (substring[i] == '}') {
            bracketBalance--;
        }
        i++;
    }

    return substring.substr(0, i - 1);
}

std::string removeStringBetweenBrackets(const std::string& fullStr, const std::string& str) {
    size_t startBracketPos = fullStr.find(str);
    if (startBracketPos == std::string::npos) return fullStr; // Return original if str not found

    std::string fullStrOriginal = fullStr.substr(0, startBracketPos);
    std::string fullStr2 = fullStr.substr(startBracketPos);

    startBracketPos = fullStr2.find("{");
    if (startBracketPos == std::string::npos) return fullStrOriginal; // Return original if '{' not found

    std::string fullStr3 = fullStr2.substr(startBracketPos + 1);

    uint8_t  bracketBalance = 1;
    size_t i = 0;

    while (bracketBalance != 0 && i < fullStr2.size()) {
        if (fullStr3[i] == '{') {
            bracketBalance++;
        }
        else if (fullStr3[i] == '}') {
            bracketBalance--;
        }
        i++;
    }

    std::string stringToReturn = fullStrOriginal + fullStr3.substr(i);
    return stringToReturn;
}

std::string rgbToHex(uint8_t red, uint8_t green, uint8_t blue) {
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%02X%02X%02X", red, green, blue);
    std::string buffer2 = std::string(buffer);
    return buffer2;
}

std::vector<uint32_t> returnRandomStateRGBValues(const uint16_t noOfStates) {
    std::vector<uint32_t> rgbValues(noOfStates, 0);

    for (int i = 0; i < noOfStates; ++i) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 generator(seed);
        std::uniform_int_distribution<uint32_t> distribution(1, 16777215);

        uint32_t t = distribution(generator);
        if ((std::find(rgbValues.begin(), rgbValues.end(), t) == rgbValues.end()))
        {
            rgbValues[i] = t;
        }
    }

    return rgbValues;
}

int returnWindowXPixels() {
    return GetSystemMetrics(SM_CXSCREEN);
}

int returnWindowYPixels() {
    return GetSystemMetrics(SM_CYSCREEN);
}
static std::string strToRegexStr(const std::string& path) {
    std::string regexString;

    for (size_t i = 0; i < path.length(); ++i) {
        if (path[i] == '/') {
            regexString += "[\\\\/]";
        }
        else {
            regexString += path[i];
        }
    }

    regexString = R"(replace_path\s*=\s*")" + regexString + R"(")";

    return regexString;
}

bool modFileReplacesFolder(const std::filesystem::path& modPath, std::string folder) {
    for (const auto& file : std::filesystem::directory_iterator(modPath)) {
        if (file.is_regular_file() && file.path().extension() == ".mod") {
            std::ifstream currentFile(file.path());
            std::string fileContent((std::istreambuf_iterator<char>(currentFile)), std::istreambuf_iterator<char>());
            std::regex whiteSpaceRegex("\\s+");
            fileContent = regex_replace(fileContent, whiteSpaceRegex, " ");

            std::regex replacePathRegex(strToRegexStr(folder));
            return regex_search(fileContent, replacePathRegex);
        }
    }

    return false;
}

std::vector<std::filesystem::path> findFilesToLoad(std::string folder, std::filesystem::path vanillaGamePath, std::filesystem::path modPath) {
    std::vector<std::filesystem::path> tArray;
    bool replacePathBool = modFileReplacesFolder(modPath, folder);

    std::regex forwardSlashRegex("/");
    folder = regex_replace(folder, forwardSlashRegex, "\\");

    vanillaGamePath = vanillaGamePath / folder;
    modPath = modPath / folder;

    //    std::cout << vanillaGamePath << "\n" << modPath << "\n";


    if (replacePathBool && std::filesystem::exists(modPath) && std::filesystem::is_directory(modPath)) {
        for (const auto& file : std::filesystem::directory_iterator(modPath)) {
            if (file.is_regular_file() && file.path().extension() == ".txt") {
                tArray.emplace_back(file.path());
            }
        }
    }
    else if (!replacePathBool && std::filesystem::exists(modPath) && std::filesystem::is_directory(modPath)) {
        for (const auto& file : std::filesystem::directory_iterator(vanillaGamePath)) {
            if (file.is_regular_file() && file.path().extension() == ".txt") {

                std::string pathStr = file.path().string();
                std::istringstream ss(pathStr);
                std::string token;
                std::vector<std::string> tokens;

                while (getline(ss, token, '\\')) { tokens.push_back(token); }

                int tokenSize = tokens.size();
                tokenSize--;
                std::filesystem::path fileToCheckFor = modPath / tokens[tokenSize];
                if (!std::filesystem::exists(fileToCheckFor)) {
                    tArray.emplace_back(file.path());
                }
            }
        }

        for (const auto& file : std::filesystem::directory_iterator(modPath)) {
            if (file.is_regular_file() && file.path().extension() == ".txt") {
                tArray.emplace_back(file.path());
            }
        }
    }
    else {
        for (const auto& file : std::filesystem::directory_iterator(vanillaGamePath)) {
            tArray.emplace_back(file.path());
        }
    }

    return tArray;
}

std::string returnTXTFileAsStringNoHashes(const std::filesystem::path& path) {
    std::ifstream currentFile(path);

    std::string line;
    std::string fileContent;
    while (getline(currentFile, line)) {
        size_t hashPos = line.find('#');
        if (hashPos != std::string::npos) {
            line = line.substr(0, hashPos);
        }
        fileContent += line;
    }

    std::regex whiteSpaceRegex("\\s+");
    fileContent = regex_replace(fileContent, whiteSpaceRegex, " ");
    return fileContent;
}

bool isInt(const std::string& str) {
    std::string::const_iterator it = str.begin();
    while (it != str.end() && std::isdigit(*it)) ++it;
    return !str.empty() && it == str.end();
}

bool isFloat(const std::string& str) {
    if (str.empty()) return false;
    std::istringstream iss(str);
    float val;
    char extra;
    if (!(iss >> val)) return false;
    return !(iss >> extra);
}

int doubleToIntEightBit(double d) {
    return (int)d * 255;
}