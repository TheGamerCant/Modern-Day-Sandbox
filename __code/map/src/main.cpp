#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "selectFolders.hpp"
#include "mapTypes.hpp"
#include "loadMap.hpp"


//#define RAYLIBACTIVE

int main() {
	std::filesystem::path modDirectory, vanillaDirectory;
	returnModAndVanillaDirectories(modDirectory, vanillaDirectory);

	PDX::vectorStringIndexMap<PDX::terrain> terrainArray;
	PDX::vectorStringIndexMap<PDX::building> stateBuildingsrray;
	PDX::vectorStringIndexMap<PDX::building> provinceBuildingsArray;
	PDX::vectorStringIndexMap<PDX::resource> resourcesArray;
	PDX::vectorStringIndexMap<PDX::state_category> stateCategoryArray;
	PDX::vectorStringIndexMap<PDX::country> countriesArray;

	std::vector<PDX::province> provincesArray;
	std::vector<PDX::state> statesArray;

	loadMap(vanillaDirectory, modDirectory, terrainArray, stateBuildingsArray, provinceBuildingsArray, resourcesArray, stateCategoryArray, countriesArray, provincesArray, statesArray);

	return 0;
}