#include <iostream>
#include <cmath>

#include "data_types.hpp"

#include "raylib.h"

#include "rlgl.h"
#include "raymath.h"

#include "dds.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

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
    AddCodepointRange(&font, fontPath, fontSize, 0xa1, 0xac);
    AddCodepointRange(&font, fontPath, fontSize, 0xae, 0x17f);
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
        UnsignedInteger8* imgDataPtr = static_cast<UnsignedInteger8*>(font.glyphs[i].image.data);
        SizeT imgDataSize = static_cast<SizeT>(font.recs[i].width) * static_cast<SizeT>(font.recs[i].height) * 2;

        letterMap[font.glyphs[i].value] = Letter{
            static_cast<UnsignedInteger32>(font.glyphs[i].value),
            static_cast<UnsignedInteger16>(font.baseSize),
            Vector<UnsignedInteger8>(imgDataPtr, imgDataPtr + imgDataSize),
            static_cast<UnsignedInteger16>(font.recs[i].width),
            static_cast<UnsignedInteger16>(font.recs[i].height),
            static_cast<Float32>(font.glyphs[i].offsetX),
            static_cast<Float32>(font.glyphs[i].offsetY),
            static_cast<Float32>(font.glyphs[i].advanceX)
        };
    }
    return letterMap;
}

static Letter LettersWithPunctuation(const Letter& letter, const Punctuation& punctuation, const UnsignedInteger32 unicodeChar) {
    if (punctuation.realWidth > letter.realWidth) {
        std::cout << "Char " << unicodeChar << " has a wider accent than character";
        return Letter();
    }

    Image letterImage = {
        .data = const_cast<UnsignedInteger8*>(letter.imgDataGreyAlpha.data()),
        .width = letter.imgWidth,
        .height = letter.imgHeight,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA
	};
    Texture2D letterTexture = LoadTextureFromImage(letterImage);

    Image punctuationImage = {
        .data = const_cast<UnsignedInteger8*>(punctuation.imgDataGreyAlpha.data()),
        .width = punctuation.imgWidth,
        .height = punctuation.imgHeight,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA
    };
    Texture2D punctuationTexture = LoadTextureFromImage(punctuationImage);

    Float32 puncXOffset = (255.0f - static_cast<Float32>(letter.leftAlpha)) + static_cast<Float32>(letter.realWidth) * 0.5f;
    puncXOffset -= (255.0f - static_cast<Float32>(punctuation.leftAlpha)) + static_cast<Float32>(punctuation.realWidth) * 0.5f;
    puncXOffset /= 255.0f;


    UnsignedInteger32 imgWidth = letter.imgWidth;
    UnsignedInteger32 imgHeight = 0;
    SignedInteger32 offsetY = 0;
    if (letter.offsetY >= punctuation.offsetY) {
		imgHeight = std::ceil(letter.imgHeight + letter.offsetY - punctuation.offsetY);
        offsetY = (punctuation.offsetY >= 0.0) ? std::ceil(punctuation.offsetY) : std::floor(punctuation.offsetY);
    }
    else {
		imgHeight = std::ceil(punctuation.imgHeight + punctuation.offsetY - letter.offsetY);
        offsetY = (letter.offsetY >= 0.0) ? std::ceil(letter.offsetY) : std::floor(letter.offsetY);
    }

    RenderTexture2D target = LoadRenderTexture(imgWidth, imgHeight);
    BeginTextureMode(target);
    ClearBackground(BLANK);
        //Punctuation is above letter
        if (letter.offsetY >= punctuation.offsetY) {
            DrawTextureV(letterTexture, Vector2(0, letter.offsetY - punctuation.offsetY), BLACK);
            DrawTextureV(punctuationTexture, Vector2(puncXOffset, 0), BLACK);
        }

        //Punctuation is below letter
        else {
            DrawTextureV(letterTexture, Vector2(0, 0), BLACK);
            DrawTextureV(punctuationTexture, Vector2(puncXOffset, punctuation.offsetY - letter.offsetY), BLACK);
        }
    EndTextureMode();

    Image img = LoadImageFromTexture(target.texture);
    ImageFlipVertical(&img);

    if (img.format != PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA) {
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA);
    }

	const SizeT imgSize = img.width * img.height * 2;
    UnsignedInteger8* imgDataPtr = static_cast<UnsignedInteger8*>(img.data);
    Vector<UnsignedInteger8>newCharImgData(imgDataPtr, imgDataPtr + imgSize);

    UnloadTexture(letterTexture);
    UnloadTexture(punctuationTexture);

    UnloadRenderTexture(target);

    //UnloadImage(letterImage);
    //UnloadImage(punctuationImage);
    //UnloadImage(img);

    return Letter(
        unicodeChar,
        letter.fontSize,
        newCharImgData,
        imgWidth,
        imgHeight,
        letter.offsetX,
        offsetY,
        letter.advanceX
    );
}

