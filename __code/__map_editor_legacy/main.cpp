#include <iostream>
#include <chrono>

#include "getModAndVanillaDirectories.hpp"
#include "functions.hpp"
#include "loadMap.hpp"
#include "mapTypes.hpp"

int main() {
    std::chrono::steady_clock::time_point startTime;        //Time at the start of the program
    std::filesystem::path vanillaDirectory, modDirectory;       //File directories
    int cores;      //No. of cores we're allowed to use for multiprocessing
    std::vector<std::string> modReplaceDirectories;       //File directories overwritten by our mod
    getModAndVanillaDirectories(vanillaDirectory, modDirectory, cores, modReplaceDirectories, startTime);      //Get our directories and core count

    data_manager<terrain> terrains = loadTerrainTypes(vanillaDirectory, modDirectory, cores, modReplaceDirectories);

    std::cout << "Program ran for " << getTimeElapsedFromStart(startTime) << "ms";
    return 0;
}

