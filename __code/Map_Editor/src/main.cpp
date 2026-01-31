#include <iostream>
#include <thread>
#include <cmath>

#include "functions.hpp"
#include "data_types.hpp"
#include "load_files.hpp"
#include "bmp.hpp"
#include "write_files.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include "gui.hpp"

static void LoadBitmap(BitmapImage& bmp, const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, const String& file) { 
    bmp.LoadFile(GetGameFile(vanillaDirectory, modDirectory, modReplaceDirectories, file).string()); 
}

static void LoadFilesMain(
	const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories,

    VectorMap<GraphicalCulture>& graphicalCulturesArray,
    VectorMap<Country>& countriesArray,
    VectorMap<Building>& provinceBuildingsArray, VectorMap<Building>& stateBuildingsArray, VectorMap<BuildingSpawnPoint>& buildingSpawnPointsArray,
    VectorMap<Terrain>& landTerrainsArray, VectorMap<Terrain>& seaTerrainsArray, VectorMap<Terrain>& lakeTerrainsArray, VectorMap<GraphicalTerrain>& graphicalTerrainsArray,
    VectorMap<Resource>& resourcesArray,
    VectorMap<StateCategory>& stateCategoriesArray,
    VectorMap<Continent>& continentsArray,

    Date& defaultBookmarkDate,

	HashMap<UnsignedInteger32, UnsignedInteger16>& provinceColoursToIdMap,
	HashMap<UnsignedInteger32, UnsignedInteger16>& stateColoursToIdMap,
	HashMap<UnsignedInteger32, UnsignedInteger16>& strategicRegionColoursToIdMap,

    Vector<Province>& provincesArray,
    Vector<State>& statesArray,
    Vector<StrategicRegion>& strategicRegionsArray
    ) 

    {
    LoadGraphicalCultureFiles(vanillaDirectory, modDirectory, modReplaceDirectories, graphicalCulturesArray);
    LoadCountryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, countriesArray, graphicalCulturesArray);
    LoadBuildingFiles(vanillaDirectory, modDirectory, modReplaceDirectories, provinceBuildingsArray, stateBuildingsArray, buildingSpawnPointsArray, countriesArray);
    LoadTerrainFiles(vanillaDirectory, modDirectory, modReplaceDirectories, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray, graphicalTerrainsArray, provinceBuildingsArray);

    LoadResourceFiles(vanillaDirectory, modDirectory, modReplaceDirectories, resourcesArray);
    LoadStateCategoryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, stateCategoriesArray, provinceBuildingsArray, stateBuildingsArray);
    LoadContinentFiles(vanillaDirectory, modDirectory, modReplaceDirectories, continentsArray);
    GetDefaultDate(defaultBookmarkDate, vanillaDirectory, modDirectory, modReplaceDirectories);

    LoadProvinceFiles(vanillaDirectory, modDirectory, modReplaceDirectories, provincesArray, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray, continentsArray.Size(), provinceBuildingsArray.Size(), provinceColoursToIdMap);
    LoadStateFiles(vanillaDirectory, modDirectory, modReplaceDirectories, statesArray, provincesArray, countriesArray, provinceBuildingsArray, stateBuildingsArray,resourcesArray, stateCategoriesArray, defaultBookmarkDate, stateColoursToIdMap);
    LoadStrategicRegionFiles(vanillaDirectory, modDirectory, modReplaceDirectories, strategicRegionsArray, statesArray, provincesArray, strategicRegionColoursToIdMap, seaTerrainsArray);
}

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

    Date defaultBookmarkDate;

    HashMap<UnsignedInteger32, UnsignedInteger16> provinceColoursToIdMap, stateColoursToIdMap, strategicRegionColoursToIdMap;

    Vector<Province> provincesArray;
    Vector<State> statesArray;
    Vector<StrategicRegion> strategicRegionsArray;
    
    std::thread loadMainFilesThread(
        LoadFilesMain, 
        std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories),
        std::ref(graphicalCulturesArray),
        std::ref(countriesArray),
        std::ref(provinceBuildingsArray), std::ref(stateBuildingsArray), std::ref(buildingSpawnPointsArray),
        std::ref(landTerrainsArray), std::ref(seaTerrainsArray), std::ref(lakeTerrainsArray), std::ref(graphicalTerrainsArray),
        std::ref(resourcesArray),
        std::ref(stateCategoriesArray),
        std::ref(continentsArray),

        std::ref(defaultBookmarkDate),

        std::ref(provinceColoursToIdMap),
        std::ref(stateColoursToIdMap),
        std::ref(strategicRegionColoursToIdMap),

        std::ref(provincesArray),
        std::ref(statesArray),
        std::ref(strategicRegionsArray)
    );
    
    BitmapImage provincesBitmap(RGBA), terrainBitmap(COLOURMAP), heightmapBitmap(GREYSCALE), statesBitmap(RGBA), provinceTerrainsBitmap(RGBA);

    std::thread loadProvincesBMPThread(LoadBitmap, std::ref(provincesBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\provinces.bmp");
    std::thread loadTerrainBMPThread(LoadBitmap, std::ref(terrainBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\terrain.bmp");
    std::thread loadHeightmapBMPThread(LoadBitmap, std::ref(heightmapBitmap), std::cref(vanillaDirectory), std::cref(modDirectory), std::cref(modReplaceDirectories), "map\\heightmap.bmp");

    loadMainFilesThread.join();  loadProvincesBMPThread.join(); loadTerrainBMPThread.join(); loadHeightmapBMPThread.join();

    LoadProvincePixelData(provincesArray, provinceColoursToIdMap, statesArray, strategicRegionsArray, provincesBitmap, terrainBitmap, heightmapBitmap, statesBitmap, provinceTerrainsBitmap, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray);

    std::cout << "Files took " << GetTimeElapsedFromStart(startTime) << " to load.\n\n\n";


    Boolean writeDefinitions = false;
    Boolean writeStateHistories = false;
    Boolean writeStrategicRegions = false;

    UnsignedInteger16 windowWidth = 1200;
    UnsignedInteger16 windowHeight = 720;

    //Set up camera with possible zoom levels
    Camera2D camera = { 0 };
    camera.zoom = 0.5f;
    const UnsignedInteger8 cameraZoomLevelCount = 15;
    const Float32 cameraZoomLevels[cameraZoomLevelCount] = {
		0.125f, 0.166667f, 0.25f, 0.5f, 0.666667f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 12.0f, 16.0f, 24.0f
    };
    const String cameraZoomLevelStrings[cameraZoomLevelCount] = {
		"12.5%", "16.7%", "25.0%", "50.0%", "66.7%", "100.0%", "150.0%", "200.0%", "300.0%", "400.0%", "600.0%", "800.0%", "1200.0%", "1600.0%", "2400.0%"
    };
    SignedInteger8 currentZoomLevel = 3;

    //Right-hand panel
    UnsignedInteger16 rightHandPanelWidth = 320;
    UnsignedInteger16 halfRightHandPanelWidth = 160;
    Rectangle rightHandPanelBounds(windowWidth - rightHandPanelWidth, 0, rightHandPanelWidth, windowHeight);

    //Topbar
    const UnsignedInteger16 topbarHeight = 40;
    Rectangle topbarBounds(0, 0, windowWidth, topbarHeight);

    //Left-hand panel
    const UnsignedInteger16 leftHandPanelWidth = 40;
    Rectangle leftHandPanelBounds(0, 0, leftHandPanelWidth, windowHeight);

	//Right-hand panel buttons
    const UnsignedInteger8 buttonPadding = 6;
    const UnsignedInteger8 buttonsCount = 12;
    Rectangle rightHandButtonsArray[buttonsCount];
    std::fill(std::begin(rightHandButtonsArray), std::end(rightHandButtonsArray), Rectangle(0, 0, 0, 0));

    for (SizeT i = 0; i < buttonsCount; ++i) {
        rightHandButtonsArray[i] = Rectangle(
			rightHandPanelBounds.x + (i % 2 * halfRightHandPanelWidth) + buttonPadding,
            topbarHeight * (i / 2) + buttonPadding,
			halfRightHandPanelWidth - (buttonPadding * 2),
			topbarHeight - (buttonPadding * 2)
        );
    }

    //Map bounding box
    Rectangle mapBoundingBox(leftHandPanelWidth, topbarHeight, windowWidth - (rightHandPanelWidth + leftHandPanelWidth), windowHeight - topbarHeight);

    //Map mode dropdown box
    enum MapModeEnum : UnsignedInteger8 {
        Provinces = 0,
        ProvinceTerrains,
        States,
        Heightmap
    };

    SignedInteger32 currentMapMode = Provinces;
    Boolean mapModeDropdownBoxState = false;
    Rectangle mapModeDropdownBoxBounds(210, buttonPadding, 180, topbarHeight - (buttonPadding * 2));

    //Selected operation
	const UnsignedInteger8 operationCount = 4;
    enum SelectedOperationEnum : SignedInteger32 {
        Standard = 0,
        Pencil,
		MagicWand,
        Fill
    };
    const char* MouseIconsArray[operationCount] = {
        "#21#",
        "#22#",
		"#222#",
		"#221#"
	};
    SignedInteger32 selectedOperation = Standard;

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
    GuiLoadIcons("raygui-styles\\iconset.rgi", true);
    SetTargetFPS(60);

    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(1200, 720);

    //Define our bmp files
	const UnsignedInteger16 mapWidth = provincesBitmap.GetWidth();
	const UnsignedInteger16 mapHeight = provincesBitmap.GetHeight();
    Image provincesImage = {
        .data = provincesBitmap.GetImgDataPointer(),
        .width = mapWidth, .height = mapHeight, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    }; 
    Texture2D provincesTexture = LoadTextureFromImage(provincesImage);

    Image provinceTerrainsImage = {
        .data = provinceTerrainsBitmap.GetImgDataPointer(),
        .width = mapWidth, .height = mapHeight, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    }; 
    Texture2D provinceTerrainsTexture = LoadTextureFromImage(provinceTerrainsImage);

    Image statesImage = {
        .data = statesBitmap.GetImgDataPointer(),
        .width = mapWidth, .height = mapHeight, .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    }; 
    Texture2D statesTexture = LoadTextureFromImage(statesImage);

    Image heightmapImage = {
        .data = heightmapBitmap.GetImgDataPointer(),
        .width = mapWidth, .height = mapHeight, .mipmaps = 1,
		.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    }; 
    Texture2D heightmapTexture = LoadTextureFromImage(heightmapImage);

    //Load our fonts
    Font blenderBookFont18 = LoadFontEx("blender-font\\BlenderPro-Book.ttf", 18, NULL, 0);

    //Hide the cursor
    HideCursor();

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
            halfRightHandPanelWidth = rightHandPanelWidth / 2;

            topbarBounds = Rectangle(0, 0, windowWidth, topbarHeight);
            leftHandPanelBounds = Rectangle(0, 0, leftHandPanelWidth, windowHeight);
            mapBoundingBox = Rectangle(leftHandPanelWidth, topbarHeight, windowWidth - (rightHandPanelWidth + leftHandPanelWidth), windowHeight - topbarHeight);

            for (SizeT i = 0; i < buttonsCount; ++i) {
                rightHandButtonsArray[i] = Rectangle(
                    rightHandPanelBounds.x + (i % 2 * halfRightHandPanelWidth) + buttonPadding,
                    topbarHeight * (i / 2) + buttonPadding,
                    halfRightHandPanelWidth - (buttonPadding * 2),
                    topbarHeight - (buttonPadding * 2)
                );
            }
        }

        //Get mouse data
        mouseX = GetMouseX(); mouseY = GetMouseY(); mouseDelta = GetMouseDelta();
        if (mouseDelta.x == 0 && mouseDelta.y == 0) mouseHasMoved = false;
        else mouseHasMoved = true;


        if (mouseHasMoved) {
            mouseWithinMapNow = CheckCollisionPointRec(Vector2(mouseX, mouseY), mapBoundingBox);
        }

        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE) && mouseWithinMapNow) mouseWithinMapStartingHold = true;
        else if (IsMouseButtonReleased(MOUSE_BUTTON_MIDDLE) && mouseWithinMapStartingHold) mouseWithinMapStartingHold = false;

        //Get the pixel on the map the mouse is over
        mousePositionOnMap = GetScreenToWorld2D(Vector2(mouseX, mouseY), camera);
        if (!mouseWithinMapNow || mousePositionOnMap.x < 0 || mousePositionOnMap.x > mapWidth || mousePositionOnMap.y < 0 || mousePositionOnMap.y > mapHeight) {
            mousePositionOnMapX = -1; mousePositionOnMapY = -1;
        }
        else {
            mousePositionOnMapX = static_cast<SignedInteger16>(std::floor(mousePositionOnMap.x));
            mousePositionOnMapY = static_cast<SignedInteger16>(std::floor(mousePositionOnMap.y));
        }

        //Drag w/ middle mouse button
        if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE) && mouseWithinMapStartingHold) {
            camera.target = Vector2Add(camera.target, Vector2Scale(mouseDelta, -1.0f / camera.zoom));
        }
        //Zoom w/ scroll wheel
        if (mouseWithinMapNow) {
            mouseWheelMove = GetMouseWheelMove();
            if (mouseWheelMove != 0) {
                camera.offset = Vector2(mouseX, mouseY);
                camera.target = mousePositionOnMap;

                if (mouseWheelMove < 0 && currentZoomLevel > 0) {
                    camera.zoom = cameraZoomLevels[--currentZoomLevel];
                }
                else if (mouseWheelMove > 0 && currentZoomLevel < cameraZoomLevelCount - 1) {
                    camera.zoom = cameraZoomLevels[++currentZoomLevel];
                }
            }
        }

        //Check if we should switch map mode from key input
        if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) { currentMapMode = Provinces; }
        else if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) { currentMapMode = ProvinceTerrains; }
        else if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) { currentMapMode = States; }
        else if (IsKeyPressed(KEY_FOUR) || IsKeyPressed(KEY_KP_4)) { currentMapMode = Heightmap; }

        //Change cursor based if arrow key pressed
        if (IsKeyPressed(KEY_DOWN)) selectedOperation++;
        else if (IsKeyPressed(KEY_UP)) selectedOperation--;

        if (selectedOperation < 0) { selectedOperation = operationCount - 1; }
        else if (selectedOperation > operationCount - 1) { selectedOperation = 0; }

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
                case Heightmap:
                    DrawTexture(heightmapTexture, 0, 0, WHITE);
                    break;
                default:
                    break;
            }

            if (mousePositionOnMapX != -1) {
                //Draw lines over current pixel
                DrawRectangleLinesEx(Rectangle(mousePositionOnMapX, mousePositionOnMapY, 1, 1), 1, Color(197, 197, 197, 105));
            }

        EndMode2D();

        DrawRectangleRec(rightHandPanelBounds, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawRectangleRec(topbarBounds, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawRectangleRec(leftHandPanelBounds, GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));
        DrawRectangleLinesEx(mapBoundingBox, 2, GetColor(GuiGetStyle(DEFAULT, BORDER_COLOR_NORMAL)));

        DrawTextEx(blenderBookFont18, TextFormat("Current zoom level: %s\nMouse pos: %i, %i", cameraZoomLevelStrings[currentZoomLevel].c_str(), mousePositionOnMapX, mousePositionOnMapY), Vector2(2, 2), 18, 0, WHITE);


        //Buttons/inputs
        if (GuiDropdownBox(mapModeDropdownBoxBounds, "(1) Provinces\n(2) Province Terrains\n(3) States\n(4) Heightmap", &currentMapMode, mapModeDropdownBoxState)) { mapModeDropdownBoxState = !mapModeDropdownBoxState; }

        if (GuiButton(rightHandButtonsArray[0], "State-based province colours")) { 
            SetProvinceColoursBasedOnStateColour(provincesArray, provinceColoursToIdMap, provincesBitmap, statesArray, provincesTexture);
            writeDefinitions = true; 
        }
        if (GuiButton(rightHandButtonsArray[1], "Random province colours")) { 
            SetProvinceColoursToRandom(provincesArray, provinceColoursToIdMap, provincesBitmap, statesArray, provincesTexture); 
            writeDefinitions = true;
        }
        
        GuiToggleGroup(Rectangle(6, 40, 28, 28), "#21#\n#22#\n#222#\n#221#", &selectedOperation);
        
		//Draw cursor, must be the last thing drawn
        GuiLabel(Rectangle(mouseX, mouseY, 16, 16), MouseIconsArray[selectedOperation]);

        EndDrawing();
    }
    CloseWindow();


    if (std::filesystem::exists("out") && std::filesystem::is_directory("out")) { std::filesystem::remove_all("out"); }
    std::filesystem::create_directory("out");
    std::filesystem::create_directory("out\\common");
    std::filesystem::create_directory("out\\common\\scripted_effects");
    std::filesystem::create_directory("out\\map");
    std::filesystem::create_directory("out\\map\\strategicregions");
    std::filesystem::create_directory("out\\history");
    std::filesystem::create_directory("out\\history\\states");
    std::filesystem::create_directory("out\\localisation");
    std::filesystem::create_directory("out\\localisation\\english");

    for (auto& state : statesArray) {
        state.SortProvinces();
    }
    for (auto& strategicRegion : strategicRegionsArray) {
        strategicRegion.SortProvinces();
        strategicRegion.SortStates();
    }

    WriteStateAndStrategicRegionColours(statesArray, strategicRegionsArray);
    WriteNames(provincesArray, statesArray);

    if (writeDefinitions) {
        WriteProvinceDefinitions(provincesArray, landTerrainsArray, seaTerrainsArray, lakeTerrainsArray);
    }

	//statesBitmap.PrintBitmapFile("out\\states.bmp");
}
