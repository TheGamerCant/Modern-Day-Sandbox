#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <fstream>
#include <thread>
#include <sstream>
#include <unordered_map>

struct mapTypeStruct {
  std::string id, data;
  
  mapTypeStruct(const std::string& id_, const std::string& data_)
        : id(id_), data(data_) {}
};

void replaceEntriesFunction(const std::filesystem::path& modDirectory, const std::string& fileName){
	std::filesystem::path baseFilePath = (fileName + ".txt");
	std::filesystem::path modFilePath = modDirectory / "map" / (fileName + ".txt");
	std::filesystem::path tempFilePath = modDirectory / "map" / (fileName + ".tmp");
	
	std::ifstream mapTypesToReplaceFile(baseFilePath);
    if (!mapTypesToReplaceFile) {
        std::cerr << baseFilePath << " does not exist.\n";
        return;
    }

	//Create a vector containing the first two segments and the remaining segments
    std::vector<mapTypeStruct> mapTypesToReplaceVector;
    std::string line;

    while (std::getline(mapTypesToReplaceFile, line)) {
        size_t first_sep = line.find(';');
        if (first_sep == std::string::npos) continue;

        size_t second_sep = line.find(';', first_sep + 1);
        if (second_sep == std::string::npos) continue;

        std::string part1 = line.substr(0, second_sep);
        std::string part2 = line.substr(second_sep + 1);

        mapTypesToReplaceVector.emplace_back(part1, part2);
    }
	
//	for (const auto& row : mapTypesToReplaceVector) {
//		std::cout << "[" << row.id << "] | [" << row.data << "]\n";
//	}

	//Now replace the entries in the mod buildings.txt
	
	//Start by making sure the file exists
	std::ifstream inputFile(modFilePath);
	if (!inputFile) {
		std::cerr << modFilePath << " does not exist.\n";
		return;
	}
	std::ofstream outputFile(tempFilePath);
	
	//Make a map for quicker look up
	std::unordered_map<std::string, std::string> replacementMap;
    for (const auto& entry : mapTypesToReplaceVector) {
        replacementMap[entry.id] = entry.data;
    }

	while (std::getline(inputFile, line)) {
        size_t first_sep = line.find(';');
        if (first_sep == std::string::npos) {
            outputFile << line << '\n';
            continue;
        }

        size_t second_sep = line.find(';', first_sep + 1);
        if (second_sep == std::string::npos) {
            outputFile << line << '\n';
            continue;
        }

        std::string key = line.substr(0, second_sep);
        auto it = replacementMap.find(key);

        if (it != replacementMap.end()) {
            outputFile << key << ";" << it->second << '\n';
        } else {
            outputFile << line << '\n';
        }
    }
	
	inputFile.close();
	outputFile.close();
	
	std::filesystem::rename(tempFilePath, modFilePath);
}

int main() {
	std::filesystem::path modDirectory = std::filesystem::current_path();
	modDirectory = modDirectory.parent_path();
	modDirectory = modDirectory.parent_path();
	
	std::thread t1(replaceEntriesFunction, modDirectory, "buildings");
	std::thread t2(replaceEntriesFunction, modDirectory, "unitstacks");
	
	t1.join();
	t2.join();
	return 0;
}