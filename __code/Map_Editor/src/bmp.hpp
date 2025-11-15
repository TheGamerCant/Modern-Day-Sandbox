#pragma once
#include <fstream>
#include "data_types.hpp"
#include "functions.hpp"

static UnsignedInteger32 ReadFourBytes(Char*& data, SizeT& parse) {
    UnsignedInteger32 t = (UnsignedInteger8)data[parse + 3] << 24 | (UnsignedInteger8)data[parse + 2] << 16 | (UnsignedInteger8)data[parse + 1] << 8 | (UnsignedInteger8)data[parse + 0];
    parse += 4;
    return t;
}

static UnsignedInteger16 ReadTwoBytes(Char*& data, SizeT& parse) {
    UnsignedInteger16 t = (UnsignedInteger8)data[parse + 1] << 8 | (UnsignedInteger8)data[parse + 0];
    parse += 2;
    return t;
}

static UnsignedInteger8 ReadOneByte(Char*& data, SizeT& parse) {
    UnsignedInteger8 t = (UnsignedInteger8)data[parse + 0];
    ++parse;
    return t;
}

static ColourRGBA ReadToRGBA(Char*& data, SizeT& parse) {
    ColourRGBA bgra((UnsignedInteger8)data[parse + 0], (UnsignedInteger8)data[parse + 1], (UnsignedInteger8)data[parse + 2], (UnsignedInteger8)data[parse + 3]);
    parse += 4;
    return bgra;
}

static ColourRGB ReadToRGB(Char*& data, SizeT& parse) {
    ColourRGB bgr((UnsignedInteger8)data[parse + 0], (UnsignedInteger8)data[parse + 1], (UnsignedInteger8)data[parse + 2]);
    parse += 3;
    return bgr;
}

struct BitmapImage {
private:
    UnsignedInteger32 sizeOfBitmapFile;
    UnsignedInteger32 reservedBytes;
    UnsignedInteger32 pixelDataOffset;
    UnsignedInteger32 headerSize;
    UnsignedInteger32 imgWidth;
    UnsignedInteger32 imgHeight;
    UnsignedInteger16 numberOfColourPlanes;
    UnsignedInteger16 colourBitDepth;
    UnsignedInteger32 compression;
    UnsignedInteger32 imgSize;
    UnsignedInteger32 pixelsPerMeterWidth;
    UnsignedInteger32 pixelsPerMeterHeight;
    UnsignedInteger32 colourTableEntries;
    UnsignedInteger32 noOfImportantColours;

    UnsignedInteger32 rawDataSize;

