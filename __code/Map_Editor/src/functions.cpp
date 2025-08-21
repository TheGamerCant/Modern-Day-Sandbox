#include "functions.hpp"
#include <iostream>
#include <cstdlib>

//Throw an error - call with "fatalError"
[[noreturn]] void FATALERROR(const std::string& msg, const char* file, int line) {
    std::cerr << "Fatal error at " << file << ":" << line << " → " << msg << std::endl;
    std::exit(EXIT_FAILURE);
}