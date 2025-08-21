#include <iostream>
#include "getModAndVanillaDirectories.hpp"

int main() {
    //Get a singleton with two std::filesystem::path variables (vanillaDir & modDir) and an int cores
    systemDataSingleton directoresJSON = getModAndVanillaDirectories();

    //Only here temporarily so the compiler doesn't optimise it away
    std::cout << directoresJSON.vanillaDir;
    return 0;
}

