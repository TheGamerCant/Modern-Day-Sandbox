#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "selectFolders.hpp"
#include "mapTypes.hpp"
#include "loadMap.hpp"

int main() {
	std::filesystem::path modDirectory, vanillaDirectory;
	returnModAndVanillaDirectories(modDirectory, vanillaDirectory);

	PDX::vectorStringIndexMap<PDX::terrain> terrainArray;
	PDX::vectorStringIndexMap<PDX::building> buildingArray;
	PDX::vectorStringIndexMap<PDX::resource> resourcesArray;
	PDX::vectorStringIndexMap<PDX::state_category> stateCategoryArray;
	PDX::vectorStringIndexMap<PDX::country> countriesArray;


	std::vector<PDX::state> statesArray;

	loadMap(vanillaDirectory, modDirectory, terrainArray, buildingArray, resourcesArray, stateCategoryArray, countriesArray);

	return 0;
}