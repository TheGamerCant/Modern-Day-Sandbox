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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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

std::vector<std::string> returnFiles(){
	std::vector<std::string> tArray;
	
	for (const auto& file : std::filesystem::directory_iterator("in")) {
		if (file.is_regular_file() && file.path().extension() == ".png") {
			tArray.emplace_back(file.path().string());
		}
	}
	
	return tArray;
}

void getVideoData(int& width, int& height, int& frameCount, int& channels, int& noOfOutImages, int& xPos, int& yPos, std::string& fps, std::string& outFilePrefix, std::string& outFileDirectory, bool& looping, bool& transparencyCheck, std::vector<std::string>& inputFilesArray) {
	//Just get the width and height of one frame to get the width and height for the video
	unsigned char* img = stbi_load(inputFilesArray[0].c_str(), &width, &height, &channels, 0);
	stbi_image_free(img);
	frameCount = inputFilesArray.size();
	
	
	//Just have channels = 4,
	channels = 4;
	
	//How many pixels are there in total?
	uint64_t u64NoOfOutImages = width * height * frameCount;
	
	//Larger number = larger file size and less files overall
	noOfOutImages = ceil((float)u64NoOfOutImages / 8000000.0f);
	
	std::cout << "Please enter the Frames Per Second: ";
	std::string userInput;
	std::getline(std::cin, userInput);
	fps = userInput;
	
	std::cout << "Please enter the name of your output file (e.g my_anim_): ";
	std::getline(std::cin, userInput);
	outFilePrefix = userInput;
	
	std::cout << "Please enter the name of your output directory (e.g 'gfx/interface/animated'): ";
	std::getline(std::cin, userInput);
	outFileDirectory = userInput;
	
	std::cout << "Please enter whether the animation is looping (0 or 1): ";
	std::getline(std::cin, userInput);
	if (is_numeric(userInput)) looping = std::stoi(userInput);
	
	std::cout << "Please enter whether the animation should check for transparency (0 or 1): ";
	std::getline(std::cin, userInput);
	if (is_numeric(userInput)) transparencyCheck = std::stoi(userInput);
	
	std::cout << "Please enter the starting X position for the animation: ";
	std::getline(std::cin, userInput);
	if (is_numeric(userInput)) xPos = std::stoi(userInput);
	
	std::cout << "Please enter the starting Y position for the animation: ";
	std::getline(std::cin, userInput);
	if (is_numeric(userInput)) yPos = std::stoi(userInput);
}

void getSliceSizeArray(std::vector<int>& sliceSizeArray, const int width, const int noOfOutImages){
	int quotient = floor(width / noOfOutImages);
	int remainder = width % noOfOutImages;
	
	//Initialise a vector with noOfOutImages entries, all set to quotient
	//Then add the remainder to the vector
	for (int i = 0; i < remainder; ++i) {
		++sliceSizeArray[i];
	}
}

void getoutDirectoryNameArray(std::vector<std::string>& outDirectoryNameArray, std::vector<std::string>& outFileNameArray, const int noOfOutImages, const std::string& outFilePrefix){
	int numDigits = std::to_string(noOfOutImages - 1).length();
	
	for (size_t i = 0; i < noOfOutImages; ++i) {
		//Create the file name
        std::ostringstream indexStream;
        indexStream << std::setw(numDigits) << std::setfill('0') << i;
        std::string outName = outFilePrefix + indexStream.str();  
		
		outFileNameArray[i] = outName;
		outDirectoryNameArray[i] = "out/" + outName + ".png";
	}
}

void createLargeImage(const std::vector<int>& sliceSizeArray, const int height, const int frameCount){
	int fileWidthLarge = sliceSizeArray[0] * frameCount;
	int imageSizeLarge = fileWidthLarge * height * 4;
	unsigned char* imageLarge = new unsigned char[imageSizeLarge];
	memset(imageLarge, 255, imageSizeLarge);
	stbi_write_png("out/large.png", fileWidthLarge, height, 4, imageLarge, fileWidthLarge * 4);
	delete[] imageLarge;
}

