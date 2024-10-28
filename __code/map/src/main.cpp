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

	BMP::bitmapImage river ("C:\\Users\\charl\\OneDrive\\Documents\\GitHub\\Modern-Day-Sandbox\\__code\\map\\game files\\tno\\map\\rivers.bmp");
	BMP::bitmapImage heightmap ("C:\\Users\\charl\\OneDrive\\Documents\\GitHub\\Modern-Day-Sandbox\\__code\\map\\game files\\tno\\map\\heightmap.bmp");
	BMP::bitmapImage provinces ("C:\\Users\\charl\\OneDrive\\Documents\\GitHub\\Modern-Day-Sandbox\\__code\\map\\game files\\tno\\map\\provinces.bmp");

	provinces.flipImageData();
	provinces.swapRBData();

//	loadMap(vanillaDirectory, modDirectory, terrainArray, stateBuildingsArray, provinceBuildingsArray, resourcesArray, stateCategoryArray, countriesArray, provincesArray, statesArray, strategicRegionsArray);

	const int screenWidth = returnWindowXPixels();
	const int screenHeight = returnWindowYPixels();
	InitWindow(screenWidth, screenHeight, "TGC's Map Editor");
	ToggleFullscreen();
	const int sidebarWidth = 360;

	Image provincesImage = {
		.data = provinces.returnData(),
		.width = provinces.GetWidth(),
		.height = provinces.GetHeight(),
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