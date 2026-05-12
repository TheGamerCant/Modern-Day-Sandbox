#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cmath>

#include "data_types.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static void SwapRedBlueChannels(UnsignedInteger8* data, SignedInteger32 width, SignedInteger32 height, SignedInteger32 channels) {
    SizeT pixelCount = width * height * channels;

	for (SizeT i = 0; i < pixelCount; i += channels) {
		UnsignedInteger8& r = data[i + 0];
		UnsignedInteger8& b = data[i + 2];
		std::swap(r, b);
	}
}

static void ApplyAlpha(UnsignedInteger8* colour_data, UnsignedInteger8* impassable_data, UnsignedInteger8* cityalpha_data, SignedInteger32 width, SignedInteger32 height) {
    SizeT pixelCount = width * height * 4;

	for (SizeT i = 0; i < pixelCount; i += 4) {
		//Anything between 0-235 is regular city alpha
		//240-255 = Impassable alpha
		//255 = Regular terrain
		
		Float64 city_alpha = 0.0;
		if (cityalpha_data[i + 3] > 0) {
			city_alpha = cityalpha_data[i + 3];
			city_alpha *= 235.0 / 255.0;
		}
		
		Float64 impassable_alpha = 0.0;
		if (impassable_data[i + 3] > 0) {
			impassable_alpha = impassable_data[i + 3];
			impassable_alpha *= 15.0/255.0;
		}
		
		Float64 out_alpha = 0.0;
		
		
		if (impassable_alpha != 0.0) { out_alpha = impassable_alpha + 240.0; }
		else if (city_alpha != 0.0) { out_alpha = 236.0 - city_alpha; }
		
		if (out_alpha == 0.0) { out_alpha = 238.0; }
		
		colour_data[i + 3] = round(out_alpha);
	}
}

static void SaveToDDS(const String& filename, const UnsignedInteger8* data, SignedInteger32 width, SignedInteger32 height) {
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


int main() {
	auto program_start_time = std::chrono::high_resolution_clock::now();
	
	//Alphamap used for the final image to show city lights
	SignedInteger32 alphamap_width{}, alphamap_height{}, alphamap_channels{};
	UnsignedInteger8 *alphamap_data = stbi_load("in/alphamap.png", &alphamap_width, &alphamap_height, &alphamap_channels, 4);
	
	//RGB Colourmap, used for terrain colour
	SignedInteger32 colourmap_width{}, colourmap_height{}, colourmap_channels{};
	UnsignedInteger8 *colourmap_data = stbi_load("in/colourmap.png", &colourmap_width, &colourmap_height, &colourmap_channels, 4);
	
	//Impassable terrain
	SignedInteger32 impassablesmap_width{}, impassablesmap_height{}, impassablesmap_channels{};
	UnsignedInteger8 *impassablesmap_data = stbi_load("in/impassablesmap.png", &impassablesmap_width, &impassablesmap_height, &impassablesmap_channels, 4);
	
	if ( alphamap_width != colourmap_width || alphamap_width != impassablesmap_width || alphamap_height != colourmap_height || alphamap_height != impassablesmap_height ) {
        throw std::invalid_argument( "Images must be the same size" );
    }
	
	ApplyAlpha (colourmap_data, impassablesmap_data, alphamap_data, colourmap_width, colourmap_height);
	
	SwapRedBlueChannels(colourmap_data, colourmap_width, colourmap_height, 4);
	SaveToDDS("out/colormap_rgb_cityemissivemask_a.dds", colourmap_data, colourmap_width, colourmap_height);
	
	stbi_image_free(alphamap_data);
	stbi_image_free(colourmap_data);
	stbi_image_free(impassablesmap_data);
	
	auto program_end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = program_end_time - program_start_time;
    std::cout << "\n\nElapsed time: " << elapsed_time.count() << " seconds\n";
	
	return 0;
}