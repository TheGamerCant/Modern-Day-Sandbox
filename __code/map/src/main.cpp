#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

#include "selectFolders.hpp"
#include "mapTypes.hpp"
#include "loadMap.hpp"
#include "func.hpp"


#define RAYLIBACTIVE

#ifdef RAYLIBACTIVE

#include "raylib.h"
#include "raymath.h"

#include "gui.h"

#endif


int main() {
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::milliseconds ms;
	typedef std::chrono::duration<float> fsec;
	auto t0 = Time::now();

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

//	provincesBMP.flipImageData();
//	provincesBMP.swapRBData();

	auto t1 = Time::now();
	fsec fs = t1 - t0;
	ms d = std::chrono::duration_cast<ms>(fs);
	std::cout << d.count() << "s\n";

#ifdef RAYLIBACTIVE
	const int screenWidth = returnWindowXPixels();
	const int screenHeight = returnWindowYPixels();
	const int mapWidth = provincesBMP.GetWidth();
	const int mapHeight = provincesBMP.GetHeight();
	const int toolbarWidth = 400;

	InitWindow(screenWidth, screenHeight, "TGC's Map Editor");
	ToggleFullscreen();
	const int sidebarWidth = 360;

	Image provincesImage = {
		.data = provincesBMP.returnRawDataVoidPtr(),
		.width = provincesBMP.GetWidth(),
		.height = provincesBMP.GetHeight(),
		.mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8
	};
	Texture2D provincesTexture = LoadTextureFromImage(provincesImage);

	Vector2 mousePos;
	bool isMouseWithinMap;
	Vector2 currentMapPixelXY;

	Camera2D camera = { 0 };
	camera.zoom = 0.4121875f;
	camera.target = { (float)((mapWidth - screenWidth) / 2), (float)((mapHeight - screenHeight) / 2) };
	unsigned int currentZoomIndex = 2;		//0.4121875f
	const float zoomArray[25] = {
		0.2373047f, 0.31640625f, 0.4121875f, 0.5625f, 0.75f, 1.0f, 1.25f, 1.5625f, 1.95315f, 2.441406f, 3.0517578f, 3.814697f, 4.68372f, 5.960464f,
		7.4505806f, 9.3132257f, 11.641532f, 14.55192f, 18.189894f, 22.737368f, 28.421709f, 35.527137f, 44.408921f, 55.511151f, 64.0f
	};

	SetTargetFPS(60);
	while (!WindowShouldClose())
	{
		mousePos = GetMousePosition();
		isMouseWithinMap = GUI::inBounds(mousePos, screenWidth, screenHeight, toolbarWidth);
		currentMapPixelXY = GetScreenToWorld2D(mousePos, camera);

		if (isMouseWithinMap) {
			// Scroll function
			if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
			{
				Vector2 delta = GetMouseDelta();
				delta = Vector2Scale(delta, -1.0f / camera.zoom);
				camera.target = Vector2Add(camera.target, delta);
			}

			//Zoom Function
			float wheel = GetMouseWheelMove();

			if (wheel != 0)
			{
				currentZoomIndex = Clamp(currentZoomIndex + wheel, 0, 24);

				camera.offset = mousePos;
				camera.target = currentMapPixelXY;
				camera.zoom = zoomArray[currentZoomIndex];
			}
		}

		BeginDrawing();

			ClearBackground(RAYWHITE);

			BeginMode2D(camera);
				DrawTexture(provincesTexture, camera.offset.x, camera.offset.y, WHITE);
			EndMode2D();

		EndDrawing();
	}
#endif
	return 0;
}