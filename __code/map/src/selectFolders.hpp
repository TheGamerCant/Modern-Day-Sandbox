#pragma once

#include <iostream>
#include <string>
#include <filesystem>

std::string SelectFolder();

void returnDirectoriesFromGUI(std::filesystem::path& modDirectory, std::filesystem::path& vanillaDirectory);

void returnModAndVanillaDirectories(std::filesystem::path& modDirectory, std::filesystem::path& vanillaDirectory);