struct BmpFontEntry {
    String name;
    UnsignedInteger16 size;
    Color colour;
    UnsignedInteger16 outline;
    Float32 outlineXAdvance;
    UnsignedInteger8 opacity;
   
    BmpFontEntry() : name(""), size(0), colour(WHITE), outline(0), outlineXAdvance(0.0f), opacity(255) {}
    BmpFontEntry(const String& name, const UnsignedInteger16 size, const Color colour) : name(name), size(size), colour(colour), outline(0), outlineXAdvance(0.0f), opacity(255) {}
    BmpFontEntry(const String& name, const UnsignedInteger16 size, const Color colour, const UnsignedInteger8 opacity) :
        name(name), size(size), colour(colour), outline(0), outlineXAdvance(0.0f), opacity(opacity) {}
    BmpFontEntry(const String& name, const UnsignedInteger16 size, const Color colour, const UnsignedInteger16 outline, const Float32 outlineXAdvance, const UnsignedInteger8 opacity) :
        name(name), size(size), colour(colour), outline(outline), outlineXAdvance(outlineXAdvance), opacity(opacity) {}
};

struct FontsData {
    String dir;
    String fontName;
    Vector<BmpFontEntry> entries;

    FontsData() {}
    FontsData(const String& dir, const String& fontName, const Vector<BmpFontEntry>& entries) : dir(dir), fontName(fontName), entries(entries) {}
};

