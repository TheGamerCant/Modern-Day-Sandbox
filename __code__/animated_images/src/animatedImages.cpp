#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <future>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <stdexcept>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// g++ animatedImages.cpp -O3 -static -o animatedImages.exe

static void SaveToDDS(const std::string& filename, const uint8_t* data, uint32_t width, uint32_t height) {
    struct DDSHeader {
        uint32_t magic = 0x20534444; // "DDS "
        uint32_t size = 124;
        uint32_t flags = 0x0002100F; // caps | height | width | pixelfmt | linearsize
        uint32_t height = 0;
        uint32_t width = 0;
        uint32_t pitchOrLinearSize = 0;
        uint32_t depth = 0;
        uint32_t mipMapCount = 0;
        uint32_t reserved1[11] = {};

        // Pixel Format
        uint32_t pfSize = 32;
        uint32_t pfFlags = 0x41;       // uncompressed RGB + alpha
        uint32_t pfFourCC = 0;
        uint32_t pfRGBBitCount = 32;
        uint32_t pfRMask = 0x00FF0000;
        uint32_t pfGMask = 0x0000FF00;
        uint32_t pfBMask = 0x000000FF;
        uint32_t pfAMask = 0xFF000000;

        // Caps
        uint32_t caps = 0x1000;
        uint32_t caps2 = 0;
        uint32_t caps3 = 0;
        uint32_t caps4 = 0;
        uint32_t reserved2 = 0;
    };

    DDSHeader header;
    header.width = width;
    header.height = height;
    header.pitchOrLinearSize = width * 4;

    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(data), width * height * 4);
}

class segmentStruct {
public:
	int32_t x, y, width, height;
	std::string fileName, fileDirectory, PDXDirectory;
	
	segmentStruct(const int32_t x, const int32_t y, const int32_t width, const int32_t height, const std::string& fileName, const std::string& fileDirectory, const std::string& PDXDirectory)
	: x(x), y(y), width(width), height(height), fileName(fileName), fileDirectory(fileDirectory), PDXDirectory(PDXDirectory) {}
	
	void print() {
		printf("x = %d, y = %d, width = %d, height = %d\nFile name: %s\nFile Directory: %s\nPDX Directory: %s\n\n", x, y, width, height, fileName.c_str(), fileDirectory.c_str(), PDXDirectory.c_str());
	}
};

//Is the number a valid integer or float
bool is_numeric(const std::string& str){
    bool decimalFound = false;
    bool hasDigits = false;
    int start = 0;

    if (str.empty()) return false;
    

    if (str[0] == '+' || str[0] == '-') start = 1;  //Can have + or - at the front
    

    for (int i = start; i < str.size(); ++i) {
        if (std::isdigit(str[i])) hasDigits = true;	
        else if (str[i] == '.' && !decimalFound) decimalFound = true;		//Only one decimal point allowed
        else return false; 
    }

    return hasDigits;
}


//Return a vector of every file in the "in/" folder
std::vector<std::string> returnFiles(){
	std::vector<std::string> tArray;
	tArray.reserve(1500);
	
	for (const auto& file : std::filesystem::directory_iterator("in")) {
		if (file.is_regular_file() && file.path().extension() == ".png") {
			tArray.emplace_back(file.path().string());
		}
	}
	
	return tArray;
}

