#include "data_types.hpp"
#include "functions.hpp"
#include <iostream>
#include <cmath>
#include <fstream>
#include "json.hpp"
using json = nlohmann::json;

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#include "raylib.h"

static void AddCodepointRange(Font* font, const Char* fontPath, const SignedInteger32 fontSize, SignedInteger32 start, SignedInteger32 stop) {
    SignedInteger32 rangeSize = stop - start + 1;
    SignedInteger32 currentRangeSize = font->glyphCount;

    // TODO: Load glyphs from provided vector font (if available),
    // add them to existing font, regenerating font image and texture

    SignedInteger32 updatedCodepointCount = currentRangeSize + rangeSize;
    SignedInteger32* updatedCodepoints = (SignedInteger32*)RL_CALLOC(updatedCodepointCount, sizeof(SignedInteger32));

    // Get current codepoint list
    for (SignedInteger32 i = 0; i < currentRangeSize; i++) updatedCodepoints[i] = font->glyphs[i].value;

    // Add new codepoints to list (provided range)
    for (SignedInteger32 i = currentRangeSize; i < updatedCodepointCount; i++)
        updatedCodepoints[i] = start + (i - currentRangeSize);

    UnloadFont(*font);
    *font = LoadFontEx(fontPath, fontSize, updatedCodepoints, updatedCodepointCount);
    RL_FREE(updatedCodepoints);
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
static void SaveToDDS(const String& filename, const UnsignedInteger8* data, UnsignedInteger32 width, UnsignedInteger32 height) {
    struct DDSHeader {
        UnsignedInteger32 magic = 0x20534444; // "DDS "
        UnsignedInteger32 size = 124;
        UnsignedInteger32 flags = 0x0002100F; // caps | height | width | pixelfmt | linearsize
        UnsignedInteger32 height = 0;
        UnsignedInteger32 width = 0;
        UnsignedInteger32 pitchOrLinearSize = 0;
        UnsignedInteger32 depth = 0;
        UnsignedInteger32 mipMapCount = 0;
        UnsignedInteger32 reserved1[11] = {};

        // Pixel Format
        UnsignedInteger32 pfSize = 32;
        UnsignedInteger32 pfFlags = 0x41;       // uncompressed RGB + alpha
        UnsignedInteger32 pfFourCC = 0;
        UnsignedInteger32 pfRGBBitCount = 32;
        UnsignedInteger32 pfRMask = 0x00FF0000;
        UnsignedInteger32 pfGMask = 0x0000FF00;
        UnsignedInteger32 pfBMask = 0x000000FF;
        UnsignedInteger32 pfAMask = 0xFF000000;

        // Caps
        UnsignedInteger32 caps = 0x1000;
        UnsignedInteger32 caps2 = 0;
        UnsignedInteger32 caps3 = 0;
        UnsignedInteger32 caps4 = 0;
        UnsignedInteger32 reserved2 = 0;
    };

    DDSHeader header;
    header.width = width;
    header.height = height;
    header.pitchOrLinearSize = width * 4;

    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<Char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const Char*>(data), width * height * 4);
}

struct SubFont {
    String fontFileName;
    SignedInteger32 fontSize;
    UnsignedInteger8 r, g, b, a;
    SignedInteger32 outline, additionalAdvanceX;
    Vector<UnsignedInteger32> charsToLoad;

    SubFont() : fontFileName(""), fontSize(0), r(255), g(255), b(255), a(255), outline(0), additionalAdvanceX(0), charsToLoad() {}
    SubFont(const String& fontFileName, const SignedInteger32 fontSize, const UnsignedInteger8 r, const UnsignedInteger8 g, 
        const UnsignedInteger8 b, const UnsignedInteger8 a, const SignedInteger32 outline, const SignedInteger32 additionalAdvanceX,
        const Vector<UnsignedInteger32>& charsToLoad) :
        fontFileName(fontFileName), fontSize(fontSize), r(r), g(g), b(b), a(a), outline(outline), additionalAdvanceX(additionalAdvanceX), charsToLoad(charsToLoad) {}
};

struct FontToLoad {
    String fontPath;
    String fontName;
    HashMap<UnsignedInteger32, UnsignedInteger32> similarGlyphs;
    Vector<SubFont> subFonts;

