#pragma once

#include "data_types.hpp"

//Global throw error function
[[noreturn]] void FATALERROR(const String& msg, const char* file, int line);
#define FatalError(msg) FATALERROR(msg, __FILE__, __LINE__)

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

//Loads a file to string, removing all comments and handling @ variables
String LoadFileToString(const String& file);

//Different string parsers for each need
HashMap<String, String> ParseStringForPairsMapUnique(const String& stringIn);
HashMap<String, Vector<String>> ParseStringForPairsMapRepeat(const String& stringIn);

Vector<String> ParseStringAsStringArray(const String& stringIn, Boolean ignoreQuotations = false);
Vector<SignedInteger64> ParseStringAsSignedInteger64Array(const String& stringIn, Boolean ignoreQuotations = false);
Vector<Float64> ParseStringAsFloat64Array(const String& stringIn, Boolean ignoreQuotations = false);
Vector<UnsignedInteger16> ParseStringAsUnsignedInteger16Array(const String& stringIn, Boolean ignoreQuotations = false);