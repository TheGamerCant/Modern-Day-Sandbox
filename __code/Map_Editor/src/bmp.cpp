#include <fstream>

#include "data_types.hpp"
#include "bmp.hpp"
#include "functions.hpp"

BitmapImage::BitmapImage() : sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0),
numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0), pixelsPerMeterHeight(0),
colourTableEntries(0), noOfImportantColours(0), rawDataSize(0), colourTable(), colourToIndex(), rawData(), rgbData() {}

BitmapImage::BitmapImage(const String& fileIn) :
    sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0),
    numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0), pixelsPerMeterHeight(0),
    colourTableEntries(0), noOfImportantColours(0), rawDataSize(0), colourTable(), colourToIndex(), rawData(), rgbData()
{
    LoadFile(fileIn);
}

BitmapImage::BitmapImage(Vector<UnsignedInteger8>& data, const UnsignedInteger32 width, const UnsignedInteger32 height) :
    sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(54), headerSize(40), imgWidth(width), imgHeight(height),
    numberOfColourPlanes(1), colourBitDepth(24), compression(0), imgSize(0), pixelsPerMeterWidth(2834), pixelsPerMeterHeight(2834),
    colourTableEntries(0), noOfImportantColours(0), rawDataSize(width * height * 3), colourTable(), colourToIndex(), rawData(), rgbData() {
    if (width * height * 3 != data.size()) {
        FatalError("Data size does not match width and height for BitmapImage constructor.");
    }

    sizeOfBitmapFile = (width * height * 3) + pixelDataOffset;
	//Round up to nearest multiple of 4
    sizeOfBitmapFile = ((sizeOfBitmapFile + 3) / 4) * 4;
    imgSize = sizeOfBitmapFile - pixelDataOffset;

    std::swap(data, rgbData);
}

void BitmapImage::LoadFile(const String& fileIn) {
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
    else {
        rawDataSize *= 3;
        rgbData.resize(rawDataSize);
        file.seekg(pixelDataOffset, std::ios::beg);
        file.read((Char*)rgbData.data(), rgbData.size());
    }

    delete[] data;

    SwapRBData();
    FlipImage();
}

void BitmapImage::RawToRGB() {
    rgbData.clear();
    rgbData.reserve(rawDataSize * 3);

    for (const auto& pixel : rawData) {
        ColourRGB colour = colourTable[pixel];
        rgbData.push_back(colour.r);
        rgbData.push_back(colour.g);
        rgbData.push_back(colour.b);
    }
}

void BitmapImage::UpdateColourHashmap() {
    colourToIndex.clear();
    for (SizeT i = 0; i < colourTable.size(); ++i) {
        colourToIndex[colourTable[i].ToInteger()] = i;
    }
}

void BitmapImage::SwapRBData() {
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

void BitmapImage::FlipImage() {
    if (rawData.size() != 0) {
        const SizeT rowSize = imgWidth;
        const SizeT iterations = imgHeight / 2;

        for (int y = 0; y < iterations; ++y) {
            UnsignedInteger8* row1 = &rawData[y * rowSize];
            UnsignedInteger8* row2 = &rawData[(imgHeight - 1 - y) * rowSize];
            std::swap_ranges(row1, row1 + rowSize, row2);
        }
    }

    const SizeT rowSize = imgWidth * (colourBitDepth / 8);
    const SizeT iterations = imgHeight / 2;

    for (int y = 0; y < iterations; ++y) {
        UnsignedInteger8* row1 = &rgbData[y * rowSize];
        UnsignedInteger8* row2 = &rgbData[(imgHeight - 1 - y) * rowSize];
        std::swap_ranges(row1, row1 + rowSize, row2);
    }
}

UnsignedInteger16 BitmapImage::GetWidth() { return imgWidth; }
const UnsignedInteger16 BitmapImage::GetWidth() const { return imgWidth; }
UnsignedInteger16 BitmapImage::GetHeight() { return imgHeight; }
const UnsignedInteger16 BitmapImage::GetHeight() const { return imgHeight; }

ColourRGB BitmapImage::GetColourFromIndex(const SizeT index) {
    SizeT index3 = index * 3;
    return ColourRGB(rgbData[index3 + 0], rgbData[index3 + 1], rgbData[index3 + 2]);
}
const ColourRGB BitmapImage::GetColourFromIndex(const SizeT index) const {
    SizeT index3 = index * 3;
    return ColourRGB(rgbData[index3 + 0], rgbData[index3 + 1], rgbData[index3 + 2]);
}

ColourRGB BitmapImage::GetColourFromIndexPremultiplied(const SizeT index) {
    return ColourRGB(rgbData[index + 0], rgbData[index + 1], rgbData[index + 2]);
}
const ColourRGB BitmapImage::GetColourFromIndexPremultiplied(const SizeT index) const {
    return ColourRGB(rgbData[index + 0], rgbData[index + 1], rgbData[index + 2]);
}

UnsignedInteger8 BitmapImage::GetRawDataFromIndex(const SizeT index) { return rawData[index]; }
const UnsignedInteger8 BitmapImage::GetRawDataFromIndex(const SizeT index) const { return rawData[index]; }
UnsignedInteger8* BitmapImage::GetRgbDataPointer() { return rgbData.data(); }
const UnsignedInteger8* BitmapImage::GetRgbDataPointer() const { return rgbData.data(); }

void BitmapImage::PrintBitmapFile(const String& fileOut) {
    FlipImage();
    SwapRBData();

    std::ofstream file(fileOut, std::ios::binary);

    file.write("BM", 2);

    file.write((Char*)&sizeOfBitmapFile, sizeof(UnsignedInteger32));
    file.write((Char*)&reservedBytes, sizeof(UnsignedInteger32));
    file.write((Char*)&pixelDataOffset, sizeof(UnsignedInteger32));
    file.write((Char*)&headerSize, sizeof(UnsignedInteger32));
    file.write((Char*)&imgWidth, sizeof(UnsignedInteger32));
    file.write((Char*)&imgHeight, sizeof(UnsignedInteger32));

    file.write((Char*)&numberOfColourPlanes, sizeof(UnsignedInteger16));
    file.write((Char*)&colourBitDepth, sizeof(UnsignedInteger16));
    file.write((Char*)&compression, sizeof(UnsignedInteger32));
    file.write((Char*)&imgSize, sizeof(UnsignedInteger32));
    file.write((Char*)&pixelsPerMeterWidth, sizeof(UnsignedInteger32));
    file.write((Char*)&pixelsPerMeterHeight, sizeof(UnsignedInteger32));
    file.write((Char*)&colourTableEntries, sizeof(UnsignedInteger32));
    file.write((Char*)&noOfImportantColours, sizeof(UnsignedInteger32));

    if (colourBitDepth == 8) {
        Vector<UnsignedInteger8> rgbaColours;
        rgbaColours.reserve(colourTable.size() * 4);

        for (const auto& colour : colourTable) {
            rgbaColours.push_back(colour.r);
            rgbaColours.push_back(colour.g);
            rgbaColours.push_back(colour.b);
			rgbaColours.push_back(0);
        }
    
        file.write(reinterpret_cast<const Char*>(rgbaColours.data()), rgbaColours.size());
        file.write(reinterpret_cast<const Char*>(rawData.data()), rawData.size());
    }
    else {
        file.write(reinterpret_cast<const Char*>(rgbData.data()), rgbData.size());
    }
}