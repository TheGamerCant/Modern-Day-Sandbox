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

struct segmentSizeStruct {
	uint32_t x, y, width, height;
};
struct segmentStruct {
	uint32_t x, y, width, height;
	std::string fileName, fileDirectory, PDXDirectory;
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

//
void getSliceSizeArray(std::vector<segmentSizeStruct>& sliceSizeArray, uint16_t videoWidth, uint16_t videoHeight, int32_t segmentWidth, int32_t segmentHeight, uint16_t noOfInImages, uint32_t noOfOutImages){
	//Doubles to make sure division is correct
	double dVideoWidth = videoWidth, dVideoHeight = videoHeight, dSegmentWidth = segmentWidth, dSegmentHeight = segmentHeight;
	
	//Initialise variables
	uint32_t ySlices = ceil(dVideoHeight / dSegmentHeight);
	uint32_t xSlices = 0, yPos = 0, thisWidth = 0, thisHeight = 0;

	int32_t index = 0;
	
	//User set segment width and height
	if (segmentWidth != -1) {
		xSlices = ceil(dVideoWidth / dSegmentWidth);
		
		for (int32_t y = 0; y < ySlices; ++y) {
			uint32_t xPos = 0;
			yPos = y * segmentHeight;
			
			//If we are on the last iteration, do a modulo. Otherwise it's just the regular segment height
			if (y + 1 == ySlices && videoHeight % segmentHeight != 0) {
				thisHeight = videoHeight % segmentHeight;
			}
			else {
				thisHeight = segmentHeight;
			}
			
			for (int32_t x = 0; x < xSlices; ++x) {
				//yPos and xPos is the location of the top left within the image, width and height are the size of the crop
				xPos = x * segmentWidth;
				
				//If we are on the last iteration, do a modulo. Otherwise it's just the regular segment width
				if (x + 1 == xSlices && videoWidth % segmentWidth != 0) {
					thisWidth = videoWidth % segmentWidth;
				}
				else {
					thisWidth = segmentWidth;
				}
				
				sliceSizeArray[index] = { xPos, yPos, thisHeight, thisWidth };
				
				
				//Easier to just keep track of the index by increment than by doing (y * xSlices) + x
				++index;
			}
			
		}
	}
	//No set width, we should optimise it
	else {
		
		// No point doing this calculation more than once
		uint64_t t = ceil((videoWidth * noOfInImages * segmentHeight) / 8000000.0f);
		
		for (int32_t y = 0; y < ySlices; ++y) {
			//How many different segments is this segment going to be divided into?
			uint64_t entriesPerThisSegment = t;
			uint32_t thisHeight = segmentHeight;
			
			uint32_t yPos = y * segmentHeight;
			uint32_t xPos = 0;
			
			
			if (y + 1 == ySlices && videoHeight % segmentHeight != 0) { 
				entriesPerThisSegment = ceil((videoWidth * noOfInImages * (videoHeight % segmentHeight)) / 8000000.0f);
				thisHeight = (videoHeight % segmentHeight);
			}
			
			for (int32_t x = 0; x < entriesPerThisSegment; ++x) {
				uint16_t thisWidth = videoWidth / entriesPerThisSegment; //Rounds down
				
				if (x < videoWidth % entriesPerThisSegment) { thisWidth += 1; }
				
				sliceSizeArray[index] = { xPos, yPos, thisWidth, thisHeight };
				
				xPos += thisWidth;
				++index;
			}
		}
	}
}

//What will the names of our files be
void getOutDirectoryNameArray(std::vector<std::string>& outDirectoryNameArray, std::vector<std::string>& outFileNameArray, const uint32_t noOfOutImages, const std::string& outputFilePrefix){
	int numDigits = std::to_string(noOfOutImages - 1).length();
	
	for (size_t i = 0; i < noOfOutImages; ++i) {
		//Create the file name
        std::ostringstream indexStream;
		
        indexStream << std::setw(numDigits) << std::setfill('0') << i;
        std::string outName;
		outName = outputFilePrefix + (outputFilePrefix.back() == '_' ? "" : "_") + indexStream.str();
		
		outFileNameArray[i] = outName;
		outDirectoryNameArray[i] = "out/" + outName + ".png";
	}
}

//How many images are we going to output
uint32_t getNoOfOutImages(uint16_t videoWidth, uint16_t videoHeight, int32_t segmentWidth, int32_t segmentHeight, uint16_t noOfInImages) {
	//Doubles to make sure division is correct
	double dVideoWidth = videoWidth, dVideoHeight = videoHeight, dSegmentWidth = segmentWidth, dSegmentHeight = segmentHeight;
	
	// User has entered predefined segment slices
	if (segmentWidth != -1) {	
		return ceil(dVideoWidth / dSegmentWidth) * ceil(dVideoHeight / dSegmentHeight);
	}
	
	//Otherwise try and optimise it
	int32_t ySegments = ceil(dVideoHeight / dSegmentHeight);
	uint64_t xSize = 0;
	
	// No point doing this calculation more than once
	uint64_t t = ceil((videoWidth * noOfInImages * segmentHeight) / 8000000.0f);
		
	for (int32_t y = 0; y < ySegments; ++y) {
		
		if (y + 1 == ySegments && videoHeight % segmentHeight != 0) {
			xSize += ceil((videoWidth * noOfInImages * (videoHeight % segmentHeight)) / 8000000.0f);
		}
		else {
			xSize += t;
		}
	}
	return xSize;
}

//Create the .gfx file (spriteTypes)
void createGFXFile(const std::vector<segmentStruct>& segmentsArray, const uint16_t noOfInImages, const bool looping, const bool transparencyCheck) {
	std::filesystem::path GFXFileName = "out/__GFX.gfx";
    std::ofstream file(GFXFileName, std::ios::out | std::ios::binary);
	
	if (file.is_open()) {
		file << "spriteTypes = {\n";
		for (const auto& segment : segmentsArray) {
			file << "\tFrameAnimatedSpriteType = {" <<
				"\n\t\tname = \"GFX_" << segment.fileName << "\"" <<
				"\n\t\ttextureFile = \"" << segment.PDXDirectory << "\"" <<
				"\n\t\tnoOfFrames = " << noOfInImages <<
				"\n\t\talwaysTransparent = " << (transparencyCheck ? "yes" : "no") <<"\"" <<
				"\n\t\tlooping = " << (looping ? "yes" : "no") <<
				"\n\t\tplay_on_show = yes" <<
				"\n\t\tpause_on_loop = 0.0" <<
				"\n\t}\n";
		}
		file << "}";
		file.close();
	}
}

//Create the .gui scripted gui file
void createGUIFile(const std::vector<segmentStruct>& segmentsArray, const bool alwaysTransparent, const int32_t startingGuiXPos, const int32_t startingGuiYPos) {
	std::filesystem::path GUIFileName = "out/__interface.gui";
    std::ofstream file(GUIFileName, std::ios::out | std::ios::binary);
	
	int currentXPos = xPos;
	
	if (file.is_open()) {
		file << "guiTypes = {\n";
		for (int i = 0; i < outFileNameArray.size(); ++i) {
			file << "\ticonType = {" <<
				"\n\t\tname = \"" << outFileNameArray[i] << "\"" <<
				"\n\t\tspriteType = \"GFX_" << outFileNameArray[i] << "\""
				"\n\t\tposition = { x = " << currentXPos << " y = " << yPos << " }" <<
				"\n\t\talwaystransparent = " << (transparencyCheck ? "yes" : "no") <<
				"\n\t}\n";
				
			currentXPos += sliceSizeArray[i];
		}
		file << "}";
		file.close();
	}
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
		uint16_t videoWidth = std::stoi(argv[1]);
		uint16_t videoHeight = std::stoi(argv[2]);
		int32_t segmentWidth = std::stoi(argv[3]);
		int32_t segmentHeight = std::stoi(argv[4]);
		std::string framesPerSecond = argv[5];
		std::string outputFilePrefix = argv[6];
		std::string outputFileDirectory = argv[7];
		int32_t startingGuiXPos = std::stoi(argv[8]);
		int32_t startingGuiYPos = std::stoi(argv[9]);
		bool looping = std::stoi(argv[10]);
		bool alwaysTransparent = std::stoi(argv[11]);
		bool transparencyCheck = std::stoi(argv[12]);
		int32_t coresToUse = std::stoi(argv[13]);
		
		const uint16_t noOfInImages = inputFilesArray.size();
		
		if (videoHeight < 1) {
			std::cout << "Error: Height cannot be less than 1.";
			throw std::runtime_error("");
		}
		
		uint32_t noOfOutImages = getNoOfOutImages(videoWidth, videoHeight, segmentWidth, segmentHeight, noOfInImages);

		std::vector<segmentSizeStruct> sliceSizeArray(noOfOutImages, {0, 0, 0, 0});
		std::vector<std::string> outDirectoryNameArray(noOfOutImages, "");
		std::vector<std::string> outFileNameArray(noOfOutImages, "");

		std::thread threadOne(getSliceSizeArray, std::ref(sliceSizeArray), videoWidth, videoHeight, segmentWidth, segmentHeight, noOfInImages, noOfOutImages);
		std::thread threadTwo(getOutDirectoryNameArray, std::ref(outDirectoryNameArray), std::ref(outFileNameArray), noOfOutImages, std::cref(outputFilePrefix));
		
		threadOne.join();
		threadTwo.join();
		
		//We now have the vector "sliceSizeArray", containing the data of where we have to crop from, and "outDirectoryNameArray" and "outFileNameArray", containing it's file directory
		//Merge these into one vector for easier use
		
		std::vector<segmentStruct> segmentsArray(noOfOutImages, {0, 0, 0, 0, "", ""});
		
		for (int i = 0; i < noOfOutImages; ++i) {
			segmentsArray[i] = {sliceSizeArray[i].x, sliceSizeArray[i].y, sliceSizeArray[i].width, sliceSizeArray[i].height, outFileNameArray[i], outDirectoryNameArray[i], outputFileDirectory + (outputFileDirectory.back() == '/' ? "" : "/") + outFileNameArray[i]  + ".png"};
		}
		
//		for (const auto& segment : segmentsArray) {
//			printf("Image points: %d, %d, %d, %d\nFile name: %s\nFile Directory: %s\nPDX Directory: %s\n\n", segment.x, segment.y, segment.width, segment.height, segment.fileName.c_str(), segment.fileDirectory.c_str(), segment.PDXDirectory.c_str());
//		}
	}
	catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << "\n";
        std::terminate();
    }
	return 0;
}