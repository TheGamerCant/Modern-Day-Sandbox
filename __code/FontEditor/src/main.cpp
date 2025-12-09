#include <iostream>
#include "data_types.hpp"

#include "raylib.h"

#include "rlgl.h"
#include "raymath.h"

// Add codepoint range to existing font
static void AddCodepointRange(Font* font, const char* fontPath, const int fontSize, int start, int stop)
{
    int rangeSize = stop - start + 1;
    int currentRangeSize = font->glyphCount;

    // TODO: Load glyphs from provided vector font (if available),
    // add them to existing font, regenerating font image and texture

    int updatedCodepointCount = currentRangeSize + rangeSize;
    int* updatedCodepoints = (int*)RL_CALLOC(updatedCodepointCount, sizeof(int));

    // Get current codepoint list
    for (int i = 0; i < currentRangeSize; i++) updatedCodepoints[i] = font->glyphs[i].value;

    // Add new codepoints to list (provided range)
    for (int i = currentRangeSize; i < updatedCodepointCount; i++)
        updatedCodepoints[i] = start + (i - currentRangeSize);

    UnloadFont(*font);
    *font = LoadFontEx(fontPath, fontSize, updatedCodepoints, updatedCodepointCount);
    RL_FREE(updatedCodepoints);
}

static void LoadCodepoints(Font& font, const char* fontPath, const int fontSize) {
    //Latin
    AddCodepointRange(&font, fontPath, fontSize, 0xa1, 0x17f);
    AddCodepointRange(&font, fontPath, fontSize, 0x192, 0x192);
    AddCodepointRange(&font, fontPath, fontSize, 0x218, 0x219);
    AddCodepointRange(&font, fontPath, fontSize, 0x2c6, 0x2c7);
    AddCodepointRange(&font, fontPath, fontSize, 0x2d8, 0x2dd);

    //Cyrillic
    AddCodepointRange(&font, fontPath, fontSize, 0x401, 0x40c);
    AddCodepointRange(&font, fontPath, fontSize, 0x40e, 0x44f);
    AddCodepointRange(&font, fontPath, fontSize, 0x451, 0x45c);
    AddCodepointRange(&font, fontPath, fontSize, 0x45e, 0x45f);
    AddCodepointRange(&font, fontPath, fontSize, 0x490, 0x491);

    //Punctuation
    AddCodepointRange(&font, fontPath, fontSize, 0x2013, 0x2014);
    AddCodepointRange(&font, fontPath, fontSize, 0x2018, 0x201a);
    AddCodepointRange(&font, fontPath, fontSize, 0x201c, 0x201e);
    AddCodepointRange(&font, fontPath, fontSize, 0x2026, 0x2026);
    AddCodepointRange(&font, fontPath, fontSize, 0x2030, 0x2030);
    AddCodepointRange(&font, fontPath, fontSize, 0x2039, 0x203a);
    AddCodepointRange(&font, fontPath, fontSize, 0x20ac, 0x20ac);
}

static HashMap<UnsignedInteger32, Letter> GetLetterMapFromFont(const Font& font) {
    HashMap<UnsignedInteger32, Letter> letterMap;
    for (SizeT i = 0; i < font.glyphCount; ++i) {
        letterMap[font.glyphs[i].value] = Letter{
            static_cast<UnsignedInteger16>(font.baseSize),
            reinterpret_cast<const Char*>(font.glyphs[i].image.data),
            static_cast<UnsignedInteger16>(font.recs[i].width),
            static_cast<UnsignedInteger16>(font.recs[i].height),
            static_cast<SignedInteger16>(font.glyphs[i].offsetX),
            static_cast<SignedInteger16>(font.glyphs[i].offsetY),
            static_cast<SignedInteger16>(font.glyphs[i].advanceX)
        };
    }
    return letterMap;
}

static Letter SubtractLetters(const Letter& a, const Letter& b) {

}

int main(void)
{
    Path modPath = std::filesystem::current_path().parent_path().parent_path();
    Path fontsPath = modPath / "gfx\\fonts";

    String blenderBoldPath = (fontsPath / "blender_font\\BlenderPro-Bold.ttf").string();

    const int screenWidth = 800;
    const int screenHeight = 450;

    SetTraceLogLevel(LOG_NONE);
    InitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera mouse zoom");

    Font blenderBold24 = LoadFontEx(blenderBoldPath.c_str(), 100, NULL, 0);
    LoadCodepoints(blenderBold24, blenderBoldPath.c_str(), 100);

	HashMap<UnsignedInteger32, Letter> blenderBold24Letters = GetLetterMapFromFont(blenderBold24);

    for (const auto& [unicode, letter] : blenderBold24Letters) {
        std::cout << unicode << " - " << letter.offsetY << "\n";
    }

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    Vector2 mousePositionVector = {0, 0};

    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    const char testText[] = "Hello World!";

    // Main game loop
    while (!WindowShouldClose())
    {
        mousePositionVector = GetMousePosition();
        // Translate based on mouse right click
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            Vector2 delta = GetMouseDelta();
            delta = Vector2Scale(delta, -1.0f / camera.zoom);
            camera.target = Vector2Add(camera.target, delta);
        }

        float wheel = GetMouseWheelMove();
        if (wheel != 0)
        {
            Vector2 mouseWorldPos = GetScreenToWorld2D(mousePositionVector, camera);

            camera.offset = mousePositionVector;
            camera.target = mouseWorldPos;

            float scale = 0.2f * wheel;
            camera.zoom = Clamp(expf(logf(camera.zoom) + scale), 0.125f, 64.0f);
        }
        BeginDrawing();
        ClearBackground(GREEN);

        BeginMode2D(camera);
            //DrawTextEx(BlenderBold24, testText, Vector2(0, 0), 24.0f, 2, BLACK);
            DrawTexture(blenderBold24.texture, 0, 0, BLACK);

        EndMode2D();

        EndDrawing();
    }

    CloseWindow();


    return 0;
}