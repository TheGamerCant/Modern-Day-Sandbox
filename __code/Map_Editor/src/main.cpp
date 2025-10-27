#include <iostream>
#include <thread>

#include "functions.hpp"
#include "data_types.hpp"
#include "load_files.hpp"

int main()
{
    Timestamp startTime = std::chrono::high_resolution_clock::now();

    Path vanillaDirectory, modDirectory; Vector<String> modReplaceDirectories;
    LoadFileDirectories(vanillaDirectory, modDirectory, modReplaceDirectories);

    Decimal damageFactor = "1.0";

    VectorMap<GraphicalCulture> graphicalCulturesArray = LoadGraphicalCultureFiles(vanillaDirectory, modDirectory, modReplaceDirectories);
    VectorMap<Country> countriesArray = LoadCountryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, graphicalCulturesArray);
    VectorMap<Building> provinceBuildingsArray, stateBuildingsArray;
    LoadBuildingFiles(vanillaDirectory, modDirectory, modReplaceDirectories, provinceBuildingsArray, stateBuildingsArray, countriesArray);

    std::cout << "Program ran for " << GetTimeElapsedFromStart(startTime) << "ms.";
}
