#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <filesystem>
#include <vector>


[[noreturn]] void FATALERROR(const std::string& msg, const char* file, int line);
#define fatalError(msg) FATALERROR(msg, __FILE__, __LINE__)

uint32_t getTimeElapsedFromStart(const std::chrono::steady_clock::time_point& startTime);

void HSVToRGB(uint8_t& red, uint8_t& green, uint8_t& blue, double H, double S, double V);

std::string returnStringBetweenBrackets(std::string_view fullStr, std::string_view strToFind);
std::string removeStringBetweenBrackets(std::string_view fullStr, std::string_view strToFind);

std::vector<std::filesystem::path> getGameFiles(const std::filesystem::path& vanillaDirectory, const std::filesystem::path& modDirectory,
	const std::vector<std::string>& modReplaceDirectories, std::string path, const std::vector<std::string>& fileTypes);

//Take a vector and divide it into a 2d vector based on our number of cores
template <typename dateType>
std::vector<std::vector<dateType>> divideVector(const std::vector<dateType>& inputVector, std::size_t cores) {
    std::vector<std::vector<dateType>> returnVector;

    if (cores == 0 || inputVector.empty()) return returnVector;

    std::size_t noOfEntries = inputVector.size();
    std::size_t groups = std::min<std::size_t>(cores, noOfEntries);
    std::size_t chunkSize = noOfEntries / cores;
    std::size_t remainder = noOfEntries % cores;

    auto it = inputVector.begin();
    for (std::size_t i = 0; i < groups; ++i) {
        std::size_t currentChunkSize = chunkSize + (i < remainder ? 1 : 0);
        returnVector.emplace_back(it, it + currentChunkSize);
        it += currentChunkSize;
    }

    return returnVector;
}