static Vector<FontsData> GetFonts() {
    return {
        FontsData{
            "blender_font\\BlenderPro-Bold.ttf", "Blender Pro Bold", {
                BmpFontEntry{"Blender_Bold_White_10", 10, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_12", 12, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_13", 13, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_14", 14, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_15", 15, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_16", 16, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_16_outline", 16, WHITE, 2, 0.5f, 255},
                BmpFontEntry{"Blender_Bold_White_18", 18, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_18_outline", 18, WHITE, 3, 0.5f, 255},
                BmpFontEntry{"Blender_Bold_White_19", 19, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_20", 20, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_20_outline", 20, WHITE, 3, 0.5f, 255},
                BmpFontEntry{"Blender_Bold_White_24", 24, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Bold_White_28", 28, WHITE, 0, 0.0f, 255},
            }
        },
        FontsData{
            "blender_font\\BlenderPro-Heavy.ttf", "Blender Pro Heavy", { 
                BmpFontEntry{"Blender_Heavy_White_18_outline", 18, WHITE, 3, 0.5f, 255},
                BmpFontEntry{"Blender_Heavy_White_22_outline", 22, WHITE, 3, 0.5f, 255},
                BmpFontEntry{"Blender_Heavy_White_24", 24, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Heavy_White_24_50pc_opacity", 24, WHITE, 0, 0.0f, 127},
                BmpFontEntry{"Blender_Heavy_White_24_outline", 24, WHITE, 3, 0.5f, 255},
                BmpFontEntry{"Blender_Heavy_White_28", 28, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Heavy_White_30", 30, WHITE, 0, 0.0f, 255},
                BmpFontEntry{"Blender_Heavy_White_32", 32, WHITE, 0, 0.0f, 255},
            }
        }
    };
}

static void ImageDropShadow(Image& src, int blurRadius, Color shadowColor) {
    Image shadow = ImageCopy(src);
    ImageFormat(&shadow, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

    unsigned char* px = (unsigned char*)shadow.data;
    int count = shadow.width * shadow.height;

    for (int i = 0; i < count; i++) {
        px[i * 4 + 0] = shadowColor.r;
        px[i * 4 + 1] = shadowColor.g;
        px[i * 4 + 2] = shadowColor.b;
        px[i * 4 + 3] = (px[i * 4 + 3] * shadowColor.a) / 255;
    }


    for (int i = 0; i < blurRadius; i++) {
        ImageResize(&shadow, shadow.width / 2, shadow.height / 2);
        ImageResize(&shadow, src.width, src.height);
    }

    Image out = GenImageColor(src.width, src.height, BLANK);

    ImageDraw(
        &out,
        shadow,
        { 0, 0, (float)shadow.width, (float)shadow.height },
        { 0, 0, (float)shadow.width, (float)shadow.height },
        WHITE
    );

    ImageDraw(
        &out,
        src,
        { 0, 0, (float)src.width, (float)src.height },
        { 0, 0, (float)src.width, (float)src.height },
        WHITE
    );

    UnloadImage(shadow);
    UnloadImage(src);

    src = out;
}

int main(void)
{
    Path modPath = std::filesystem::current_path().parent_path().parent_path();
    Path fontsPath = modPath / "gfx\\fonts";

    const int screenWidth = 800;
    const int screenHeight = 450;

    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(screenWidth, screenHeight, "raylib [core] example - 2d camera mouse zoom");

    Camera2D camera = { 0 };
    camera.zoom = 1.0f;

    Vector2 mousePositionVector = { 0, 0 };

    SetTargetFPS(60);
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    Vector<FontsData> fonts = GetFonts();

    for (const auto& [dir, fontName, entries] : fonts) {
        String fontPath = (fontsPath / dir).string();

        for (const auto& [name, size, colour, outline, outlineXAdvance, opacity] : entries) {
            Font font = LoadFontEx(fontPath.c_str(), size, NULL, 0);
            LoadCodepoints(font, fontPath.c_str(), size);

            HashMap<UnsignedInteger32, Letter> fontLetterMap = GetLetterMapFromFont(font);

            //Right accent
            Punctuation capitalAcute(fontLetterMap.at(0xd3), fontLetterMap.at(0x4f));
            Punctuation lowerAcute(fontLetterMap.at(0x0f3), fontLetterMap.at(0x6f));

            //Left accent
            Punctuation capitalGrave(fontLetterMap.at(0xd2), fontLetterMap.at(0x4f));
            Punctuation lowerGrave(fontLetterMap.at(0x0f2), fontLetterMap.at(0x6f));

            //Two dots above
            Punctuation capitalDiaeresis(fontLetterMap.at(0xd6), fontLetterMap.at(0x4f));
            Punctuation lowerDiaeresis(fontLetterMap.at(0x0f6), fontLetterMap.at(0x6f));

            //Down arrow above
            Punctuation capitalCharon(fontLetterMap.at(0x0160), fontLetterMap.at(0x53));
            Punctuation lowerCharon(fontLetterMap.at(0x0161), fontLetterMap.at(0x73));

            //Dot above
            Punctuation capitalDotAbove(fontLetterMap.at(0x017b), fontLetterMap.at(0x5a));
            Punctuation lowerDotAbove(fontLetterMap.at(0x017c), fontLetterMap.at(0x7a));

            //Dot below
            Punctuation dotBelow(fontLetterMap.at(0x0116), fontLetterMap.at(0x45));
            dotBelow.offsetY += static_cast<Float32>(dotBelow.fontSize) * 0.80f;

            //Line below
            Punctuation macronBelow(fontLetterMap.at(0x016a), fontLetterMap.at(0x55));
            macronBelow.offsetY += static_cast<Float32>(macronBelow.fontSize) * 0.80f;

            //T with dot below
            fontLetterMap[0x1e6c] = LettersWithPunctuation(fontLetterMap.at(0x54), dotBelow, 0x1e6c);
            fontLetterMap[0x1e6d] = LettersWithPunctuation(fontLetterMap.at(0x74), dotBelow, 0x1e6d);

            //T with line below
            fontLetterMap[0x1e6e] = LettersWithPunctuation(fontLetterMap.at(0x54), macronBelow, 0x1e6e);
            fontLetterMap[0x1e6f] = LettersWithPunctuation(fontLetterMap.at(0x74), macronBelow, 0x1e6f);

            //D with dot below
            fontLetterMap[0x1e0c] = LettersWithPunctuation(fontLetterMap.at(0x44), dotBelow, 0x1e0c);
            fontLetterMap[0x1e0d] = LettersWithPunctuation(fontLetterMap.at(0x64), dotBelow, 0x1e0d);

            //D with line below
            fontLetterMap[0x1e0e] = LettersWithPunctuation(fontLetterMap.at(0x44), macronBelow, 0x1e0e);
            fontLetterMap[0x1e0f] = LettersWithPunctuation(fontLetterMap.at(0x64), macronBelow, 0x1e0f);

            //H with dot below
            fontLetterMap[0x1e24] = LettersWithPunctuation(fontLetterMap.at(0x48), dotBelow, 0x1e24);
            fontLetterMap[0x1e25] = LettersWithPunctuation(fontLetterMap.at(0x68), dotBelow, 0x1e25);

            //Z with dot below
            fontLetterMap[0x1e92] = LettersWithPunctuation(fontLetterMap.at(0x5a), dotBelow, 0x1e92);
            fontLetterMap[0x1e93] = LettersWithPunctuation(fontLetterMap.at(0x7a), dotBelow, 0x1e93);

            //H with dot above
            fontLetterMap[0x1e22] = LettersWithPunctuation(fontLetterMap.at(0x48), capitalDotAbove, 0x1e22);
            fontLetterMap[0x1e23] = LettersWithPunctuation(fontLetterMap.at(0x68), lowerDotAbove, 0x1e23);

            //B with dot above
            fontLetterMap[0x1e02] = LettersWithPunctuation(fontLetterMap.at(0x42), capitalDotAbove, 0x1e02);
            fontLetterMap[0x1e03] = LettersWithPunctuation(fontLetterMap.at(0x62), lowerDotAbove, 0x1e03);

            //B with line below
            fontLetterMap[0x1e06] = LettersWithPunctuation(fontLetterMap.at(0x42), macronBelow, 0x1e06);
            fontLetterMap[0x1e07] = LettersWithPunctuation(fontLetterMap.at(0x62), macronBelow, 0x1e07);

            //P with dot above
            fontLetterMap[0x1e56] = LettersWithPunctuation(fontLetterMap.at(0x50), capitalDotAbove, 0x1e56);
            fontLetterMap[0x1e57] = LettersWithPunctuation(fontLetterMap.at(0x70), lowerDotAbove, 0x1e57);

            //h with line below
            fontLetterMap[0x1e96] = LettersWithPunctuation(fontLetterMap.at(0x68), macronBelow, 0x1e96);

            //y with diaeresis
            fontLetterMap[0x1e97] = LettersWithPunctuation(fontLetterMap.at(0x74), capitalDiaeresis, 0x1e97);

            //S with dot below
            fontLetterMap[0x1e62] = LettersWithPunctuation(fontLetterMap.at(0x53), dotBelow, 0x1e62);
            fontLetterMap[0x1e63] = LettersWithPunctuation(fontLetterMap.at(0x73), dotBelow, 0x1e63);

            //W with dot above
            fontLetterMap[0x1e86] = LettersWithPunctuation(fontLetterMap.at(0x57), capitalDotAbove, 0x1e86);
            fontLetterMap[0x1e87] = LettersWithPunctuation(fontLetterMap.at(0x77), lowerDotAbove, 0x1e87);

            //G with down arrow
            fontLetterMap[0x0186] = LettersWithPunctuation(fontLetterMap.at(0x47), capitalCharon, 0x0186);
            fontLetterMap[0x0187] = LettersWithPunctuation(fontLetterMap.at(0x67), lowerCharon, 0x0187);

            //Y with left accent
            fontLetterMap[0x1ef2] = LettersWithPunctuation(fontLetterMap.at(0x59), capitalGrave, 0x1ef2);
            fontLetterMap[0x1ef3] = LettersWithPunctuation(fontLetterMap.at(0x79), lowerGrave, 0x1ef3);

			Vector<stbrp_rect> rects(fontLetterMap.size());
            SizeT i = 0, area = 0;
            const UnsignedInteger16 outline2 = outline * 2;

            for (const auto& [key, val] : fontLetterMap) {
                area += static_cast<SizeT>(val.imgWidth + outline2) * static_cast<SizeT>(val.imgHeight + outline2);
                rects[i] = {
                    .id = static_cast<int>(key),
                    .w = static_cast<stbrp_coord>(val.imgWidth + outline2),
                    .h = static_cast<stbrp_coord>(val.imgHeight + outline2),
                    .x = 0,
                    .y = 0,
                    .was_packed = 0
                };

                i++;
            }


            const SignedInteger32 dimension = static_cast<SignedInteger32>(std::sqrt(area) * 1.25f);
            Vector<stbrp_node> nodes(dimension);

			stbrp_context context;
            stbrp_init_target(&context, dimension, dimension, nodes.data(), dimension);
            stbrp_pack_rects(&context, rects.data(), rects.size());

            UnsignedInteger8* imgDataPtr = new UnsignedInteger8[static_cast<SizeT>(dimension) * static_cast<SizeT>(dimension) * 2]();
            SizeT imgDataPtrIndex = 0, letterDataIndex = 0;
            for (const auto& letterData : rects) {
                const Letter& letter = fontLetterMap.at(letterData.id);
                imgDataPtrIndex = (static_cast<SizeT>(letterData.y + outline) * static_cast<SizeT>(dimension) + letterData.x + outline) * 2;
                letterDataIndex = 0;

                for (SizeT row = 0; row < letter.imgHeight; row++) {
                    std::memcpy(&imgDataPtr[imgDataPtrIndex], &letter.imgDataGreyAlpha[letterDataIndex], static_cast<SizeT>(letter.imgWidth) * 2);

                    letterDataIndex += static_cast<SizeT>(letter.imgWidth) * 2;
                    imgDataPtrIndex += static_cast<SizeT>(dimension) * 2;
                }
            }

            Image outImg{
                .data = imgDataPtr,
                .width = dimension,
                .height = dimension,
                .mipmaps = 1,
                .format = PIXELFORMAT_UNCOMPRESSED_GRAY_ALPHA
            };

            ImageAlphaCrop(&outImg, 0.0f);
            ImageFormat(&outImg, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
            ImageColorTint(&outImg, colour);

            if (outline != 0) {
                ImageResizeCanvas(
                    &outImg,
                    outImg.width + outline2,
                    outImg.height + outline2,
                    outline, outline,
                    Color(0, 0, 0, 0) 
                );

                for (SizeT i = 0; i < outline; i++) {
                    ImageDropShadow(outImg, 1, Color(0, 0, 0, 255));
                }
            }

            if (opacity != 255) {
                ImageColorTint(&outImg, Color ( 255, 255, 255, opacity));
            }

            const String outName = name + ".dds";

            SaveDDS_RGBA(
                outName,
                static_cast<const UnsignedInteger8*>(outImg.data),
                outImg.width,
                outImg.height
            );

            UnloadImage(outImg);

            
            std::ofstream fntFile(name + ".fnt", std::ios::binary);
            String fntFileString = "info face=\"" + fontName + "\" size=-" + std::to_string(size) + " bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=0,0 outline="
                + std::to_string(outline) + "\ncommon lineheight=" + std::to_string(size) + " base=" + std::to_string(std::round(0.79f)) + " scaleW=" +
                std::to_string(outImg.width) + " scaleH=" + std::to_string(outImg.height) + " pages=1 packed=0 alphaChnl=0 redChnl=0 greenChnl=0 blueChnl=0\npage id=0 file=\"" +
                outName + "\"\nchars count=" + std::to_string(rects.size());

            for (const auto& letterData : rects) {
                const Letter& letter = fontLetterMap.at(letterData.id);

                fntFileString += "\nchar id=" + std::to_string(letterData.id) + " x=" + std::to_string(letterData.x) + " y=" + std::to_string(letterData.y) + 
                    " width=" + std::to_string(letterData.w) + " height=" + std::to_string(letterData.h) + " xoffset=" + std::to_string(int(letter.offsetX + outline)) + 
                    " yoffset=" + std::to_string(int(letter.offsetY + outline)) + " xadvance=" + std::to_string(int(letter.advanceX + outline * outlineXAdvance)) + " page=0  chnl=15";
            }

            fntFile << fntFileString;
            
        }
    }

    CloseWindow();
    

    return 0;
}