    FontToLoad() : fontPath(""), fontName(""), similarGlyphs() {}
    FontToLoad(const json& fontsData) : fontPath(fontsData.at("font_path").get<String>()), fontName(fontsData.at("font_name").get<String>()), similarGlyphs(), subFonts(){
        if (fontsData.contains("similar_glyphs")) {
            for (auto it = fontsData["similar_glyphs"].begin(); it != fontsData["similar_glyphs"].end(); ++it) {
                similarGlyphs.emplace(
                    std::stoi(it.key()), 
                    it.value().get<SignedInteger32>()
                );
            }
        }

        if (!fontsData.contains("sub_fonts") || !fontsData["sub_fonts"].is_array()) { return; }

        for (const auto& subFontEntry : fontsData["sub_fonts"]) {
            subFonts.emplace_back(
                subFontEntry.at("bitmap_font_name").get<String>(),
                subFontEntry.at("size").get<SignedInteger32>(),
                subFontEntry.at("red").get<UnsignedInteger8>(),
                subFontEntry.at("green").get<UnsignedInteger8>(),
                subFontEntry.at("blue").get<UnsignedInteger8>(),
                subFontEntry.at("alpha").get<UnsignedInteger8>(),
                subFontEntry.at("outline").get<SignedInteger32>(),
                subFontEntry.at("additional_advance_x").get<SignedInteger32>(),
                subFontEntry.at("chars").get<Vector<UnsignedInteger32>>()
            );
        }
    }
};

