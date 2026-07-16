#include "data_types.hpp"
#include "functions.hpp"

#include <iostream>

int main() {
    /*
    PdxJson pdxJson;
    pdxJson["Testing"] = 0;
    std::cout << pdxJson["Testing"].as<SignedInteger64>();

    pdxJson["Testing"] = "Hello World";
    std::cout << pdxJson["Testing"].as<String>();

    pdxJson["Test_Dict_1"]["Test_Dict_2"]["Test_Dict_3"] = 1000;

    std::cout << "\n\n" << pdxJson.toString() << "\n";
    */

    PdxJson pdxJson = ParseFileToPdxJson("/Users/charles/Documents/GitHub/Modern-Day-Sandbox/common/graphicalculturetype.txt");
    std::cout << "\n\n" << pdxJson.toString() << "\n";

    return 0;
}