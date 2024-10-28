#pragma once

#include <string>
#include <iostream>
#include <cstdint>
#include <vector>
#include <iomanip>
#include <filesystem>

std::string returnStringBetweenBrackets(const std::string& fullStr, const std::string& str);
std::string removeStringBetweenBrackets(const std::string& fullStr, const std::string& str);
std::string rgbToHex(uint8_t red, uint8_t green, uint8_t blue);
std::vector<uint32_t> returnRandomStateRGBValues(const uint16_t noOfStates);

int returnWindowXPixels();
int returnWindowYPixels();

bool modFileReplacesFolder(const std::filesystem::path& modPath, std::string folder);
std::vector<std::filesystem::path> findFilesToLoad(std::string folder, std::filesystem::path vanillaGamePath, std::filesystem::path modPath);
std::vector<std::filesystem::path> findFilesToLoadIncludeSubDirectories(std::string folder, std::filesystem::path vanillaGamePath, std::filesystem::path modPath);
std::string returnTXTFileAsStringNoHashes(const std::filesystem::path& path);

bool isInt(const std::string& str);
bool isFloat(const std::string& str);
int doubleToIntEightBit(double d);