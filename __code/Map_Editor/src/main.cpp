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

    VectorMap<GraphicalCulture> graphicalCulturesArray = LoadGraphicalCultureFiles(vanillaDirectory, modDirectory, modReplaceDirectories);
    VectorMap<Country> countriesArray = LoadCountryFiles(vanillaDirectory, modDirectory, modReplaceDirectories, graphicalCulturesArray);

    std::cout << "Program ran for " << GetTimeElapsedFromStart(startTime) << "ms.";
}
