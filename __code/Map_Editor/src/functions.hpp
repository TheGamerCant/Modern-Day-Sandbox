#pragma once

#include <string>
#include <chrono>
#include <cstdint>
#include <string_view>
#include <filesystem>

[[noreturn]] void FATALERROR(const std::string& msg, const char* file, int line);
#define fatalError(msg) FATALERROR(msg, __FILE__, __LINE__)

uint32_t getTimeElapsedFromStart(const std::chrono::steady_clock::time_point& startTime);

void HSVToRGB(uint8_t& red, uint8_t& green, uint8_t& blue, double H, double S, double V);

std::string returnStringBetweenBrackets(std::string_view fullStr, std::string_view strToFind);
std::string removeStringBetweenBrackets(std::string_view fullStr, std::string_view strToFind);

std::vector<std::filesystem::path> getGameFiles(const std::filesystem::path& vanillaDirectory, const std::filesystem::path& modDirectory,
	const std::vector<std::string>& modReplaceDirectories, std::string path, const std::vector<std::string>& fileTypes);