#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <thread>

std::string escapeBackslashes(std::string& path) {		//Replace all instances of '\\' in a string with '/', necessary for Clausewitz engine to read directory
    std::string str = path;
    size_t pos = 0;
    while ((pos = str.find("\\", pos)) != std::string::npos) {
        str.replace(pos, 1, "/");
        pos += 2;
    }
    return str;
}

void stdSpriteFunction(const std::filesystem::path &modDirectory, const std::string inDir, const std::string outFile){
	std::filesystem::path searchDirectory = modDirectory / inDir;
	std::filesystem::path outDirectory = modDirectory / outFile;
	
	std::vector<std::string> spritesVector;				//File directory
	std::vector<std::string> spritesNamesVector;		//File name
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDirectory)) {		//Go through folders and subfolders
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
			
			if (extension == ".png" || extension == ".dds"){		//.png or .dds only
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), modDirectory);
				spritesVector.push_back(relativePath.string());					//"gfx\\interface\\decision\\foobar.dds"
				spritesNamesVector.push_back(entry.path().stem().string());		//"foobar.dds"
			}
        }
    }
	
	for (auto& directory : spritesVector){		//Clausewitz doesn't allow double backslashes (even tho there are some in the game files???)
		directory = escapeBackslashes(directory);
	}
	
	std::ofstream spriteFile(outDirectory, std::ios::out | std::ios::binary);
	
	int noOfSprites = spritesVector.size();
	if (spriteFile.is_open()) {
		spriteFile << "spriteTypes = {\n";
		
		for (int i = 0; i < noOfSprites; ++i) {				//Standard output
			spriteFile << "\tSpriteType = {\n\t\tname = \"GFX_" << spritesNamesVector[i] << "\"\n\t\ttexturefile = \""
			<< spritesVector[i] << "\"\n\t}\n";
		}
		
		spriteFile << "}";
        spriteFile.close();
    }
}

void ideasFunction(const std::filesystem::path &modDirectory, const std::string inDir, const std::string outFile){		//Allow for gfx to start with 'idea_'
	std::filesystem::path searchDirectory = modDirectory / inDir;
	std::filesystem::path outDirectory = modDirectory / outFile;
	
	std::vector<std::string> spritesVector;
	std::vector<std::string> spritesNamesVector;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDirectory)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
			
			if (extension == ".png" || extension == ".dds"){		//.png or .dds only
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), modDirectory);
				spritesVector.push_back(relativePath.string());	
				
				//Check first five characters
				std::string ideaString = entry.path().stem().string();
				if (ideaString.substr(0,5) != "idea_"){
					ideaString = "idea_" + ideaString;
				}
				
				spritesNamesVector.push_back(ideaString);	
			}
        }
    }
	
	for (auto& directory : spritesVector){	
		directory = escapeBackslashes(directory);
	}
	
	std::ofstream spriteFile(outDirectory, std::ios::out | std::ios::binary);
	
	int noOfSprites = spritesVector.size();
	if (spriteFile.is_open()) {
		spriteFile << "spriteTypes = {\n";
		
		for (int i = 0; i < noOfSprites; ++i) {	
			spriteFile << "\tSpriteType = {\n\t\tname = \"GFX_" << spritesNamesVector[i] << "\"\n\t\ttexturefile = \""
			<< spritesVector[i] << "\"\n\t}\n";
		}
		
		spriteFile << "}";
        spriteFile.close();
    }
}

void goalsFunction(const std::filesystem::path &modDirectory, const std::string inDir, const std::string outFile){						//Same as previous, just different output
	std::filesystem::path searchDirectory = modDirectory / inDir;
	std::filesystem::path outDirectory = modDirectory / outFile;
	
	std::vector<std::string> spritesVector;
	std::vector<std::string> spritesNamesVector;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchDirectory)) {
        if (entry.is_regular_file()) {
            std::string extension = entry.path().extension().string();
			
			if (extension == ".png" || extension == ".dds"){
				std::filesystem::path relativePath = std::filesystem::relative(entry.path(), modDirectory);
				spritesVector.push_back(relativePath.string());
				spritesNamesVector.push_back(entry.path().stem().string());
			}
        }
    }
	
	for (auto& directory : spritesVector){
		directory = escapeBackslashes(directory);
	}
	
	std::ofstream spriteFile(outDirectory, std::ios::out | std::ios::binary);
	
	int noOfSprites = spritesVector.size();
	if (spriteFile.is_open()) {
		spriteFile << "spriteTypes = {\n";
		
		for (int i = 0; i < noOfSprites; ++i) {		//Chunky Monkey
			spriteFile << "\tSpriteType = {\n\t\tname = \"GFX_" << spritesNamesVector[i] << "\"\n\t\ttexturefile = \""
			<< spritesVector[i] << "\"\n\t}\n\n" <<
			"\tSpriteType = {\n\t\tname = \"GFX_" << spritesNamesVector[i] << "_shine\"\n\t\ttexturefile = \""
			<< spritesVector[i] << "\"\n\t\teffectFile = \"gfx/FX/buttonstate.lua\"\n\t\tanimation = {\n\t\t\tanimationmaskfile = \""<<
			spritesVector[i] << "\"\n\t\t\tanimationtexturefile = \"gfx/interface/goals/shine_overlay.dds\"\n\t\t\tanimationrotation = -90.0\n" <<
			"\t\t\tanimationlooping = no\n\t\t\tanimationtime = 0.75\n\t\t\tanimationdelay = 0\n\t\t\tanimationblendmode = \"add\"\n\t\t\tanimationtype = \"scrolling\"\n" <<
			"\t\t\tanimationrotationoffset = { x = 0.0 y = 0.0 }\n\t\t\tanimationtexturescale = { x = 1.0 y = 1.0 }\n\t\t}\n\n\t\tanimation = {\n" <<
			"\t\t\tanimationmaskfile = \"" << spritesVector[i] << "\"\n\t\t\tanimationtexturefile = \"gfx/interface/goals/shine_overlay.dds\"\n\t\t\tanimationrotation = 90.0\n" <<
			"\t\t\tanimationlooping = no\n\t\t\tanimationtime = 0.75\n\t\t\tanimationdelay = 0\n\t\t\tanimationblendmode = \"add\"\n\t\t\tanimationtype = \"scrolling\"\n" <<
			"\t\t\tanimationrotationoffset = { x = 0.0 y = 0.0 }\n\t\t\tanimationtexturescale = { x = 1.0 y = 1.0 }\n\t\t}\n\t\tlegacy_lazy_load = no\n\t}\n";
		}
		
		spriteFile << "}";
        spriteFile.close();
    }
}


int main() {
	std::filesystem::path modDirectory = std::filesystem::current_path();
	modDirectory = modDirectory.parent_path();
	modDirectory = modDirectory.parent_path();
	
	std::thread t1(goalsFunction, modDirectory, "gfx\\interface\\goals", "interface\\MOD_goals.gfx");
	std::thread t2(stdSpriteFunction, modDirectory, "gfx\\interface\\decisions", "interface\\MOD_decisions.gfx");
	std::thread t3(stdSpriteFunction, modDirectory, "gfx\\leaders", "interface\\MOD_leaders.gfx");
	std::thread t4(ideasFunction, modDirectory, "gfx\\interface\\ideas", "interface\\MOD_ideas.gfx");
	std::thread t5(stdSpriteFunction, modDirectory, "gfx\\event_pictures", "interface\\MOD_eventpictures.gfx");
	
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	return 0;
}