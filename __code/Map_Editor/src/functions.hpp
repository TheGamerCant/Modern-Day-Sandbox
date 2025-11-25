#pragma once
#include "data_types.hpp"

//Global throw error function
[[noreturn]] void FATALERROR(const String& msg, const char* file, int line);
#define FatalError(msg) FATALERROR(msg, __FILE__, __LINE__)

//Get time elapsed since beginning of program
String GetTimeElapsedFromStart(const Timestamp& startTime);

//Convert HSV values to RGB
void HSVToRGB(UnsignedInteger8& red, UnsignedInteger8& green, UnsignedInteger8& blue, Float64 H, Float64 S, Float64 V);

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

Boolean ValidDateMonth(const UnsignedInteger8 month, const UnsignedInteger8 date);
Boolean StringCanBecomeDate(const String& str);

//Returns a boolean from a string "yes" or "no"
Boolean GetBoolFromYesNo(String str);

//Return all files of the specified types in a vector
Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String folderPath, const Vector<String>& fileTypes, UnsignedInteger16 reserve = 16);
Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String folderPath, const String& fileType, UnsignedInteger16 reserve = 16);
Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String folderPath, UnsignedInteger16 reserve = 16);

//Get a singluar game file
Path GetGameFile(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String path);

//Loads a file to string, removing all comments and handling @ variables
String LoadFileToString(const String& file);

//Different string parsers for each need
HashMap<String, String> ParseStringForPairsMapUnique(const String& stringIn);
HashMap<String, Vector<String>> ParseStringForPairsMapRepeat(const String& stringIn);
Vector<DoubleString> ParseStringForPairsArray(const String& stringIn, UnsignedInteger32 reserve = 16);

Vector<String> ParseStringAsStringArray(const String& stringIn, Boolean ignoreQuotations = false);
Vector<SignedInteger64> ParseStringAsSignedInteger64Array(const String& stringIn, Boolean ignoreQuotations = false);
Vector<Float64> ParseStringAsFloat64Array(const String& stringIn, Boolean ignoreQuotations = false);
Vector<UnsignedInteger16> ParseStringAsUnsignedInteger16Array(const String& stringIn, Boolean ignoreQuotations = false);
Vector<Decimal> ParseStringAsDecimalArray(const String& stringIn, Boolean ignoreQuotations = false);

//Checks if a country tag is valid
Boolean TagIsValid(const String& tag);

//Generate x amount of random colours
Vector<ColourRGB> GenerateRandomColours(const UnsignedInteger32 colourCount);
Vector<ColourRGB> GenerateRandomColoursInRange(Set<UnsignedInteger32>& usedColours, const UnsignedInteger32 newColourCount,
    const ColourRGB colour, const UnsignedInteger8 range);

//Split a vector into n no. of vectors (original vector becomes empty)
template<typename VectorType>
Vector<Vector<VectorType>> SplitVector(Vector<VectorType>& vec, SizeT chunks) {
    Vector<Vector<VectorType>> result(chunks);

    SizeT base = vec.size() / chunks;
    SizeT extra = vec.size() % chunks;

    auto it = vec.begin();

    for (size_t i = 0; i < chunks; ++i) {
        SizeT count = base + (i < extra ? 1 : 0);

        result[i].reserve(count);

        for (SizeT j = 0; j < count; ++j) {
            result[i].push_back(std::move(*it));
            ++it;
        }
    }

    vec.clear();
    vec.shrink_to_fit();
    return result;
}