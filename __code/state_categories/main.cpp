#include <iostream>
#include <string>
#include <filesystem>
#include <vector>
#include <regex>
#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>

std::string returnNumberStringOneToSixtyFour(const int number){
	int tens = number / 10;
	int ones = number % 10;
	
	if (tens > 0 && tens < 2 ){
		switch(number){
			case(10):
				return "ten";
			case(11):
				return "eleven";
			case(12):
				return "twelve";
			case(13):
				return "thirteen";
			case(14):
				return "fourteen";
			case(15):
				return "fifteen";
			case(16):
				return "sixteen";
			case(17):
				return "seventeen";
			case(18):
				return "eighteen";
			case(19):
				return "nineteen";
			default:
				break;
		}
	}
	
	std::string tensStr = "";
	if (tens > 1){
		switch(tens){
			case(2):
				tensStr = "twenty";
				break;
			case(3):
				tensStr = "thirty";
				break;
			case(4):
				tensStr = "forty";
				break;
			case(5):
				tensStr = "fifty";
				break;
			case(6):
				tensStr = "sixty";
				break;
			default:
				break;
		}
	}
	
	std::string onesStr = "";
	switch(ones){
		case(1):
			onesStr = "one";
			break;
		case(2):
			onesStr = "two";
			break;
		case(3):
			onesStr = "three";
			break;
		case(4):
			onesStr = "four";
			break;
		case(5):
			onesStr = "five";
			break;
		case(6):
			onesStr = "six";
			break;
		case(7):
			onesStr = "seven";
			break;
		case(8):
			onesStr = "eight";
			break;
			break;
		case(9):
			onesStr = "nine";
			break;
		default:
			break;
	}
	
	if (ones != 0 && tens > 1)  onesStr[0] = std::toupper(static_cast<unsigned char>(onesStr[0]));
	
	return (tensStr + onesStr);
}

std::vector<std::filesystem::path> findFilesToLoad(std::string folder, std::filesystem::path modPath){
	std::vector<std::filesystem::path> tArray;
	
	modPath = modPath / folder;
	
	int fileCount{};
	for (const auto& file : std::filesystem::directory_iterator(modPath)) ++fileCount;
	tArray.resize(fileCount);
	
	fileCount = 0;
	for (const auto& file : std::filesystem::directory_iterator(modPath)){
		tArray[fileCount] = file.path();
		++fileCount;
	}
	return tArray;
}

void updateStateCategory(const std::filesystem::path& file){
	std::string filePath = file.string();
	
	std::ifstream stateFile(filePath);
	std::stringstream buffer;
    buffer << stateFile.rdbuf();
    std::string fileContent = buffer.str();
	stateFile.close();
	
	double population{};
	std::regex manpowerRegex(R"(manpower\s*=\s*(\d+))");
    std::smatch manpowerMatch;
    if (regex_search(fileContent, manpowerMatch, manpowerRegex)) population = stoi(manpowerMatch[1]);

	
	double fBuildingSlotCount = std::round(cbrt(population) / 4.60f)  - 2;
	const int iBuildingSlotCount = std::clamp(fBuildingSlotCount, 1.0, 64.0);
	
	
//	std::string outString = filePath + ", " + std::to_string(population) + ", " + std::to_string(iBuildingSlotCount) + "\n";
//	std::cout << outString;
	
	std::string stateCatString = "state_category = STATE_CATEGEGORY_" + returnNumberStringOneToSixtyFour(iBuildingSlotCount);
	std::regex stateCategoryRegex(R"(state_category\s*=\s*\w+)");
	fileContent = regex_replace(fileContent, stateCategoryRegex, stateCatString);
	
	std::ofstream stateFileOut(filePath);
	stateFileOut << fileContent;
	stateFileOut.close();
}

int main(){
	const std::filesystem::path modPath = std::filesystem::current_path().parent_path().parent_path();
	std::vector<std::filesystem::path> stateFiles = findFilesToLoad ("history\\states", modPath);
	const int vecSize = stateFiles.size();

	
	for (int i = 0; i < vecSize; ++i) {
		updateStateCategory(stateFiles[i]);
	}
	return 0;
}