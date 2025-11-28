#include <fstream>

#include "data_types.hpp"
#include "bmp.hpp"
#include "functions.hpp"

BitmapImage::BitmapImage() : sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0),
numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0), pixelsPerMeterHeight(0),
colourTableEntries(0), noOfImportantColours(0), imageType(RGBA), widthTimesHeight(0), colourTable(), colourToIndex(), rawData(), imgData() {}

BitmapImage::BitmapImage(const BitmapImageType imageType) : sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0),
numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0), pixelsPerMeterHeight(0),
colourTableEntries(0), noOfImportantColours(0), imageType(imageType), widthTimesHeight(0), colourTable(), colourToIndex(), rawData(), imgData() {}

BitmapImage::BitmapImage(const String& fileIn, const BitmapImageType imageType) :
    sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0),
    numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0), pixelsPerMeterHeight(0),
    colourTableEntries(0), noOfImportantColours(0), imageType(imageType), widthTimesHeight(0), colourTable(), colourToIndex(), rawData(), imgData()
{
    LoadFile(fileIn);
}

BitmapImage::BitmapImage(Vector<UnsignedInteger8>& data, const UnsignedInteger32 width, const UnsignedInteger32 height, const BitmapImageType imageType) :
    sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(54), headerSize(40), imgWidth(width), imgHeight(height),
    numberOfColourPlanes(1), colourBitDepth(8), compression(0), imgSize(0), pixelsPerMeterWidth(2834), pixelsPerMeterHeight(2834),
    colourTableEntries(0), noOfImportantColours(0), imageType(imageType), widthTimesHeight(width * height), colourTable(), colourToIndex(), rawData(), imgData() {

    if (width * height * GetBytesPerPixel(imageType) != data.size()) {
        FatalError("Data size does not match width and height for BitmapImage constructor.");
    }
    if (imageType == RGBA) { colourBitDepth = 24; }

    sizeOfBitmapFile = (width * height * (colourBitDepth / 8)) + pixelDataOffset;
	//Round up to nearest multiple of 4
    sizeOfBitmapFile = ((sizeOfBitmapFile + 3) / 4) * 4;
    imgSize = sizeOfBitmapFile - pixelDataOffset;

    std::swap(data, imgData);
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

    if (colourTableEntries > 0) {
        colourTable.reserve(colourTableEntries);

        for (SizeT i = 0; i < colourTableEntries; ++i) {
            colourTable.emplace_back(ReadToRGBA(data, parse));
            if (colourTable.back().a == 0) { colourTable.back().a = 255; }
        }
        UpdateColourHashmap();
    }


    if (!(parse > pixelDataOffset)) { parse = pixelDataOffset; }
    else {
        delete[] data;
        FatalError("Header definition for bmp file too long in: " + fileIn);
    }

    widthTimesHeight = imgWidth * imgHeight;
    if (imageType == COLOURMAP) {
        rawData.resize(widthTimesHeight);
        file.seekg(pixelDataOffset, std::ios::beg);
        file.read((Char*)rawData.data(), rawData.size());

        SwapRBData();
        RawToRgba();
    }
    else if (imageType == GREYSCALE) {
        imgData.resize(widthTimesHeight);
        file.seekg(pixelDataOffset, std::ios::beg);
        file.read((Char*)imgData.data(), imgData.size());
    }
    else if (imageType == RGBA) {
		const UnsignedInteger32 widthTimesHeight4 = widthTimesHeight * 4;

        imgData.assign(widthTimesHeight4, 255);
        file.seekg(pixelDataOffset, std::ios::beg);

        for (SizeT i = 0; i < widthTimesHeight4; i += 4) {
			imgData[i + 0] = (UnsignedInteger8)data[parse + 2];
			imgData[i + 1] = (UnsignedInteger8)data[parse + 1];
			imgData[i + 2] = (UnsignedInteger8)data[parse + 0];

            parse += 3;
        }
        
    }

    delete[] data;

    FlipImage();
}

void BitmapImage::RawToRgba() {
    imgData.clear();
    imgData.reserve(widthTimesHeight * 4);

    for (const auto& pixel : rawData) {
        ColourRGB colour = colourTable[pixel];
        imgData.push_back(colour.r);
        imgData.push_back(colour.g);
        imgData.push_back(colour.b);
        imgData.push_back(255);
    }
}

void BitmapImage::UpdateColourHashmap() {
    colourToIndex.clear();
    for (SizeT i = 0; i < colourTable.size(); ++i) {
        colourToIndex[colourTable[i].ToInteger()] = i;
    }
}

