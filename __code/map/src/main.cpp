#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "selectFolders.hpp"
#include "mapTypes.hpp"
#include "loadMap.hpp"


#define RAYLIBACTIVE

int main() {
	std::filesystem::path modDirectory, vanillaDirectory;
	returnModAndVanillaDirectories(modDirectory, vanillaDirectory);

	PDX::vectorStringIndexMap<PDX::terrain> terrainArray;
	PDX::vectorStringIndexMap<PDX::building> stateBuildingsArray;
	PDX::vectorStringIndexMap<PDX::building> provinceBuildingsArray;
	PDX::vectorStringIndexMap<PDX::resource> resourcesArray;
	PDX::vectorStringIndexMap<PDX::state_category> stateCategoryArray;
	PDX::vectorStringIndexMap<PDX::country> countriesArray;

	std::vector<PDX::province> provincesArray;
	std::vector<PDX::state> statesArray;
	std::vector<PDX::strategic_region> strategicRegionsArray;

	BMP::bitmapImage river ("C:\\Users\\charl\\OneDrive\\Documents\\GitHub\\Modern-Day-Sandbox\\__code\\map\\game files\\tno\\map\\rivers.bmp");
	BMP::bitmapImage heightmap ("C:\\Users\\charl\\OneDrive\\Documents\\GitHub\\Modern-Day-Sandbox\\__code\\map\\game files\\tno\\map\\heightmap.bmp");
	BMP::bitmapImage provinces ("C:\\Users\\charl\\OneDrive\\Documents\\GitHub\\Modern-Day-Sandbox\\__code\\map\\game files\\tno\\map\\provinces.bmp");

	loadMap(vanillaDirectory, modDirectory, terrainArray, stateBuildingsArray, provinceBuildingsArray, resourcesArray, stateCategoryArray, countriesArray, provincesArray, statesArray, strategicRegionsArray);

	return 0;
}