//Get the width, height and starting x and y positions of our segments
std::vector<segmentStruct> getSliceSizeArray(int32_t videoWidth, int32_t videoHeight, int32_t segmentWidth, int32_t segmentHeight, uint16_t noOfInImages, const bool pngOutType){
	//Create a local vector for return
	std::vector<segmentStruct> localArray;
	
	//Doubles to make sure division is correct
	double dVideoWidth = videoWidth, dVideoHeight = videoHeight, dSegmentWidth = segmentWidth, dSegmentHeight = segmentHeight;
	
	//Both will be integers, just store them as doubles to make life easier for division
	double ySlices = ceil(dVideoHeight / dSegmentHeight);

	//Define a variable to determine how large our sergments are going to be
	//Larger no. = larger and fewer segments, smaller no. = smaller and more segments
	double constant = (pngOutType) ? 12000000.0f : 2400000.0f;

	//User has left their desired width undefined, we should optimise 
	if (segmentWidth < 1) {
		//There will be two types of segments. If the video height is not perfectly divisible by the user defined height, the last y segment will be significantly smaller than the previous ones.
		//The previous ones will all be the same however
		//
		//Thus, we should create two vectors containing the widths of our segments - a regular one and a final, smaller one

		int32_t segmentCount = ceil((dVideoWidth * dSegmentHeight * noOfInImages) / constant);
		std::vector<int32_t>segmentWidths(segmentCount, videoWidth / segmentCount);
		if (videoWidth % segmentCount != 0) {
			for (int32_t i = 0; i < (videoWidth % segmentCount); ++i) { ++segmentWidths[i]; }
		}
		
		//Go through each y slice and add their data to localArray
		for (int32_t y = 0; y < ySlices; ++y) {	
			//We are on the last one and it's going to be smaller than the rest, set it to be smaller than the rest
			if (y + 1 == ySlices && videoHeight % segmentHeight != 0) {
				segmentCount = ceil((dVideoWidth * dSegmentHeight * noOfInImages) / constant);
				segmentWidths.assign(segmentCount, videoWidth / segmentCount);
				if (videoWidth % segmentCount != 0) {
					for (int32_t i = 0; i < (videoWidth % segmentCount); ++i) { ++segmentWidths[i]; }
				}
			}
			
			
			//Add our segments to the vector
			
			int32_t currentX = 0, currentWidth = 0, currentHeight = 0;
			for(int32_t x = 0; x < segmentCount; ++x) {
				//Get the current width and y height and emplace back
				currentWidth = segmentWidths[x];
				currentHeight = segmentHeight;
				if (y + 1 == ySlices && videoHeight % segmentHeight != 0) { currentHeight = videoHeight % segmentHeight; }
				
				localArray.emplace_back(currentX, y * segmentHeight, currentWidth, currentHeight, "", "", "");
				
				currentX += segmentWidths[x];
			}
		}
	}
	
	//User has input a predefined x width, as well as a predefined y height
	else {
		double xSlices = ceil(dVideoWidth / dSegmentWidth);
		int32_t currentX = 0, currentY = 0, currentWidth = 0, currentHeight = 0;
		
		//Pretty simple - go over each part of the grid and add it's x and y positions as well as width and height
		for(int32_t y = 0; y < ySlices; ++y) {
			currentY = y * segmentHeight;
			currentHeight = segmentHeight;
			if (y + 1 == ySlices && videoHeight % segmentHeight != 0) { currentHeight = videoHeight % segmentHeight; }
			
			for(int32_t x = 0; x < xSlices; ++x) {
				currentX = x * segmentWidth;
				currentWidth = segmentWidth;
				if (x + 1 == xSlices && videoWidth % segmentWidth != 0) { currentWidth = videoWidth % segmentWidth; }
				
				localArray.emplace_back(currentX, currentY, currentWidth, currentHeight, "", "", "");
			}
		}
	}
	
	return localArray;
}


//What will the names of our files be
void getOutDirectoryNames(std::vector<segmentStruct>& segmentsArray, const std::string& outputFilePrefix, std::string& outputFileDirectory, const std::string& outFileType){
	uint32_t noOfOutImages = segmentsArray.size();
	int numDigits = std::to_string(noOfOutImages - 1).length();
	
	for (size_t i = 0; i < noOfOutImages; ++i) {
		//Create the file name
        std::ostringstream indexStream;
		
        indexStream << std::setw(numDigits) << std::setfill('0') << i;
        std::string outName;
		outName = outputFilePrefix + (outputFilePrefix.back() == '_' ? "" : "_") + indexStream.str();
	
		segmentsArray[i].fileName = outName;	
		segmentsArray[i].fileDirectory = "out/" + outName + "." + outFileType;
		segmentsArray[i].PDXDirectory = outputFileDirectory + (outputFileDirectory.back() == '/' ? "" : "/") + outName  + "." + outFileType;
	}
}