void createSmallImage(const std::vector<int>& sliceSizeArray, const int height, const int frameCount){
	int fileWidthSmall = sliceSizeArray[sliceSizeArray.size() - 1] * frameCount;
	int imageSizeSmall = fileWidthSmall * height * 4;
	unsigned char* imageSmall = new unsigned char[imageSizeSmall];
	memset(imageSmall, 255, imageSizeSmall);
	stbi_write_png("out/small.png", fileWidthSmall, height, 4, imageSmall, fileWidthSmall * 4);
	delete[] imageSmall;
}

void copyFile(const std::string& fileName, const int thisSize, const int maxSize) {
	if (thisSize == maxSize) {
		std::filesystem::copy_file("out/large.png", fileName, std::filesystem::copy_options::overwrite_existing);
	}
	else {
		std::filesystem::copy_file("out/small.png", fileName, std::filesystem::copy_options::overwrite_existing);
	}
}

void createBlankImages(const std::vector<int>& sliceSizeArray, const std::vector<std::string>& outDirectoryNameArray, const int height, const int frameCount) {
	std::filesystem::remove_all("out");
	std::filesystem::create_directory("out");
	
	//Create default large and small files
	std::thread threadOne(createLargeImage, std::cref(sliceSizeArray), height, frameCount);
	std::thread threadTwo(createSmallImage, std::cref(sliceSizeArray), height, frameCount);
	
	threadOne.join();
	threadTwo.join();
	
	std::vector<std::future<void>> futures;
	
	for (size_t t = 0; t < sliceSizeArray.size(); ++t) {
		futures.push_back(std::async(std::launch::async, copyFile, std::cref(outDirectoryNameArray[t]), sliceSizeArray[t], sliceSizeArray[0]));
    }
	
	for (auto& fut : futures) { fut.get(); }
	
	std::filesystem::remove("out/large.png");
	std::filesystem::remove("out/small.png");
}