int main(void) {
    std::ifstream file("in/fonts.json");
    json fontsData = json::parse(file);

    Vector<FontToLoad> fontsToLoad;
    for (const auto& font : fontsData.at("fonts")) {
        fontsToLoad.emplace_back(font);
    }

    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_HIDDEN);
    InitWindow(1, 1, "Bitmap Font Maker");
    SetTargetFPS(60);

    if (!std::filesystem::exists("out") || !std::filesystem::is_directory("out")) { std::filesystem::create_directory("out"); }

    for (const auto& font : fontsToLoad) {
        for (const auto& subFont : font.subFonts) {
            //Load font into raylib
            Font raylibFont = LoadFontEx(font.fontPath.c_str(), subFont.fontSize, nullptr, 0);
            for (SizeT charToLoadIndex = 0; charToLoadIndex < subFont.charsToLoad.size(); charToLoadIndex += 2) {
                AddCodepointRange(
                    &raylibFont,
                    font.fontPath.c_str(),
                    subFont.fontSize,
                    subFont.charsToLoad[charToLoadIndex + 0],
                    subFont.charsToLoad[charToLoadIndex + 1]
                );
            }

            //Create map of letters
			HashMap<UnsignedInteger32, UnsignedInteger32> letterIndexMap;
            for (SizeT glyphIndex = 0; glyphIndex < raylibFont.glyphCount; glyphIndex++) {
                const GlyphInfo& glyphInfo = raylibFont.glyphs[glyphIndex];
                letterIndexMap.emplace(glyphInfo.value, static_cast<UnsignedInteger32>(glyphIndex));
			}

            //Create rectangles to pack
            Vector<stbrp_rect> rects(raylibFont.glyphCount);
            SizeT totalFontArea = 0;
            const UnsignedInteger32 outline2 = subFont.outline * 2;

            for (SizeT rectangleIndex = 0; rectangleIndex < raylibFont.glyphCount; rectangleIndex++) {
                const GlyphInfo& glyphInfo = raylibFont.glyphs[rectangleIndex];

                totalFontArea += static_cast<SizeT>(glyphInfo.image.width + outline2) * static_cast<SizeT>(glyphInfo.image.width + outline2);
                rects[rectangleIndex] = {
                    .id = static_cast<int>(glyphInfo.value),
                    .w = static_cast<stbrp_coord>(glyphInfo.image.width + outline2),
                    .h = static_cast<stbrp_coord>(glyphInfo.image.height + outline2),
                    .x = 0,
                    .y = 0,
                    .was_packed = 0
                };
            }

            //Get the width and height of the total font image
            const SignedInteger32 dimension = static_cast<SignedInteger32>(std::sqrt(totalFontArea) * 1.25f);
            UnsignedInteger8* imgDataPtr = new UnsignedInteger8[static_cast<SizeT>(dimension) * static_cast<SizeT>(dimension) * 2]();

            //Pack rectanges
            Vector<stbrp_node> nodes(dimension);
            stbrp_context context;
            stbrp_init_target(&context, dimension, dimension, nodes.data(), dimension);
            stbrp_pack_rects(&context, rects.data(), rects.size());


            //Write image information to imgDataPtr
            SizeT imgDataPtrIndex = 0, letterDataIndex = 0, rectIndex = 0;
            for (const auto& rect : rects) {
                const GlyphInfo& currentGlyph = raylibFont.glyphs[rectIndex];
                imgDataPtrIndex = (static_cast<SizeT>(rect.y + subFont.outline) * static_cast<SizeT>(dimension) + rect.x + subFont.outline) * 2;
                letterDataIndex = 0;

                for (SizeT row = 0; row < currentGlyph.image.height; row++) {
                    std::memcpy(&imgDataPtr[imgDataPtrIndex], static_cast<unsigned char*>(currentGlyph.image.data) + letterDataIndex, static_cast<SizeT>(currentGlyph.image.width) * 2);

                    letterDataIndex += static_cast<SizeT>(currentGlyph.image.width) * 2;
                    imgDataPtrIndex += static_cast<SizeT>(dimension) * 2;
                }
                rectIndex++;
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

            if (subFont.outline != 0) {
                ImageResizeCanvas(
                    &outImg,
                    outImg.width + outline2,
                    outImg.height + outline2,
                    subFont.outline, subFont.outline,
                    Color(0, 0, 0, 0)
                );

                for (SizeT i = 0; i < subFont.outline; i++) {
                    ImageDropShadow(outImg, 1, Color(0, 0, 0, 255));
                }
            }

            ImageColorTint(&outImg, Color(subFont.r, subFont.g, subFont.b, subFont.a));

            SaveToDDS(
                "out\\" + subFont.fontFileName + ".dds",
                static_cast<const UnsignedInteger8*>(outImg.data),
                outImg.width,
                outImg.height
            );


            UnloadImage(outImg);

            std::ofstream fntFile("out\\" + subFont.fontFileName + ".fnt", std::ios::binary);
            String fntFileString = "info face=\"" + font.fontName + "\" size=-" + std::to_string(subFont.fontSize) + " bold=0 italic=0 charset=\"\" unicode=1 stretchH=100 smooth=1 aa=1 padding=0,0,0,0 spacing=0,0 outline="
                + std::to_string(subFont.outline) + "\ncommon lineheight=" + std::to_string(subFont.fontSize) + " base=" + std::to_string(static_cast<SignedInteger32>(std::round(0.79f * subFont.fontSize))) + " scaleW=" +
                std::to_string(outImg.width) + " scaleH=" + std::to_string(outImg.height) + " pages=1 packed=0 alphaChnl=0 redChnl=0 greenChnl=0 blueChnl=0\npage id=0 file=\"" +
                subFont.fontFileName + ".dds\"\nchars count=" + std::to_string(rects.size());


            for (SizeT glyphIndex = 0; glyphIndex < raylibFont.glyphCount; glyphIndex++) {
                const GlyphInfo& letter = raylibFont.glyphs[glyphIndex];
                const stbrp_rect& letterData = rects[glyphIndex];

				SignedInteger32 xOffset = letter.offsetX - subFont.outline;
				SignedInteger32 xAdvance = letter.advanceX + subFont.outline + subFont.additionalAdvanceX;

                
                if (font.similarGlyphs.contains(letter.value)) {
                    const UnsignedInteger32 similarGlyphValue = font.similarGlyphs.at(letter.value);
                    const UnsignedInteger32 similarGlyphIndex = letterIndexMap.at(similarGlyphValue);
                    const GlyphInfo& similarGlyph = raylibFont.glyphs[similarGlyphIndex];
                    xOffset = similarGlyph.offsetX - subFont.outline;
                    xAdvance = similarGlyph.advanceX + subFont.outline + subFont.additionalAdvanceX;
				}
                

                fntFileString += "\nchar id=" + std::to_string(letter.value) + " x=" + std::to_string(letterData.x) + " y=" + std::to_string(letterData.y) +
                    " width=" + std::to_string(letterData.w) + " height=" + std::to_string(letterData.h) + " xoffset=" + std::to_string(xOffset) +
                    " yoffset=" + std::to_string(letter.offsetY - subFont.outline) + " xadvance=" + std::to_string(xAdvance) + " page=0 chnl=15";
            }

            fntFile << fntFileString;
            UnloadFont(raylibFont);

            std::cout << "Generated font " << subFont.fontFileName << "\n";
        }
    }

    return 0;
}