#pragma once

#include <string>

[[noreturn]] void FATALERROR(const std::string& msg, const char* file, int line);
#define fatalError(msg) FATALERROR(msg, __FILE__, __LINE__)