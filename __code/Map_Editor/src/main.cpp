#include <iostream>
#include <thread>

#include "functions.hpp"
#include "data_types.hpp"
#include "load_files.hpp"
#include "bmp.hpp"

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
    
    BitmapImage provincesBitmap, terrainBitmap, heightmapBitmap;

    std::thread loadProvincesBMPThread(LoadBitmap, std::ref(provincesBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\provinces.bmp");
    std::thread loadTerrainBMPThread(LoadBitmap, std::ref(terrainBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\terrain.bmp");
    std::thread loadHeightmapBMPThread(LoadBitmap, std::ref(heightmapBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\heightmap.bmp");

    loadProvincesBMPThread.join(); loadTerrainBMPThread.join(); loadHeightmapBMPThread.join();

    LoadProvincePixelData(provincesArray, provinceColoursToIdMap, provincesBitmap, terrainBitmap, heightmapBitmap);

    std::cout << "Files took " << GetTimeElapsedFromStart(startTime) << " to load.\n\n\n";


    

    //const UnsignedInteger16 screenWidth = GetScreenWidth();
    //const UnsignedInteger16 screenHeight = GetScreenHeight();

    const UnsignedInteger16 screenWidth = GetScreenWidth();
    const UnsignedInteger16 screenHeight = GetScreenHeight();


    //Set up camera with possible zoom levels
    Camera2D camera = { 0 };
    camera.zoom = 0.5f;
    const UnsignedInteger8 cameraZoomLevelCount = 10;
    const Float32 cameraZoomLevels[cameraZoomLevelCount] = {
        0.125f, 0.166667f, 0.25f, 0.666667f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f
    };
    const String cameraZoomLevelStrings[cameraZoomLevelCount] = {
        "12.5%", "16.7%", "25.0%", "50.0%", "66.7%", "100.0%", "150.0%", "200.0%", "300.0%", "400.0%"
    };
    SignedInteger8 currentZoomLevel = 3;

    //Main loop
    InitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera mouse zoom");
    GuiLoadStyle("raygui-styles\\dark\\style_dark.rgs");
    SetTargetFPS(60);
    while (!WindowShouldClose())
    {
        //Drag with mouse
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) { camera.target = Vector2Add(camera.target, Vector2Scale(GetMouseDelta(), -1.0f / camera.zoom)); }

        // Zoom based on mouse wheel
        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
            Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
            camera.offset = GetMousePosition();
            camera.target = mouseWorldPos;

            if (wheel < 0 && currentZoomLevel > 0) {
                --currentZoomLevel;
                camera.zoom = cameraZoomLevels[currentZoomLevel];
            }
            else if (wheel > 0 && currentZoomLevel < cameraZoomLevelCount - 1) {
                ++currentZoomLevel;
                camera.zoom = cameraZoomLevels[currentZoomLevel];
            }
        }

        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode2D(camera);

        // Draw the 3d grid, rotated 90 degrees and centered around 0,0
        // just so we have something in the XY plane
        rlPushMatrix();
        rlTranslatef(0, 25 * 50, 0);
        rlRotatef(90, 1, 0, 0);
        DrawGrid(100, 50);
        rlPopMatrix();

        // Draw a reference circle
        DrawCircle(GetScreenWidth() / 2, GetScreenHeight() / 2, 50, MAROON);

        EndMode2D();

        // Draw mouse reference
        //Vector2 mousePos = GetWorldToScreen2D(GetMousePosition(), camera)
        DrawCircleV(GetMousePosition(), 4, DARKGRAY);
        DrawTextEx(GetFontDefault(), TextFormat("[%i, %i]", GetMouseX(), GetMouseY()),
            Vector2Add(GetMousePosition(),Vector2(-44, -24)), 20, 2, BLACK);

        DrawText(TextFormat("Current zoom level: %s", cameraZoomLevelStrings[currentZoomLevel].c_str()), 20, 20, 20, DARKGRAY);

        EndDrawing();
    }
    CloseWindow();
}