void BitmapImage::SwapRBData() {
    if (imageType == COLOURMAP) {
        UnsignedInteger8 r = 0;

        for (auto& colour : colourTable) {
            r = colour.r;
            colour.r = colour.b;
            colour.b = r;
        }

        if (imgData.size() != 0) {
            for (SizeT i = 0; i < widthTimesHeight * 4; i += 4) {
                r = imgData[i];
                imgData[i] = imgData[i + 2];
                imgData[i + 2] = r;
            }
        }
    }
    else if (imageType == RGBA) {
        UnsignedInteger8 r = 0;

        for (SizeT i = 0; i < imgData.size(); i += 4) {
            r = imgData[i];
            imgData[i] = imgData[i + 2];
            imgData[i + 2] = r;
        }
    }
}

void BitmapImage::FlipImage() {
    UnsignedInteger8* row1{};
    UnsignedInteger8* row2{};

    if (rawData.size() != 0) {
		SizeT rowIndexes[2] = { 0, (imgHeight - 1) * imgWidth };
        const SizeT iterations = imgHeight / 2;

        for (int y = 0; y < iterations; ++y) {
            row1 = &rawData[rowIndexes[0] += imgWidth];
            row2 = &rawData[rowIndexes[1] -= imgWidth];
            std::swap_ranges(row1, row1 + imgWidth, row2);
        }
    }

    const SizeT rowSize = imgWidth * GetBytesPerPixel(imageType);
    SizeT rowIndexes[2] = { 0, (imgHeight - 1) * rowSize };
    const SizeT iterations = imgHeight / 2;

    for (int y = 0; y < iterations; ++y) {
        row1 = &imgData[rowIndexes[0] += rowSize];
        row2 = &imgData[rowIndexes[1] -= rowSize];
        std::swap_ranges(row1, row1 + rowSize, row2);
    }
}

UnsignedInteger16 BitmapImage::GetWidth() { return imgWidth; }
const UnsignedInteger16 BitmapImage::GetWidth() const { return imgWidth; }
UnsignedInteger16 BitmapImage::GetHeight() { return imgHeight; }
const UnsignedInteger16 BitmapImage::GetHeight() const { return imgHeight; }

ColourRGB BitmapImage::GetColourFromIndex(const SizeT index) {
    SizeT index4 = index * 4;
    return ColourRGB(imgData[index4 + 0], imgData[index4 + 1], imgData[index4 + 2]);
}
const ColourRGB BitmapImage::GetColourFromIndex(const SizeT index) const {
    SizeT index4 = index * 4;
    return ColourRGB(imgData[index4 + 0], imgData[index4 + 1], imgData[index4 + 2]);
}

ColourRGB BitmapImage::GetColourFromIndexPremultiplied(const SizeT index) {
    return ColourRGB(imgData[index + 0], imgData[index + 1], imgData[index + 2]);
}
const ColourRGB BitmapImage::GetColourFromIndexPremultiplied(const SizeT index) const {
    return ColourRGB(imgData[index + 0], imgData[index + 1], imgData[index + 2]);
}

UnsignedInteger8 BitmapImage::GetValueFromIndex(const SizeT index) {
    return imgData[index];
}
const UnsignedInteger8 BitmapImage::GetValueFromIndex(const SizeT index) const {
    return imgData[index];
}

UnsignedInteger8 BitmapImage::GetRawDataFromIndex(const SizeT index) { return rawData[index]; }
const UnsignedInteger8 BitmapImage::GetRawDataFromIndex(const SizeT index) const { return rawData[index]; }
UnsignedInteger8* BitmapImage::GetImgDataPointer() { return imgData.data(); }
const UnsignedInteger8* BitmapImage::GetImgDataPointer() const { return imgData.data(); }

void BitmapImage::PrintBitmapFile(const String& fileOut) {
    FlipImage();

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

    if (imageType == COLOURMAP) {
        SwapRBData();

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
    else if (imageType == GREYSCALE){
        file.write(reinterpret_cast<const Char*>(imgData.data()), imgData.size());
    }
    else if (imageType == RGBA){
        Vector<UnsignedInteger8> rawData;
        rawData.reserve(widthTimesHeight * 3);

        for (SizeT i = 0; i < imgData.size(); i += 4) {
			rawData.push_back(imgData[i + 2]);
			rawData.push_back(imgData[i + 1]);
			rawData.push_back(imgData[i + 0]);
        }

        file.write(reinterpret_cast<const Char*>(rawData.data()), rawData.size());
    }
}