//Create the .gfx file (spriteTypes)
void createGFXFile(const std::vector<segmentStruct>& segmentsArray, const uint16_t noOfInImages, const std::string& framesPerSecond, const bool looping, const bool transparencyCheck) {
	std::filesystem::path GFXFileName = "out/__GFX.gfx";
    std::ofstream file(GFXFileName, std::ios::out | std::ios::binary);
	
	if (file.is_open()) {
		file << "spriteTypes = {\n";
		for (const auto& segment : segmentsArray) {
			file << "\tFrameAnimatedSpriteType = {" <<
				"\n\t\tname = \"GFX_" << segment.fileName << "\"" <<
				"\n\t\ttextureFile = \"" << segment.PDXDirectory << "\"" <<
				"\n\t\tnoOfFrames = " << noOfInImages <<
				"\n\t\tanimation_rate_fps = " << framesPerSecond <<
				"\n\t\talwaysTransparent = " << (transparencyCheck ? "yes" : "no") <<
				"\n\t\tlooping = " << (looping ? "yes" : "no") <<
				"\n\t\tplay_on_show = yes" <<
				"\n\t\tpause_on_loop = 0.0" <<
				"\n\t}\n";
		}
		file << "}";
		file.close();
	}
	
	//Also - loadType = "INGAME" || loadType = "FRONTEND"
}

//Create the .gui scripted gui file
void createGUIFile(const std::vector<segmentStruct>& segmentsArray, const bool alwaysTransparent, const int32_t startingGuiXPos, const int32_t startingGuiYPos) {
	std::filesystem::path GUIFileName = "out/__interface.gui";
    std::ofstream file(GUIFileName, std::ios::out | std::ios::binary);
	
	if (file.is_open()) {
		file << "guiTypes = {\n";
		for (const auto& segment : segmentsArray) {
			file << "\ticonType = {" <<
				"\n\t\tname = \"" << segment.fileName << "\"" <<
				"\n\t\tspriteType = \"GFX_" << segment.fileName << "\""
				"\n\t\tposition = { x = " << (segment.x + startingGuiXPos) << " y = " << (segment.y + startingGuiYPos) << " }" <<
				"\n\t\talwaystransparent = " << (alwaysTransparent ? "yes" : "no") <<
				"\n\t}\n";
		}
		file << "}";
		file.close();
	}
}

//Operations for image manipulation
unsigned char* RGBtoRGBA(unsigned char* rgbData, const int32_t width, const int32_t height, int& localChannels) {
	unsigned char* rgbaData = new unsigned char[width * height * 4];
	
	for (int i = 0, j = 0; i < width * height * 3; i += 3, j += 4) {
        rgbaData[j] = rgbData[i]; 
        rgbaData[j + 1] = rgbData[i + 1];
        rgbaData[j + 2] = rgbData[i + 2];
        rgbaData[j + 3] = 255;
    }
	
	localChannels = 4;
	
	return rgbaData;
}

unsigned char* cropImage(unsigned char* src, const int32_t srcWidth, const int32_t srcHeight, const int32_t channels, const int32_t cropX, const int32_t cropY, const int32_t cropWidth, const int32_t cropHeight) {
    uint32_t rowSize = cropWidth * channels;
    unsigned char* cropped = new unsigned char[rowSize * cropHeight];

    for (int y = 0; y < cropHeight; ++y) {
        uint64_t srcPos = ((cropY + y) * srcWidth + cropX) * channels;
        uint64_t dstPos = y * rowSize;
		
		//Copy onto cropped data from src of length rowSize
        std::memcpy(&cropped[dstPos], &src[srcPos], rowSize);
    }

    return cropped;
}

void pasteImage(unsigned char* dest, const int32_t destWidth, const int32_t destHeight, const int32_t channels, unsigned char* cropped, const int32_t cropWidth, const int32_t cropHeight, const int32_t pasteX, const int32_t pasteY) {
	uint32_t rowSize = cropWidth * channels;
	
    for (int y = 0; y < destHeight; ++y) {
        uint64_t destPos = (((pasteY + y) * destWidth) + pasteX) * channels;
        uint64_t cropPos = y * rowSize;
		
		//printf("%d, %d, %d\n", destPos, cropPos, rowSize);
		
		//Copy onto dest data from cropped of length rowSize
        std::memcpy(&dest[destPos], &cropped[cropPos], rowSize);
    }
}

