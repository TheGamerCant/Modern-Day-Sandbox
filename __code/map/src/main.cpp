#include <iostream>
#include <string>
#include <vector>
#include <filesystem>

#include "selectFolders.hpp"
#include "mapTypes.hpp"
#include "loadMap.hpp"
#include "func.hpp"


#define RAYLIBACTIVE

#ifdef RAYLIBACTIVE

#include "raylib.h"

#endif


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

	BMP::bitmapImage provincesBMP, riversBMP, heightmapBMP, statesBMP, stateBordersBMP;

	std::unordered_map<std::tuple<uint8_t, uint8_t, uint8_t>, uint16_t, RGBHash> provinceColourToProvinceIDMap, stateColourToStateIDMap;

	loadMap(vanillaDirectory, modDirectory, terrainArray, stateBuildingsArray, provinceBuildingsArray, resourcesArray, stateCategoryArray, countriesArray,
		provincesArray, statesArray, strategicRegionsArray, provincesBMP, riversBMP, heightmapBMP, statesBMP, stateBordersBMP, provinceColourToProvinceIDMap, stateColourToStateIDMap);

	statesBMP.flipImageData();
	statesBMP.swapRBData();
	statesBMP.save("states.bmp");

	stateBordersBMP.flipImageData();
	stateBordersBMP.swapRBData();
	stateBordersBMP.save("stateborders.bmp");

	const int screenWidth = returnWindowXPixels();
	const int screenHeight = returnWindowYPixels();
	InitWindow(screenWidth, screenHeight, "TGC's Map Editor");
	ToggleFullscreen();
	const int sidebarWidth = 360;

	Image provincesImage = {
		.data = statesBMP.returnRawDataVoidPtr(),
		.width = statesBMP.GetWidth(),
		.height = statesBMP.GetHeight(),
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
	};
	Texture2D provincesTexture = LoadTextureFromImage(provincesImage);

	Vector2 mousePos;

	SetTargetFPS(60);
	while (!WindowShouldClose())
	{
		mousePos = GetMousePosition();

		BeginDrawing();

		ClearBackground(RAYWHITE);

		DrawTexture(provincesTexture, 0, 0, WHITE);

		EndDrawing();
	}

	return 0;
}