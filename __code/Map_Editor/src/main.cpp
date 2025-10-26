#include <iostream>
#include <thread>

#include "functions.hpp"
#include "data_types.hpp"
#include "load_files.hpp"

int main()
{
    Timestamp startTime = std::chrono::high_resolution_clock::now();
    Vector<String>errorsLog; errorsLog.reserve(256);

    Path vanillaDirectory, modDirectory; Vector<String> modReplaceDirectories;
    LoadFileDirectories(vanillaDirectory, modDirectory, modReplaceDirectories);

    Vector<Country> countriesArray;
    LoadCountryFiles(countriesArray, vanillaDirectory, modDirectory, modReplaceDirectories, errorsLog);

    std::cout << "Program ran for " << GetTimeElapsedFromStart(startTime) << "ms.";
}