    Vector<ColourRGB> colourTable;
    HashMap<UnsignedInteger32, UnsignedInteger16> colourToIndex;
    Vector<UnsignedInteger8> rawData;
    Vector<UnsignedInteger8> rgbData;

public:
    BitmapImage() : sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0),
        numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0), pixelsPerMeterHeight(0),
        colourTableEntries(0), noOfImportantColours(0), rawDataSize(0), colourTable(), colourToIndex(), rawData(), rgbData() {}

    BitmapImage(const String& fileIn) :
        sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0),
        numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0), pixelsPerMeterHeight(0),
        colourTableEntries(0), noOfImportantColours(0), rawDataSize(0), colourTable(), colourToIndex(), rawData(), rgbData() 
    {
        LoadFile(fileIn);
    }

    void LoadFile(const String & fileIn) {
        std::ifstream file(fileIn, std::ios::binary | std::ios::ate);
        if (!file) {
            FatalError("Cannot open file " + fileIn);
        }
        SizeT fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        Char* data = new Char[fileSize];
        file.read(data, fileSize);
        SizeT parse = 0;
        
        UnsignedInteger16 header = ReadTwoBytes(data, parse);

        if (header != 0x4D42) {
            delete[] data;
            FatalError("Not a valid bitmap file: " + fileIn);
        }

        sizeOfBitmapFile = ReadFourBytes(data, parse);
        reservedBytes = ReadFourBytes(data, parse);
        pixelDataOffset = ReadFourBytes(data, parse);
        headerSize = ReadFourBytes(data, parse);
        imgWidth = ReadFourBytes(data, parse);
        imgHeight = ReadFourBytes(data, parse);
        numberOfColourPlanes = ReadTwoBytes(data, parse);
        colourBitDepth = ReadTwoBytes(data, parse);
        compression = ReadFourBytes(data, parse);
        imgSize = ReadFourBytes(data, parse);
        pixelsPerMeterWidth = ReadFourBytes(data, parse);
        pixelsPerMeterHeight = ReadFourBytes(data, parse);
        colourTableEntries = ReadFourBytes(data, parse);
        noOfImportantColours = ReadFourBytes(data, parse);

        if (!(colourBitDepth == 8 || colourBitDepth == 24)) {
            delete[] data;
            FatalError("BMP files have be either 8-bit or 24-bit depth: " + fileIn);
        }

        //Heightmap.bmp has incorrect formatting
        if (colourBitDepth == 8) { colourTableEntries = 255; noOfImportantColours = 255; }

        if (colourTableEntries > 0) {
            colourTable.reserve(colourTableEntries);

            for (SizeT i = 0; i < colourTableEntries; ++i) {
                colourTable.emplace_back(ReadToRGB(data, parse));
                parse++;
            }
        }
        UpdateColourHashmap();


        if (!(parse > pixelDataOffset)) { parse = pixelDataOffset; }
        else {
            delete[] data;
            FatalError("Header definition for bmp file too long in: " + fileIn);
        }

        rawDataSize = imgWidth * imgHeight;
        if (colourBitDepth == 8) {
            rawData.resize(rawDataSize);
            file.seekg(pixelDataOffset, std::ios::beg);
            file.read((Char*)rawData.data(), rawData.size());

            RawToRGB();
        }
        else{
            rawDataSize *= 3;
            rgbData.resize(rawDataSize);
            file.seekg(pixelDataOffset, std::ios::beg);
            file.read((Char*)rgbData.data(), rgbData.size());
        }

        delete[] data;

        SwapRBData();
    }

    void RawToRGB() {
        rgbData.clear();
        rgbData.reserve(rawDataSize * 3);

        for (const auto& pixel : rawData) {
            ColourRGB colour = colourTable[pixel];
            rgbData.push_back(colour.r);
            rgbData.push_back(colour.g);
            rgbData.push_back(colour.b);
        }
    }

    void UpdateColourHashmap() {
        colourToIndex.clear();
        for (SizeT i = 0; i < colourTable.size(); ++i) {
            colourToIndex[colourTable[i].ToInteger()] = i;
        }
    }

    void FlipImage() {
        const UnsignedInteger16 charsPerRow = imgWidth * (colourBitDepth / 8);
        const UnsignedInteger16 iterations = (imgHeight / 2);

        for (SizeT i = 0; i < iterations; ++i) {
            SizeT start = i * charsPerRow;
            SizeT endStart = (imgHeight - 1 - i) * charsPerRow;

            // Swap the two rows in place
            for (SizeT j = 0; j < charsPerRow; ++j) {
                std::swap(rawData[start + j], rawData[endStart + j]);
            }
        }
    }

    void SwapRBData() {
        UnsignedInteger8 r = 0;
        SizeT rgbImgSize = rawDataSize;

        if (colourBitDepth == 8) {
            rgbImgSize *= 3;

            for (auto& colour : colourTable) {
                r = colour.r;
                colour.r = colour.b;
                colour.b = r;
            }
        }

        for (SizeT i = 0; i < rgbImgSize; i += 3) {
            r = rgbData[i];
            rgbData[i] = rgbData[i + 2];
            rgbData[i + 2] = r;
        }
    }

    UnsignedInteger16 GetWidth() { return imgWidth; }
    const UnsignedInteger16 GetWidth() const { return imgWidth; }
    UnsignedInteger16 GetHeight() { return imgHeight; }
    const UnsignedInteger16 GetHeight() const { return imgHeight; }

    ColourRGB GetColourFromIndex(const SizeT index) {
        SizeT index3 = index * 3;
        return ColourRGB(rgbData[index3 + 0], rgbData[index3 + 1], rgbData[index3 + 2]);
    }
    const ColourRGB GetColourFromIndex(const SizeT index) const {
        SizeT index3 = index * 3;
        return ColourRGB(rgbData[index3 + 0], rgbData[index3 + 1], rgbData[index3 + 2]);
    }

    ColourRGB GetColourFromIndexPremultiplied(const SizeT index) {
        return ColourRGB(rgbData[index + 0], rgbData[index + 1], rgbData[index + 2]);
    }
    const ColourRGB GetColourFromIndexPremultiplied(const SizeT index) const{
        return ColourRGB(rgbData[index + 0], rgbData[index + 1], rgbData[index + 2]);
    }

    UnsignedInteger8 GetRawDataFromIndex(const SizeT index) {
        return rawData[index];
    }
    const UnsignedInteger8 GetRawDataFromIndex(const SizeT index) const {
        return rawData[index];
    }
};