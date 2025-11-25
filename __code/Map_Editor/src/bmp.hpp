#pragma once
#include "data_types.hpp"

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

enum BitmapImageType : UnsignedInteger8 {
    RGBA = 0,
    GREYSCALE,
    COLOURMAP
};

static UnsignedInteger8 GetBytesPerPixel(const BitmapImageType imageType) {
    switch (imageType) {
        case RGBA:
            return 4;
        case GREYSCALE:
            return 1;
        case COLOURMAP:
            return 1;
        default:
            return 0;
    }
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

	BitmapImageType imageType;
    UnsignedInteger32 rawDataSize;
    Vector<ColourRGBA> colourTable;
    HashMap<UnsignedInteger32, UnsignedInteger16> colourToIndex;
    Vector<UnsignedInteger8> rawData;
    Vector<UnsignedInteger8> imgData;

public:
    BitmapImage();
    BitmapImage(const BitmapImageType imageType);
    BitmapImage(const String& fileIn, const BitmapImageType imageType);
    BitmapImage(Vector<UnsignedInteger8>& data, const UnsignedInteger32 width, const UnsignedInteger32 height, const BitmapImageType imageType);


    void LoadFile(const String& fileIn);

    void RawToRgba();
    void UpdateColourHashmap();
    void SwapRBData();
    void FlipImage();

    UnsignedInteger16 GetWidth();
    const UnsignedInteger16 GetWidth() const;
    UnsignedInteger16 GetHeight();
    const UnsignedInteger16 GetHeight() const;

    ColourRGB GetColourFromIndex(const SizeT index);
    const ColourRGB GetColourFromIndex(const SizeT index) const;

    ColourRGB GetColourFromIndexPremultiplied(const SizeT index);
    const ColourRGB GetColourFromIndexPremultiplied(const SizeT index) const;

    UnsignedInteger8 GetValueFromIndex(const SizeT index);
    const UnsignedInteger8 GetValueFromIndex(const SizeT index) const;

    UnsignedInteger8 GetRawDataFromIndex(const SizeT index);
    const UnsignedInteger8 GetRawDataFromIndex(const SizeT index) const;
    UnsignedInteger8* GetImgDataPointer();
    const UnsignedInteger8* GetImgDataPointer() const;

    void PrintBitmapFile(const String& fileOut);
};