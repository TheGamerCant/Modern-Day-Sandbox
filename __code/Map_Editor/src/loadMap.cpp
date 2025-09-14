#include "loadMap.hpp"
#include "functions.hpp"

#include <future>

data_manager<terrain> loadTerrainTypes(const std::filesystem::path& vanillaDirectory, const std::filesystem::path& modDirectory, const int cores, const std::vector<std::string>& modReplaceDirectories) {
	//List of terrain files
	std::vector<std::filesystem::path> terrain_files = getGameFiles(vanillaDirectory, modDirectory, modReplaceDirectories, "common/terrain", { ".txt" });

	//Divide them up into a 2d vector
	std::vector<std::vector<std::filesystem::path>> iteration_vector = divideVector(terrain_files, cores);

	std::vector<std::future<int>> futures;

	data_manager<terrain> rv;
	return rv;
}