//Go over a vector of images
void processImages(std::vector<segmentStruct>& segmentsArray, const std::vector<std::string>& inputFilesArray, const int32_t videoWidth, const int32_t videoHeight, const bool pngOutType) {
	for (auto& segment : segmentsArray) {
		//Get the dimensions of the out image
		int32_t outWidth = segment.width * inputFilesArray.size();
		int32_t outHeight = segment.height;
		int64_t outSize = outWidth * outHeight * 4;
		
		//Set all values equal to 255
		unsigned char* currentOutImage = new unsigned char[outSize];
		memset(currentOutImage, 255, outSize);
		
		for (int32_t i = 0; i < inputFilesArray.size(); ++i) {
			//Load image
			int inWidth, inHeight, inChannels;
			unsigned char* currentInputImage = stbi_load(inputFilesArray[i].c_str(), &inWidth, &inHeight, &inChannels, 0);
			
			//Make sure image is valid
			if (!currentInputImage) {
				throw std::runtime_error("Error: Failed to load " + inputFilesArray[i] + ".");
			}
			else if (inWidth != videoWidth || inHeight != videoHeight) {
				throw std::runtime_error("Error: " + inputFilesArray[i] + " does not have valid dimensions.");
			}
			
			//Get crop of current input image
			unsigned char* croppedImage = cropImage(currentInputImage, inWidth, inHeight, inChannels, segment.x, segment.y, segment.width, segment.height);
			stbi_image_free(currentInputImage);
			
			//More efficient to turn the cropped version to RGBA than the entire image
			if (inChannels == 3) { croppedImage = RGBtoRGBA(croppedImage, segment.width, segment.height, inChannels); }
			
			//Paste onto currentOutImage
			pasteImage(currentOutImage, outWidth, outHeight, 4, croppedImage, segment.width, segment.height, i * segment.width, 0);
		
			delete[] croppedImage;
		}
		if (pngOutType) {
			stbi_write_png(segment.fileDirectory.c_str(), outWidth, outHeight, 4, currentOutImage, outWidth * 4);
		}
		else {
			SaveToDDS(
				segment.fileDirectory, currentOutImage, outWidth, outHeight
			);
		}
		delete[] currentOutImage;
	}
}

void divideVectorAndMultiThread(const std::vector<segmentStruct>& segmentsArray, const std::vector<std::string>& inputFilesArray, const int32_t coresToUse, const int32_t videoWidth, const int32_t videoHeight, bool debug, bool pngOutType) {
    uint32_t baseSize = segmentsArray.size() / coresToUse;
    uint32_t remainder = segmentsArray.size() % coresToUse;

	//Create vector containing the vectors
	std::vector<std::vector<segmentStruct>> segmentsArray2D;
	
	//If there's less segments than cores account for that
	if (baseSize == 0) {
		segmentsArray2D.resize(segmentsArray.size());
	}
	else {
		segmentsArray2D.resize(coresToUse);
	}
	
	
	for (int32_t i = 0; i < segmentsArray2D.size(); ++i) {
		if (i < remainder) {
			segmentsArray2D[i].reserve(baseSize + 1);
		}
		else {
			segmentsArray2D[i].reserve(baseSize);
		}
	}
	
	//Go over and fill the vector
	for (int32_t i = 0; i < segmentsArray.size(); ++i) {
		int32_t firstDimension = i % segmentsArray2D.size();
		
		segmentsArray2D[firstDimension].push_back(segmentsArray[i]);
	}
	
	if (debug == true) {
		int32_t index = 0;
		for (auto& vectors: segmentsArray2D) {
			printf("\n\nVector %d:\n", index);
			
			for (auto& segment : vectors) { segment.print(); }
			
			++index;
		}
	}
	
	//Now do multithreading
	std::vector<std::thread> threads;
	
	for (size_t i = 0; i < segmentsArray2D.size(); ++i) { 
		threads.emplace_back(processImages, std::ref(segmentsArray2D[i]), std::cref(inputFilesArray), videoWidth, videoHeight, pngOutType);
	}
	
	for (auto& thread : threads) { thread.join(); }
}

