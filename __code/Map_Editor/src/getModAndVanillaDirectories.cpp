#include <array>
#include <memory>
#include <sstream>

#include "getModAndVanillaDirectories.hpp"
#include "functions.hpp"

//If we're on windows, we need to define 'popen' and 'pclose' as '_popen' and 'p_close' respectively
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

systemDataSingleton getModAndVanillaDirectories() {
    std::array<char, 256> buffer;
    std::string result;
    result.reserve(256);

    //Run Python GUI and capture output
    std::shared_ptr<FILE> pipe(popen("python src/getModAndVanillaDirectories.py", "r"), pclose);
    if (!pipe) {
        fatalError("Failed to run \"src/getModAndVanillaDirectories.py\"");
    }

    size_t n;
    while ((n = std::fread(buffer.data(), 1, buffer.size(), pipe.get())) > 0) {
        result.append(buffer.data(), n);
    }

    //Values are seperated by newlines ('\n' character), use stringstream to seperate them
    std::istringstream ss(result);
    std::string line;
    systemDataSingleton returnValue{};

    std::getline(ss, line);
    returnValue.vanillaDir = line;

    std::getline(ss, line);
    returnValue.modDir = line;

    std::getline(ss, line);
    returnValue.cores = std::stoi(line);

    return returnValue;
}