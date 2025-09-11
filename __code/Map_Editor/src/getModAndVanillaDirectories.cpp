#include <array>
#include <memory>
#include <sstream>
#include <fstream>

#include "getModAndVanillaDirectories.hpp"
#include "functions.hpp"

//If we're on windows, we need to define 'popen' and 'pclose' as '_popen' and 'p_close' respectively
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

void getModAndVanillaDirectories(std::filesystem::path& vanillaDirectory, std::filesystem::path& modDirectory, int& cores,
    std::vector<std::string>& modReplaceDirectories, std::chrono::steady_clock::time_point& startTime) {
    //Char array to store input from Python
    std::array<char, 512> buffer;

    //Run Python GUI and capture output
    std::shared_ptr<FILE> pipe(popen("python src/getModAndVanillaDirectories.py", "r"), pclose);
    if (!pipe) {
        fatalError("Failed to run \"src/getModAndVanillaDirectories.py\"");
    }

    //Read output and write to return value
    if (fgets(buffer.data(), buffer.size(), pipe.get())) {
        //Start the clock from when the user closes the Python GUI
        startTime = std::chrono::high_resolution_clock::now();

        vanillaDirectory = std::string(buffer.data());
        vanillaDirectory = vanillaDirectory.lexically_normal();
        vanillaDirectory = vanillaDirectory.string().substr(0, vanillaDirectory.string().find_last_not_of("\r\n") + 1);
    }
    else { fatalError("Failed to read output from \"src/getModAndVanillaDirectories.py\""); }

    if (fgets(buffer.data(), buffer.size(), pipe.get())) {
        modDirectory = std::string(buffer.data());
        modDirectory = modDirectory.lexically_normal();
        modDirectory = modDirectory.string().substr(0, modDirectory.string().find_last_not_of("\r\n") + 1);
    }
    else { fatalError("Failed to read output from \"src/getModAndVanillaDirectories.py\""); }

    if (fgets(buffer.data(), buffer.size(), pipe.get())) {
        std::string coreStr(buffer.data());
        coreStr.erase(coreStr.find_last_not_of("\r\n") + 1);
        cores = std::stoi(coreStr);
    }
    else { fatalError("Failed to read output from \"src/getModAndVanillaDirectories.py\""); }


    //Now go to .mod file
    int modFileCount = 0;           //How many .mod files are there in our directory
    std::filesystem::path modFilePath;      //Directory of our .mod file

    //Go through our mod directory
    for (const auto& entry : std::filesystem::directory_iterator(modDirectory)) {
        if (entry.is_regular_file() && entry.path().extension() == ".mod") {
            ++modFileCount;
            modFilePath = entry.path();
        }
    }

    //Throw error if there's not only one .mod file
    if (modFileCount == 0) { fatalError("Could not find any .mod files in the mod directory " + modDirectory.string()); }
    else if (modFileCount == 0) { fatalError("More than one .mod files in the mod directory " + modDirectory.string()); }

    //All good :)
    //Go through .mod file, get all the replace_path entries and put them in vector modReplaceDirectories
    else {
        //Reserve some space
        modReplaceDirectories.reserve(32);

        //Open file
        std::ifstream file(modFilePath);
        if (!file.is_open()) { fatalError("Could not open file " + modFilePath.string()); }

        std::string line;

        //Go through line-by-line
        while (std::getline(file, line)) {
            //Get rid of any whitespace
            size_t start = line.find_first_not_of(" \t\r\n");
            //Skip empty lines
            if (start == std::string::npos) continue;

            line = line.substr(start);

            //Check if line starts with "replace_path="
            if (line.rfind("replace_path=", 0) == 0) {
                //Extract the value inside quotes
                size_t firstQuote = line.find('"');
                size_t lastQuote = line.find_last_of('"');
                if (firstQuote != std::string::npos && lastQuote != std::string::npos && lastQuote > firstQuote) {
                    std::string path = line.substr(firstQuote + 1, lastQuote - firstQuote - 1);
                    //Replace all '/' characters with '\\'
                    for (char& c : path) { if (c == '/') { c = '\\'; } }
                    modReplaceDirectories.push_back(path);
                }
            }
        }
    }
}