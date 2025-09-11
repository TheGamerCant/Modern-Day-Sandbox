#pragma once

#include <string>
#include <iostream>
#include <filesystem>
#include <vector>

void getModAndVanillaDirectories(std::filesystem::path& vanillaDirectory, std::filesystem::path& modDirectory, int& cores,
    std::vector<std::string>& modReplaceDirectories, std::chrono::steady_clock::time_point& startTime);