void createGFXFile(const std::vector<std::string>& outFileNameArray, const std::string& outFileDirectory, const int frameCount, const bool looping, const bool transparencyCheck) {
	std::filesystem::path GFXFileName = "out/__GFX.gfx";
    std::ofstream file(GFXFileName, std::ios::out | std::ios::binary);
	
	if (file.is_open()) {
		file << "spriteTypes = {\n";
		for (const auto& fileName : outFileNameArray) {
			file << "\tFrameAnimatedSpriteType = {" <<
				"\n\t\tname = \"GFX_" << fileName << "\"" <<
				"\n\t\ttextureFile = \"" << outFileDirectory << "/" << fileName << ".png\"" <<
				"\n\t\tnoOfFrames = " << frameCount <<
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

void createGUIFile(const std::vector<std::string>& outFileNameArray, const std::vector<int>& sliceSizeArray, const bool transparencyCheck, const int xPos, const int yPos) {
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

unsigned char* RGBtoRGBA(unsigned char* rgbData, const int width, const int height, int& localChannels) {
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

unsigned char* cropImage(unsigned char* src, const int srcWidth, const int srcHeight, const int channels, const int cropX, const int cropY, const int cropWidth, const int cropHeight) {
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

void pasteImage(unsigned char* dest, const int destWidth, const int destHeight, const int channels, unsigned char* cropped, const int cropWidth, const int cropHeight, const int pasteX, const int pasteY) {
	uint32_t rowSize = cropWidth * channels;
	
    for (int y = 0; y < cropHeight; ++y) {
        uint64_t destPos = (((pasteY + y) * destWidth) + pasteX) * channels;
        uint64_t cropPos = y * rowSize;
		
		printf("%d, %d\n", destPos, cropPos);
		
		//Copy onto dest data from cropped of length rowSize
        std::memcpy(&dest[destPos], &cropped[cropPos], rowSize);
    }
}

void fillImages(const std::vector<int>& sliceSizeArray, const std::vector<std::string>& outDirectoryNameArray, const std::vector<std::string>& inputFilesArray, const int width, const int height, const int channels, const int noOfOutImages) {
	
	uint32_t xPosOfInputImages = 0;
	
	for (int i = 0; i < noOfOutImages; ++i) {
		
		//Load an out image
		int localWidth, localHeight, localChannels;
		unsigned char* currentOutImage = stbi_load(outDirectoryNameArray[i].c_str(), &localWidth, &localHeight, &localChannels, 0);
			
		if (localChannels == 3) { currentOutImage = RGBtoRGBA(currentOutImage, localWidth, localHeight, localChannels); }
		
		//Load each input file, crop part of it, paste it onto the output file
		for (int j = 0; j < inputFilesArray.size(); ++j) {
			int localWidth2, localHeight2, localChannels2;
			unsigned char* currentInputImage = stbi_load(inputFilesArray[j].c_str(), &localWidth2, &localHeight2, &localChannels2, 0);
			
			//Get crop of current input image
			unsigned char* croppedImage = cropImage(currentInputImage, localWidth2, localHeight2, localChannels2, xPosOfInputImages, 0, sliceSizeArray[i], height);
			stbi_image_free(currentInputImage);
			
			if (localChannels2 == 3) { croppedImage = RGBtoRGBA(croppedImage, sliceSizeArray[i], height, localChannels2); }
			
			//Paste onto currentOutImage
			pasteImage(currentOutImage, localWidth, localHeight, localChannels, croppedImage, sliceSizeArray[i], height, j * sliceSizeArray[i], 0);
		
			delete[] croppedImage;
		}
		
		//Output the out image
		stbi_write_png(outDirectoryNameArray[i].c_str(), localWidth, localHeight, channels, currentOutImage, width * channels);
		stbi_image_free(currentOutImage);
		
		//Update our starting x pos
		xPosOfInputImages += sliceSizeArray[i];
		std::cout << i << "\n";
	}
}

int main() {	
	std::vector<std::string> inputFilesArray = returnFiles();
	
	//Load video and output data
	int width, height, frameCount, channels, noOfOutImages, xPos, yPos;
	std::string fps, outFilePrefix, outFileDirectory;
	bool looping = false, transparencyCheck = false;
	getVideoData(width, height, frameCount, channels, noOfOutImages, xPos, yPos, fps, outFilePrefix, outFileDirectory, looping, transparencyCheck, inputFilesArray);
	
	//Get execution time
	typedef std::chrono::high_resolution_clock Time;
	typedef std::chrono::milliseconds ms;
	typedef std::chrono::duration<float> fsec;
	auto timeStart = Time::now();
	
	//printf("%d\n%d\n%d\n%d\n%s\n%s\n%s\n%d", width, height, frameCount, channels, fps.c_str(), outFilePrefix.c_str(), outFileDirectory.c_str(), looping);
	
	std::vector<int> sliceSizeArray(noOfOutImages, floor(width / noOfOutImages));
	std::vector<std::string> outDirectoryNameArray(noOfOutImages, "");
	std::vector<std::string> outFileNameArray(noOfOutImages, "");
	
	//Get arrays for size and directories
	std::thread threadOne(getSliceSizeArray, std::ref(sliceSizeArray), width, noOfOutImages);
	std::thread threadTwo(getoutDirectoryNameArray, std::ref(outDirectoryNameArray), std::ref(outFileNameArray), noOfOutImages, std::cref(outFilePrefix));
	
	threadOne.join();
	threadTwo.join();
	
	//Create blank images to write to
	createBlankImages(sliceSizeArray, outDirectoryNameArray, height, frameCount);
	
	
	//Main workhorse is threadThree, but might as well do the .gui and .gfx files too
	std::thread threadThree(fillImages, std::cref(sliceSizeArray), std::cref(outDirectoryNameArray), std::cref(inputFilesArray), width, height, channels, noOfOutImages);
	std::thread threadFour(createGFXFile, std::cref(outFileNameArray), std::cref(outFileDirectory), frameCount, looping, transparencyCheck);
	std::thread threadFive(createGUIFile, std::cref(outFileNameArray), std::cref(sliceSizeArray), transparencyCheck, xPos, yPos);
	
	threadThree.join();
	threadFour.join();
	threadFive.join();


	//Print execution time
	auto timeEnd = Time::now();
	fsec fs = timeEnd - timeStart;
	ms d = std::chrono::duration_cast<ms>(fs);
	std::cout << "\n\nProgram executed in " << d.count() << " ms\n";

	return 0;
}