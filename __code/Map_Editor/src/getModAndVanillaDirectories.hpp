#pragma once
#include <string>
#include <iostream>
#include <filesystem>

struct systemDataSingleton { //A singleton storing the vanilla game directory, the mod directory, and the amount of cores we're allowed to use.
    std::filesystem::path vanillaDir;
    std::filesystem::path modDir;
    int cores;

    systemDataSingleton() : vanillaDir(""), modDir(""), cores(0) {}
    systemDataSingleton(std::filesystem::path vanillaDir, std::filesystem::path modDir, int cores) : vanillaDir(std::move(vanillaDir)), modDir(std::move(modDir)), cores(cores){}
};

systemDataSingleton getModAndVanillaDirectories();