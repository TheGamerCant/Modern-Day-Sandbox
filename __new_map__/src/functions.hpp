#pragma once
#include "data_types.hpp"

//Global throw error function
[[noreturn]] void FATALERROR(const String& msg, const char* file, int line);
#define FatalError(msg) FATALERROR(msg, __FILE__, __LINE__)

//Get time elapsed since beginning of program
String GetTimeElapsedFromStart(const Timestamp& startTime);

//String manipulations
Boolean CharIsCapitalOrNumber(const Char c);
Boolean CharIsCapital(const Char c);
Boolean CharIsLower(const Char c);
Boolean CharIsNumber(const Char c);
Boolean CharIsWhitespace(Char c);
String RemoveQuotes(String str);
String ForwardToBackslashes(String str);
String ToUpper(String str);
String ToLower(String str);
String RemoveStringWhitespace(const String& stringIn);
Boolean StringCanBecomeInteger(const String& str);
Boolean StringCanBecomeFloat(const String& str);
