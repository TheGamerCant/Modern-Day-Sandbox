#include "data_types.hpp"
#include <fstream>

#pragma pack(push,1)
struct DDSHeader {
    UnsignedInteger32 magic = 0x20534444; // "DDS "
    UnsignedInteger32 size = 124;
    UnsignedInteger32 flags = 0x0002100F; // caps | height | width | pixelfmt | linearsize
    UnsignedInteger32 height;
    UnsignedInteger32 width;
    UnsignedInteger32 pitchOrLinearSize;
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
#pragma pack(pop)

void SaveDDS_RGBA(const String& filename,
    const UnsignedInteger8* data,
    UnsignedInteger32 width,
    UnsignedInteger32 height)
{
    DDSHeader header;
    header.width = width;
    header.height = height;
    header.pitchOrLinearSize = width * 4;

    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<Char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const Char*>(data), width * height * 4);
}