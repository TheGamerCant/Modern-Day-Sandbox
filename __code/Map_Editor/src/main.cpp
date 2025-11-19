#include <iostream>
#include <thread>
#include <cmath>

#include "functions.hpp"
#include "data_types.hpp"
#include "load_files.hpp"
#include "bmp.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

static void LoadBitmap(BitmapImage& bmp, const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, const String& file) { bmp.LoadFile(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, file).string()); }

int main()
{
    Timestamp startTime = std::chrono::high_resolution_clock::now();

    Path vanillaDirectory, modDirectory; Vector<String> modReplaceDirectories;
    LoadFileDirectories(vanillaDirectory, modDirectory, modReplaceDirectories);

    VectorMap<GraphicalCulture> graphicalCulturesArray;
    VectorMap<Country> countriesArray;
    VectorMap<Building> provinceBuildingsArray, stateBuildingsArray; VectorMap<BuildingSpawnPoint> buildingSpawnPointsArray;
    VectorMap<Terrain> landTerrainsArray, seaTerrainsArray, lakeTerrainsArray; VectorMap<GraphicalTerrain> graphicalTerrainsArray;
    VectorMap<Resource> resourcesArray; 
    VectorMap<StateCategory> stateCategoriesArray;
    VectorMap<Continent> continentsArray;

    LoadGraphicalCultureFiles(vanillaDirectory, modDirectory, modReplaceDirectories, graphicalCulturesArray);
    LoadCountryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, countriesArray, graphicalCulturesArray);
    LoadBuildingFiles(vanillaDirectory, modDirectory, modReplaceDirectories, provinceBuildingsArray, stateBuildingsArray, buildingSpawnPointsArray, countriesArray);
    LoadTerrainFiles(vanillaDirectory, modDirectory, modReplaceDirectories, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray, graphicalTerrainsArray, provinceBuildingsArray);

    //You could put these into std::threads at the top and then join them after but it's so negligible there's no point
    LoadResourceFiles(vanillaDirectory, modDirectory, modReplaceDirectories, resourcesArray);
    LoadStateCategoryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, stateCategoriesArray);
    LoadContinentFiles(vanillaDirectory, modDirectory, modReplaceDirectories, continentsArray);
    Date defaultBookmarkDate = GetDefaultDate(vanillaDirectory, modDirectory, modReplaceDirectories);

    HashMap<UnsignedInteger32, UnsignedInteger16> provinceColoursToIdMap, stateColoursToIdMap, strategicRegionColoursToIdMap;

    Vector<Province> provincesArray;
    LoadProvinceFiles(vanillaDirectory, modDirectory, modReplaceDirectories, provincesArray, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray, continentsArray.Size(), provinceBuildingsArray.Size(),
        provinceColoursToIdMap);

    Vector<State> statesArray;
    LoadStateFiles(vanillaDirectory, modDirectory, modReplaceDirectories, statesArray, provincesArray, countriesArray, provinceBuildingsArray, stateBuildingsArray,
        resourcesArray, stateCategoriesArray, defaultBookmarkDate, stateColoursToIdMap);

    Vector<StrategicRegion> strategicRegionsArray;
    LoadStrategicRegionFiles(vanillaDirectory, modDirectory, modReplaceDirectories, strategicRegionsArray, statesArray, provincesArray, strategicRegionColoursToIdMap);
    
    BitmapImage provincesBitmap, terrainBitmap, heightmapBitmap, statesBitmap, provinceTerrainsBitmap;

    std::thread loadProvincesBMPThread(LoadBitmap, std::ref(provincesBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\provinces.bmp");
    std::thread loadTerrainBMPThread(LoadBitmap, std::ref(terrainBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\terrain.bmp");
    std::thread loadHeightmapBMPThread(LoadBitmap, std::ref(heightmapBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\heightmap.bmp");

    loadProvincesBMPThread.join(); loadTerrainBMPThread.join(); loadHeightmapBMPThread.join();

    LoadProvincePixelData(provincesArray, provinceColoursToIdMap, statesArray, provincesBitmap, terrainBitmap, heightmapBitmap, statesBitmap, provinceTerrainsBitmap, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray);

    std::cout << "Files took " << GetTimeElapsedFromStart(startTime) << " to load.\n\n\n";


    

    UnsignedInteger16 windowWidth = 1200;
    UnsignedInteger16 windowHeight = 720;

    //Set up camera with possible zoom levels
    Camera2D camera = { 0 };
    camera.zoom = 0.5f;
    const UnsignedInteger8 cameraZoomLevelCount = 14;
    const Float32 cameraZoomLevels[cameraZoomLevelCount] = {
		0.125f, 0.166667f, 0.25f, 0.666667f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f
    };
    const String cameraZoomLevelStrings[cameraZoomLevelCount] = {
		"12.5%", "16.7%", "25.0%", "50.0%", "66.7%", "100.0%", "150.0%", "200.0%", "300.0%", "400.0%", "600.0%", "800.0%", "1200.0%", "1600.0%"
    };
    SignedInteger8 currentZoomLevel = 3;

    //Right-hand panel
    UnsignedInteger16 rightHandPanelWidth = 320;
    Rectangle rightHandPanelBounds(windowWidth - rightHandPanelWidth, 0, rightHandPanelWidth, windowHeight);

    //Topbar
    const UnsignedInteger16 topbarHeight = 40;
    Rectangle topbarBounds(0, 0, windowWidth, topbarHeight);

    //Left-hand panel
    const UnsignedInteger16 leftHandPanelWidth = 40;
    Rectangle leftHandPanelBounds(0, 0, leftHandPanelWidth, windowHeight);

    //Map bounding box
    Rectangle mapBoundingBox(leftHandPanelWidth, topbarHeight, windowWidth - (rightHandPanelWidth + leftHandPanelWidth), windowHeight - topbarHeight);

    //Mouse data
    SignedInteger16 mouseX = 0, mouseY = 0;
    Vector2 mouseDelta(0.0f, 0.0f);
    Boolean mouseHasMoved = false;
    Float32 mouseWheelMove = 0.0f;

    //Is mouse within map or GUI 
    Boolean mouseWithinMapNow = false;
    Boolean mouseWithinMapStartingHold = false;

	//Pixel the mouse is over (-1, -1 if not on the map)
	Vector2 mousePositionOnMap(0.0f, 0.0f);
	SignedInteger16 mousePositionOnMapX = -1, mousePositionOnMapY = -1;

    //Define the window and give GPU access
    SetTraceLogLevel(LOG_NONE);
    InitWindow(windowWidth, windowHeight, "Map Editor :)");
    GuiLoadStyle("raygui-styles\\dark\\style_dark.rgs");
    SetTargetFPS(60);

    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(1200, 720);

    //Define our bmp files
	const UnsignedInteger16 mapWidth = provincesBitmap.GetWidth();
	const UnsignedInteger16 mapHeight = provincesBitmap.GetHeight();
    Image provincesImage = {
        .data = provincesBitmap.GetRgbDataPointer(),
        .width = mapWidth, .height = mapHeight, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8 
    }; 
    Texture2D provincesTexture = LoadTextureFromImage(provincesImage);

    Image provinceTerrainsImage = {
        .data = provinceTerrainsBitmap.GetRgbDataPointer(),
        .width = mapWidth, .height = mapHeight, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8 
    }; 
    Texture2D provinceTerrainsTexture = LoadTextureFromImage(provinceTerrainsImage);

    Image statesImage = {
        .data = statesBitmap.GetRgbDataPointer(),
        .width = mapWidth, .height = mapHeight, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8 
    }; 
    Texture2D statesTexture = LoadTextureFromImage(statesImage);

    //Map mode dropdown box
    enum MapModeEnum : UnsignedInteger8 {
        Provinces = 0,
		ProvinceTerrains,
        States
    };

    SignedInteger32 currentMapMode = Provinces;
    Boolean mapModeDropdownBoxState = false;
	Rectangle mapModeDropdownBoxBounds(210, 6, 180, topbarHeight - 12);
   
    //Load our fonts
    Font blenderBookFont18 = LoadFontEx("blender-font\\BlenderPro-Book.ttf", 18, NULL, 0);

    //Main loop
    while (!WindowShouldClose()) {
        //Get GUI positions if we changed the window size
        if (IsWindowResized()) {
            windowWidth = GetRenderWidth();
            windowHeight = GetRenderHeight();

            rightHandPanelWidth = windowWidth * 0.4f;
            if (rightHandPanelWidth < 300) rightHandPanelWidth = 300;
            else if (rightHandPanelWidth > 600) rightHandPanelWidth = 600;
            rightHandPanelBounds = Rectangle(windowWidth - rightHandPanelWidth, 0, rightHandPanelWidth, windowHeight);

            topbarBounds = Rectangle(0, 0, windowWidth, topbarHeight);
            leftHandPanelBounds = Rectangle(0, 0, leftHandPanelWidth, windowHeight);
            mapBoundingBox = Rectangle(leftHandPanelWidth, topbarHeight, windowWidth - (rightHandPanelWidth + leftHandPanelWidth), windowHeight - topbarHeight);
        }

        //Get mouse data
        mouseX = GetMouseX(); mouseY = GetMouseY(); mouseDelta = GetMouseDelta();
        if (mouseDelta.x == 0 && mouseDelta.y == 0) mouseHasMoved = false;
        else mouseHasMoved = true;

        
        if (mouseHasMoved) {
            if (CheckCollisionPointRec(Vector2(mouseX, mouseY), mapBoundingBox)) { mouseWithinMapNow = true; }
            else mouseWithinMapNow = false;
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && mouseWithinMapNow) mouseWithinMapStartingHold = true;
        else if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE) && mouseWithinMapStartingHold) mouseWithinMapStartingHold = false;
        

        //Drag w/ middle mouse button
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && mouseWithinMapStartingHold) { camera.target = Vector2Add(camera.target, Vector2Scale(mouseDelta, -1.0f / camera.zoom)); }
        //Zoom w/ scroll wheel
        if (mouseWithinMapNow) {
            mouseWheelMove = GetMouseWheelMove();
            if (mouseWheelMove != 0) {
                Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
                camera.offset = GetMousePosition();
                camera.target = mouseWorldPos;

                if (mouseWheelMove < 0 && currentZoomLevel > 0) {
                    --currentZoomLevel;
                    camera.zoom = cameraZoomLevels[currentZoomLevel];
                }
                else if (mouseWheelMove > 0 && currentZoomLevel < cameraZoomLevelCount - 1) {
                    ++currentZoomLevel;
                    camera.zoom = cameraZoomLevels[currentZoomLevel];
                }
            }
        }

		//Get the pixel on the map the mouse is over
        mousePositionOnMap = GetScreenToWorld2D(Vector2(mouseX, mouseY), camera);
        if (!mouseWithinMapNow || mousePositionOnMap.x < 0 || mousePositionOnMap.x > mapWidth || mousePositionOnMap.y < 0 || mousePositionOnMap.y > mapHeight) {
			mousePositionOnMapX = -1; mousePositionOnMapY = -1;
        }
        else {
			mousePositionOnMapX = static_cast<SignedInteger16>(std::floor(mousePositionOnMap.x));
			mousePositionOnMapY = static_cast<SignedInteger16>(std::floor(mousePositionOnMap.y));
        }

        //Drawing
        BeginDrawing();
        ClearBackground(Color(22, 26, 31, 255));

        BeginMode2D(camera);

            //Draw map
            switch (currentMapMode) {
                case Provinces:
                    DrawTexture(provincesTexture, 0, 0, WHITE);
                    break;
                case ProvinceTerrains:
                    DrawTexture(provinceTerrainsTexture, 0, 0, WHITE);
                    break;
                case States:
                    DrawTexture(statesTexture, 0, 0, WHITE);
                    break;
                default:
                    break;
            }

            if (mousePositionOnMapX != -1) {
                //Draw lines over current pixel
                DrawRectangleLinesEx(Rectangle(mousePositionOnMapX, mousePositionOnMapY, 1, 1), 1, Color(150, 150, 150, 127));
            }

        EndMode2D();

        DrawRectangleRec(rightHandPanelBounds, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawRectangleRec(topbarBounds, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawRectangleRec(leftHandPanelBounds, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        DrawRectangleLinesEx(mapBoundingBox, 2, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));

        DrawTextEx(blenderBookFont18, TextFormat("Current zoom level: %s\nMouse pos: %i, %i", cameraZoomLevelStrings[currentZoomLevel].c_str(), mousePositionOnMapX, mousePositionOnMapY), Vector2(2, 2), 18, 0, WHITE);

		if (GuiDropdownBox(mapModeDropdownBoxBounds, "Provinces\nProvince Terrains\nStates", &currentMapMode, mapModeDropdownBoxState)) mapModeDropdownBoxState = !mapModeDropdownBoxState;

        EndDrawing();
    }
    CloseWindow();

	statesBitmap.PrintBitmapFile("out\\states.bmp");
	provinceTerrainsBitmap.PrintBitmapFile("out\\province_terrains.bmp");
}