int main(int argc, char* argv[]) {
	try {	
		//Make sure "in/" exists and has files, get those files
		if (std::filesystem::exists("out") && std::filesystem::is_directory("out")) {
			std::filesystem::permissions("out", std::filesystem::perms::owner_all, std::filesystem::perm_options::add);
			std::filesystem::remove_all("out");
		}
		std::filesystem::create_directory("out");
		std::vector<std::string> inputFilesArray;
		if (std::filesystem::exists("out") && std::filesystem::is_directory("out")) {
			inputFilesArray = returnFiles();
		}
		else {
			std::cout << "Error: 'in' folder does not exist.";
			throw std::runtime_error("");
		}
		if (inputFilesArray.size() < 1) {
			std::cout << "Error: No files in 'in' folder.";
			throw std::runtime_error("");
		}
			
		//Set variables		
		int32_t videoWidth = std::stoi(argv[1]);
		int32_t videoHeight = std::stoi(argv[2]);
		int32_t segmentWidth = std::stoi(argv[3]);
		int32_t segmentHeight = std::stoi(argv[4]);
		std::string framesPerSecond = argv[5];
		std::string outputFilePrefix = argv[6];
		std::string outputFileDirectory = argv[7];
		int32_t startingGuiXPos = std::stoi(argv[8]);
		int32_t startingGuiYPos = std::stoi(argv[9]);
		bool pngOutType = std::stoi(argv[10]);
		bool looping = std::stoi(argv[11]);
		bool alwaysTransparent = std::stoi(argv[12]);
		bool transparencyCheck = std::stoi(argv[13]);
		bool debug = std::stoi(argv[14]);
		int32_t coresToUse = std::stoi(argv[15]);
		
		
		
//		int32_t videoWidth = 1920;
//		int32_t videoHeight = 1440;
//		int32_t segmentWidth = 800;
//		int32_t segmentHeight = 1000;
//		std::string framesPerSecond = "30.0";
//		std::string outputFilePrefix = "my_hoi_animation";
//		std::string outputFileDirectory = "gfx/interface/animated";
//		int32_t startingGuiXPos = 0;
//		int32_t startingGuiYPos = 0;
//		bool pngOutType = true;
//		bool looping = false;
//		bool alwaysTransparent = false;
//		bool transparencyCheck = false;
//		bool debug = true;
//		int32_t coresToUse = 3;
		
		const uint16_t noOfInImages = inputFilesArray.size();
		std::string outFileType = (pngOutType) ? "png" : "dds";
		
		if (videoHeight < 1) {
			std::cout << "Error: Height cannot be less than 1.";
			throw std::runtime_error("");
		}

		std::vector<segmentStruct> segmentsArray = getSliceSizeArray(videoWidth, videoHeight, segmentWidth, segmentHeight, noOfInImages, pngOutType);

		getOutDirectoryNames(segmentsArray, outputFilePrefix, outputFileDirectory, outFileType);

		//Create the .gfx and .gui files
		std::thread threadGFX(createGFXFile, std::ref(segmentsArray), noOfInImages, framesPerSecond, looping, transparencyCheck);
		std::thread threadGUI(createGUIFile, std::ref(segmentsArray), alwaysTransparent, startingGuiXPos, startingGuiYPos);
		
		threadGFX.join();
		threadGUI.join();
	
		if(debug == true) {
			printf("There are a total of %d output files:\n", segmentsArray.size());
		}
	
		divideVectorAndMultiThread(segmentsArray, inputFilesArray, coresToUse, videoWidth, videoHeight, debug, pngOutType);
		
		printf("\nAnimation successful.\n\n");
	}
	catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        std::terminate();
    }
	return 0;
}