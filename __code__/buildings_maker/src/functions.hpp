#pragma once
#include "data_types.hpp"

#include "PDX_json.hpp"

using PDX::PdxJson;
using PDX::Key;
using PDX::Dict;
using PDX::List;

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
String BackSlashesToForwardSlashes(const String& str);
String ToUpper(String str);
String ToLower(String str);
String RemoveStringWhitespace(const String& stringIn);
Boolean StringCanBecomeInteger(const String& str);
Boolean StringCanBecomeFloat(const String& str);

//Returns a boolean from a string "yes" or "no"
Boolean GetBoolFromYesNo(String str);

//Return all files of the specified types in a vector
Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, const String& folderPath, const Set<String>& fileTypes, UnsignedInteger16 reserve = 16);
Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, const String& folderPath, const String& fileType, UnsignedInteger16 reserve = 16);
Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, const String& folderPath, UnsignedInteger16 reserve = 16);

//Get a singular game file
Path GetGameFile(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, const String& path);

//Loads a file to string, removing all comments and handling @ variables
String LoadFileToString(const String& file);

//Take a string input and convert it to the correct type
Key StringToCorrectTypeKey(const String& stringIn);
PdxJson StringToCorrectTypePdxJson(const String& stringIn);

//Parse a tokenised segment vector (data split on spaces / = / { }) into a PdxJson
PdxJson ParseSegmentsToPdxJson(const Vector<String>& segments);

//Parse a string into a PdxJson
PdxJson ParseFileToPdxJson(const String& fileName);

//Checks if a country tag is valid
Boolean TagIsValid(const String& tag);
