#include <filesystem>
#include <fstream>

#include "selectFolders.hpp"
#include "raylib.h"

static void removeTrailingNullChar(std::string& str) {
    if (!str.empty() && str.back() == '\0') {
        str.pop_back();  // Removes the last character from the string
    }
}

void returnDirectoriesFromGUI(std::filesystem::path& modDirectory, std::filesystem::path& vanillaDirectory) {
    const int screenWidth = 910;
    const int screenHeight = 170;

    Rectangle chooseModDirectoryRect = { 700, 10, 200, 20 };
    Rectangle chooseVanillaDirectoryRect = { 700, 40, 200, 20 };
    Rectangle endProgramRect = { 700, 70, 200, 60 };

    Vector2 mousePos = { 0.0f, 0.0f };

    SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "Select folders");
    SetExitKey(KEY_NULL);
    bool allowClose = false;
    bool exitWindow = false;

    while (!exitWindow)
    {
        mousePos = GetMousePosition();
        allowClose = std::filesystem::is_directory(modDirectory) && std::filesystem::is_directory(vanillaDirectory);

        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText("Mod directory:", 10, 10, 20, LIGHTGRAY);
        DrawText("Vanilla directory", 10, 70, 20, LIGHTGRAY);

        DrawText(TextFormat("%s", modDirectory.c_str()), 10, 32, 13, LIGHTGRAY);
        DrawText(TextFormat("%s", vanillaDirectory.c_str()), 10, 92, 13, LIGHTGRAY);

        DrawRectangleRec(chooseModDirectoryRect, LIGHTGRAY);
        DrawRectangleRec(chooseVanillaDirectoryRect, LIGHTGRAY);

        DrawText("Choose folder", 728, 10, 20, WHITE);
        DrawText("Choose folder", 728, 40, 20, WHITE);

        DrawText("These can be defined in 'file_directories.txt' to\navoid having to input them every time you run this program", 10, 132, 20, LIGHTGRAY);

        if (allowClose) {
            DrawRectangleRec(endProgramRect, LIGHTGRAY);
        }
        else {
            DrawRectangleRec(endProgramRect, BLACK);
        }
        DrawText("OK", 787, 89, 20, WHITE);

        EndDrawing();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {

            if (CheckCollisionPointRec(mousePos, chooseModDirectoryRect)) {
                std::string modDirString = SelectFolder();
                removeTrailingNullChar(modDirString);
                modDirectory = modDirString;
            }
            else if (CheckCollisionPointRec(mousePos, chooseVanillaDirectoryRect)) {
                std::string vanillaDirString = SelectFolder();
                removeTrailingNullChar(vanillaDirString);
                vanillaDirectory = vanillaDirString;
            }
            else if (allowClose && CheckCollisionPointRec(mousePos, endProgramRect)) {
                exitWindow = true;
            }
        }
    }
    CloseWindow();

    //Write file
    std::filesystem::path currentDirectory = std::filesystem::current_path();
    std::ofstream file("file_directories.txt", std::ios::out | std::ios::binary);

    if (file.is_open()) {      
        file << "mod_directory=\"" << modDirectory << "\"\nvanilla_directory=\"" << vanillaDirectory << "\"";
        file.close();
    }
}