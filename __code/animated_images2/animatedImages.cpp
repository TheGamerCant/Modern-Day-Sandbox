#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cmath>
#include <iomanip>
#include <sstream>

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

std::vector<std::filesystem::path> returnFiles(){
	std::vector<std::filesystem::path> tArray;
	
	for (const auto& file : std::filesystem::directory_iterator("in")) {
		if (file.is_regular_file() && file.path().extension() == ".png") {
			tArray.emplace_back(file.path());
		}
	}
	
	return tArray;
}

void getVideoData(int& width, int& height, int& frameCount, int& channels, std::string& fps, std::string& outFilePrefix, std::string& outFileDirectory, bool& looping, std::vector<std::filesystem::path>& filesArray) {
	//Just get the width and height of one frame to get the width and height for the video
	std::string tempStr = filesArray[0].string();
	unsigned char* img = stbi_load(tempStr.c_str(), &width, &height, &channels, 0);
	stbi_image_free(img);
	
	frameCount = filesArray.size();
	
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
	
	std::cout << "Please whether the animation is looping (0 or 1): ";
	std::getline(std::cin, userInput);
	looping = std::stoi(userInput);
}

std::vector<int> getSliceSizeArray(int width, int height, int frameCount, int channels){
	//How many pixels are there in total?
	long long noOfOutImages = width * height * frameCount * channels;
	
	//20 mil is a good number to divide by from testing
	//Larger number = larger file size and less files overall
	noOfOutImages = ceil(noOfOutImages / 20000000);
	
	int quotient = floor(width / noOfOutImages);
	int remainder = width % noOfOutImages;
	
	//Initialise a vector with noOfOutImages entries, all set to quotient
	//Then add the remainder to the vector
	std::vector<int>tArray(noOfOutImages, quotient);
	for (int i = 0; i < remainder; ++i) {
		++tArray[i];
	}
	
	return tArray;
}

void createBlankImages(const std::vector<int>& sliceSizeArray, const std::string& outFilePrefix, const int height, const int frameCount) {
	std::filesystem::remove_all("out");
	std::filesystem::create_directory("out");
	
	int numDigits = std::to_string(sliceSizeArray.size() - 1).length();
	
	//Create default large and small files
	int widthLarge = sliceSizeArray[0];
	int fileWidthLarge = sliceSizeArray[0] * frameCount;
	int imageSizeLarge = fileWidthLarge * height * 4;
	unsigned char* imageLarge = new unsigned char[imageSizeLarge];
	memset(imageLarge, 255, imageSizeLarge);
	stbi_write_png("large.png", fileWidthLarge, height, 4, imageLarge, fileWidthLarge * 4);
	delete[] imageLarge;
	
	int fileWidthSmall = sliceSizeArray[sliceSizeArray.size() - 1] * frameCount;
	int imageSizeSmall = fileWidthSmall * height * 4;
	unsigned char* imageSmall = new unsigned char[imageSizeSmall];
	memset(imageSmall, 255, imageSizeSmall);
	stbi_write_png("small.png", fileWidthSmall, height, 4, imageSmall, fileWidthSmall * 4);
	delete[] imageSmall;
	
	for (size_t i = 0; i < sliceSizeArray.size(); ++i) {
		//Create the file name
        std::ostringstream indexStream;
        indexStream << std::setw(numDigits) << std::setfill('0') << i;
        std::string outName = "out/" + outFilePrefix + indexStream.str() + ".png";       

		//Copy the necessary file
    }
	
	
	
}

int main() {
	std::vector<std::filesystem::path> filesArray = returnFiles();
	
	//Load video and output data
	int width, height, frameCount, channels;
	std::string fps, outFilePrefix, outFileDirectory;
	bool looping = false;
	getVideoData(width, height, frameCount, channels, fps, outFilePrefix, outFileDirectory, looping, filesArray);
	
	//printf("%d\n%d\n%d\n%s\n%s\n%s\n%d", width, height, frameCount, fps.c_str(), outFilePrefix.c_str(), outFileDirectory.c_str(), looping);
	
	std::vector<int> sliceSizeArray = getSliceSizeArray(width, height, frameCount, channels);
	
	createBlankImages(sliceSizeArray, outFilePrefix, height, frameCount);

	return 0;
}