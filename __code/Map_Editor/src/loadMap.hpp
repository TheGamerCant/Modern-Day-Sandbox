#pragma once

#include <filesystem>
#include "mapTypes.hpp"

data_manager<terrain> loadTerrainTypes(const std::filesystem::path& vanillaDirectory, const std::filesystem::path& modDirectory, const int cores, const std::vector<std::string>& modReplaceDirectories);