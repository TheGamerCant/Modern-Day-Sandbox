#include "functions.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <map>
#include "data_types.hpp"
#include <format>


[[noreturn]] void FATALERROR(const String& msg, const char* file, int line) {
    std::cerr << "Fatal error at " << file << ":" << line << ": " << msg << "\n";
    std::exit(EXIT_FAILURE);
}

String GetTimeElapsedFromStart(const Timestamp& startTime) {
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    UnsignedInteger64 totalDuration = duration.count();

    UnsignedInteger64 microseconds = totalDuration;
    UnsignedInteger64 milliseconds = microseconds / 1000;
    UnsignedInteger64 seconds = milliseconds / 1000;
    UnsignedInteger64 minutes = seconds / 60;
    UnsignedInteger64 hours = minutes / 60;

    if (totalDuration < 1000000) {
        return std::to_string(milliseconds) + "ms, " + std::to_string(microseconds % 1000) + "us";
    }
    if (totalDuration < 60000000) {
        return std::to_string(seconds) + "s, " + std::to_string(milliseconds % 1000) + "ms, " + std::to_string(microseconds % 1000) + "us";
    }
    if (totalDuration < 3600000000) {
        return std::to_string(minutes) + "m, " + std::to_string(seconds % 60) + "s, " + std::to_string(milliseconds % 1000) + "ms, " + std::to_string(microseconds % 1000) + "us";
    }

    return std::to_string(hours) + "h, " + std::to_string(minutes % 60) + "m, " + std::to_string(seconds % 60) + "s, " + std::to_string(milliseconds % 1000) + "ms, " + std::to_string(microseconds % 1000) + "us";
}

Boolean CharIsCapitalOrNumber(const Char c) { return (c >= 48 && c <= 57) || (c >= 65 && c <= 90); }
Boolean CharIsCapital(const Char c) { return c >= 65 && c <= 90; }
Boolean CharIsLower(const Char c) { return c >= 97 && c <= 122; }
Boolean CharIsNumber(const Char c) { return c >= 48 && c <= 57; }

String RemoveQuotes(String str) {
    Char first = str.front();
    Char last = str.back();

    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) { str = str.substr(1, str.size() - 2); }
    return str;
}

String BackSlashesToForwardSlashes(const String& input) {
    String result;
    result.reserve(input.size());

    for (SizeT i = 0; i < input.size(); ++i) {
        if (input[i] == '\\') {
            if (i + 1 < input.size() && input[i + 1] == '\\') {
                ++i;
            }
            result += '/';
        } else {
            result += input[i];
        }
    }

    return result;
}

String ToUpper(String str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::toupper(c); });
    return str;
}
String ToLower(String str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return str;
}

Boolean StringCanBecomeInteger(const String& str) {
    SizeT stringLength = str.size();

    if (stringLength == 0) { return false; }

    for (SizeT i = 0; i < stringLength; ++i) { 
        if (i == 0 && !CharIsNumber(str[i]) && str[i] != '+' && str[i] != '-') return false;
        else if (!CharIsNumber(str[i])) return false; 
    }
    return true;
}
Boolean StringCanBecomeFloat(const String& str) {
    if (str.ends_with(".") || str.ends_with("+") || str.ends_with("-")) return false;

    SizeT stringLength = str.size();

    if (stringLength == 0) { return false; }

    UnsignedInteger64 dotCount = 0;
    for (SizeT i = 0; i < stringLength; ++i) { 
        if (i == 0 && !CharIsNumber(str[i]) && str[i] != '+' && str[i] != '-') return false;
        else if (i == 0 && (str[i] == '+' || str[i] == '-') && str.length() > 1) {
            if (str[1] == '.') return false;
        }
        else if (i > 0 && str[i] == '.') { if (dotCount > 0) { return false; } dotCount++; }
        else if (i > 0 && !CharIsNumber(str[i])) return false;
    }
    return true;
}

Boolean GetBoolFromYesNo(String str) {
    str = ToLower(str);
    if (str == "yes") return true;
    else if (str != "no") FatalError("String " + str + " must be \"yes\" or \"no\"");

    return false;
}

//Return a vector of all game files, with a set of valid files endings inputted
Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, const String& folderPath, const Set<String>& fileTypes, UnsignedInteger16 reserve) {
    Vector<Path> filesReturnVector;
    filesReturnVector.reserve(reserve);

    const Boolean modReplacesDirectory = modReplaceDirectories.contains(folderPath);
    const Path vanillaFolder = vanillaDirectory / folderPath;
    const Path modFolder = modDirectory / folderPath;
    const Boolean modFolderExists = std::filesystem::exists(modFolder) && std::filesystem::is_directory(modFolder);

    if (modReplacesDirectory && !(modFolderExists)) { FatalError("Mod replaces " + folderPath + " but " + modFolder.string() + " does not exist."); }

    //If the mod replaces the directory load exclusively from there
    if (modReplacesDirectory && modFolderExists) {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(modFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file() && fileTypes.contains(file.path().extension().string())) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //If the mod does not replace the directory and there is no equivalent directory in our mod or if it does exist but is empty OR if the vanilla and mod directories are the same, load solely from vanilla
    else if ((!modReplacesDirectory && (!modFolderExists || (modFolderExists && std::filesystem::is_empty(modFolder)))) || modDirectory == vanillaDirectory) {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file() && fileTypes.contains(file.path().extension().string())) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //Otherwise load from vanilla unless the same file exists in our mod, then load all mod files
    else {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file() && fileTypes.contains(file.path().extension().string())) {
                //Create a hypothetical file to check for in our mod folder - if it exists, don't add the current vanilla file
                Path fileToCheckFor = modDirectory / folderPath / file.path().filename();
                if (!std::filesystem::exists(fileToCheckFor)) {
                    filesReturnVector.emplace_back(file.path());
                }
            }
        }

        //Now load all mod files
        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file() && fileTypes.contains(file.path().extension().string())) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }

    return filesReturnVector;
}

Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, const String& folderPath, const String& fileType, UnsignedInteger16 reserve) {
    Vector<Path> filesReturnVector;
    filesReturnVector.reserve(reserve);

    const Boolean modReplacesDirectory = modReplaceDirectories.contains(folderPath);
    const Path vanillaFolder = vanillaDirectory / folderPath;
    const Path modFolder = modDirectory / folderPath;
    const Boolean modFolderExists = std::filesystem::exists(modFolder) && std::filesystem::is_directory(modFolder);

    if (modReplacesDirectory && !(modFolderExists)) { FatalError("Mod replaces " + folderPath + " but " + modFolder.string() + " does not exist."); }

    //If the mod replaces the directory load exclusively from there
    if (modReplacesDirectory && modFolderExists) {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(modFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file() && file.path().extension() == fileType) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //If the mod does not replace the directory and there is no equivalent directory in our mod or if it does exist but is empty OR if the vanilla and mod directories are the same, load solely from vanilla
    else if ((!modReplacesDirectory && (!modFolderExists || (modFolderExists && std::filesystem::is_empty(modFolder)))) || modDirectory == vanillaDirectory) {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file() && file.path().extension() == fileType) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //Otherwise load from vanilla unless the same file exists in our mod, then load all mod files
    else {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file() && file.path().extension() == fileType) {
                //Create a hypothetical file to check for in our mod folder - if it exists, don't add the current vanilla file
                Path fileToCheckFor = modDirectory / folderPath / file.path().filename();
                if (!std::filesystem::exists(fileToCheckFor)) {
                    filesReturnVector.emplace_back(file.path());
                }
            }
        }

        //Now load all mod files
        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file() && file.path().extension() == fileType) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }

    return filesReturnVector;
}

Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, String folderPath, UnsignedInteger16 reserve) {
    Vector<Path> filesReturnVector;
    filesReturnVector.reserve(reserve);

    const Boolean modReplacesDirectory = modReplaceDirectories.contains(folderPath);
    const Path vanillaFolder = vanillaDirectory / folderPath;
    const Path modFolder = modDirectory / folderPath;
    const Boolean modFolderExists = std::filesystem::exists(modFolder) && std::filesystem::is_directory(modFolder);

    if (modReplacesDirectory && !(modFolderExists)) { FatalError("Mod replaces " + folderPath + " but " + modFolder.string() + " does not exist."); }

    //If the mod replaces the directory load exclusively from there
    if (modReplacesDirectory && modFolderExists) {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(modFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //If the mod does not replace the directory and there is no equivalent directory in our mod or if it does exist but is empty OR if the vanilla and mod directories are the same, load solely from vanilla
    else if ((!modReplacesDirectory && (!modFolderExists || (modFolderExists && std::filesystem::is_empty(modFolder)))) || modDirectory == vanillaDirectory) {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //Otherwise load from vanilla unless the same file exists in our mod, then load all mod files
    else {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file()) {
                //Create a hypothetical file to check for in our mod folder - if it exists, don't add the current vanilla file
                Path fileToCheckFor = modDirectory / folderPath / file.path().filename();
                if (!std::filesystem::exists(fileToCheckFor)) {
                    filesReturnVector.emplace_back(file.path());
                }
            }
        }

        //Now load all mod files
        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }

    return filesReturnVector;
}

Path GetGameFile(const Path& vanillaDirectory, const Path& modDirectory, const Set<String>& modReplaceDirectories, const String& path) {

    const Path vanillaPath = vanillaDirectory / path;
    const Path modPath = modDirectory / path;

    Path p(path);
    Path directory = p.parent_path();
    Path fileName = p.filename();

    const Boolean modReplacesDirectory = modReplaceDirectories.contains(directory.string());

    if (std::filesystem::exists(modPath) && std::filesystem::is_regular_file(modPath)) { return modPath; }
    else if (modReplacesDirectory) { FatalError("File " + path + " does not exist in the mod"); }
    else if (!(std::filesystem::exists(vanillaPath) && std::filesystem::is_regular_file(vanillaPath))) { FatalError("File " + path + " does not exist"); }

    return vanillaPath;
}

String LoadFileToString(const String& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) FatalError("Failed to open file: " + filename);

    HashMap<String, String> variables;
    std::ostringstream result;
    std::string line;

    while (std::getline(file, line)) {
        Boolean inSingleQuotes = false;
        Boolean inDoubleQuotes = false;

        String processedLine;
        processedLine.reserve(line.size());

        for (SizeT i = 0; i < line.size(); ++i) {
            Char c = line[i];

            if (c == '\'' && !inDoubleQuotes && (i == 0 || line[i - 1] != '\\'))
                inSingleQuotes = !inSingleQuotes;
            else if (c == '"' && !inSingleQuotes && (i == 0 || line[i - 1] != '\\'))
                inDoubleQuotes = !inDoubleQuotes;

            if (c == '#' && !inSingleQuotes && !inDoubleQuotes)
                break;

            processedLine += c;
        }

        //Trim whitespace (This is for variables won't get added to result)
        auto trim = [](String& s) {
            const auto start = s.find_first_not_of(" \t\r\n");
            const auto end = s.find_last_not_of(" \t\r\n");
            s = (start == String::npos) ? "" : s.substr(start, end - start + 1);
            };
        trim(processedLine);

        //Handle variable definition
        if (!processedLine.empty() && processedLine[0] == '@') {
            SizeT eq = processedLine.find('=');
            if (eq != String::npos) {
                String varName = processedLine.substr(1, eq - 1);
                String varValue = processedLine.substr(eq + 1);
                trim(varName);
                trim(varValue);
                variables[varName] = varValue;
            }
            continue;
        }

        //Replace variable references outside quotes
        for (const auto& [name, value] : variables) {
            String token = "@" + name;
            SizeT pos = 0;
            while ((pos = processedLine.find(token, pos)) != String::npos) {
                processedLine.replace(pos, token.size(), value);
                pos += value.size();
            }
        }

        result << processedLine << '\n';
    }

    return result.str();
}

//                                               10 = '\n', 9 = '\t', 11 = '\v', 12 = '\f', 13 = '\r'
Boolean CharIsWhitespace(Char c) { return c == ' ' || (c >= 9 && c <= 13); }

String RemoveStringWhitespace(const String& stringIn) {
    SizeT stringLength = stringIn.size();

    Char* returnCharArray = new Char[stringLength + 2];
    SizeT returnStringSize = 0;
    Boolean inWhitespace = false;
    Boolean started = false;
    Boolean inQuotation = false;

    for (Char c : stringIn) {
        if (c == 34) inQuotation = !inQuotation;

        if (CharIsWhitespace(c) && !inQuotation) {
            if (started && !inWhitespace) {
                returnCharArray[returnStringSize++] = ' ';
                inWhitespace = true;
            }
        }
        else {
            returnCharArray[returnStringSize++] = c;
            inWhitespace = false;
            started = true;
        }
    }

    if (returnStringSize > 0 && returnCharArray[returnStringSize - 1] == ' ') { --returnStringSize; }

    returnCharArray[returnStringSize] = 0;
    String returnString(returnCharArray);
    delete[] returnCharArray;

    return returnString;
}

//Same as previous code, but also removes all whitespaces that border characters '=', '{', '}'
static String PrepareStringForParse(const String& stringIn) {
    String processedString = RemoveStringWhitespace(stringIn);
    SizeT stringLength = processedString.size();

    Char* returnCharArray = new Char[stringLength + 3];
    SizeT returnStringSize = 0;
    Boolean inQuotation = false;

    auto IgnoreChar = [](Char c) {
        return c == '=' || c == '{' || c == '}';
    };

    for (SizeT i = 0; i < stringLength; ++i) {
        Char c = processedString[i];

        if (c == '"') inQuotation = !inQuotation;

        //If whitespace - if character to our left or right is '={}', don't write the whitespace
        if (c == ' ' && !inQuotation) {
            Char next = 0;
            SizeT j = i + 1;
            while (j < stringLength && CharIsWhitespace(processedString[j]))
                ++j;
            if (j < stringLength)
                next = processedString[j];

            if (IgnoreChar(next))
                continue;

            if (returnStringSize > 0 && IgnoreChar(returnCharArray[returnStringSize - 1]))
                continue;


            returnCharArray[returnStringSize++] = c;
        }
        else {
            returnCharArray[returnStringSize++] = c;
        }
    }

    returnCharArray[returnStringSize] = 0;
    String returnString(returnCharArray);
    delete[] returnCharArray;

    return returnString;
}

static Vector<String> ParseStringToVectorSegments(
    const String& stringIn,
    Boolean& hasEqualsSigns,
    Boolean& hasSquigglyBrackets,
    Boolean& hasEqualSquigglyBracketCount
) {
    String processingString = PrepareStringForParse(stringIn);
    const SizeT stringLength = processingString.size();

    Vector<String> documentSegments;
    documentSegments.reserve(1000);

    Char* holdStringArray = new Char[stringLength + 3];
    SizeT holdStringArraySize = 0;

    Boolean isInQuotations = false;

    SignedInteger64 bracketCount = 0;

    for (Char c : processingString) {
        if (c == '"') {
            isInQuotations = !isInQuotations;
        }

        if (!isInQuotations) {
            if (c == ' ') {
                if (holdStringArraySize > 0){
                    holdStringArray[holdStringArraySize++] = 0;
                    documentSegments.emplace_back(holdStringArray);
                    holdStringArraySize = 0;
                }
                continue;
            }
            else if (c == '=' || c == '{' || c == '}') {
                if (c == '=') { hasEqualsSigns = true; }
                else { hasSquigglyBrackets = true; }

                if (c == '{') { bracketCount++; }
                else if (c == '}') { bracketCount--; }


                if (holdStringArraySize > 0) {
                    holdStringArray[holdStringArraySize++] = 0;
                    documentSegments.emplace_back(holdStringArray);
                    holdStringArraySize = 0;
                }

                holdStringArray[0] = c;
                holdStringArray[1] = 0;
                documentSegments.emplace_back(holdStringArray);
                continue;
            }
        }

        holdStringArray[holdStringArraySize++] = c;
    }

    holdStringArray[holdStringArraySize] = 0;
    documentSegments.emplace_back(holdStringArray);
    delete[] holdStringArray;

    if (bracketCount == 0) { hasEqualSquigglyBracketCount = true; }
    else { hasEqualSquigglyBracketCount = false; }

    return documentSegments;
}

Key StringToCorrectTypeKey(const String& stringIn) {
    if (StringCanBecomeInteger(stringIn)) { return static_cast<SignedInteger64>(std::stoll(stringIn)); }
    else if (StringCanBecomeFloat(stringIn)) { return std::stod(stringIn); }

    if (stringIn.size() > 3 || stringIn.size() < 2) { return stringIn; }

    String stringLower = ToLower(stringIn);

    if (stringLower == "yes") { return true; }
    if (stringLower == "no") { return false; }

    return stringIn;
}

PdxJson StringToCorrectTypePdxJson(const String& stringIn) {
    if (StringCanBecomeInteger(stringIn)) { return static_cast<SignedInteger64>(std::stoll(stringIn)); }
    else if (StringCanBecomeFloat(stringIn)) { return std::stod(stringIn); }

    if (stringIn.size() > 3 || stringIn.size() < 2) { return stringIn; }

    String stringLower = ToLower(stringIn);

    if (stringLower == "yes") { return true; }
    if (stringLower == "no") { return false; }

    return stringIn;
}

// Recursively parses a run of tokens (as produced by the tokenizer, where data
// is separated by spaces and '=' '{' '}' are their own segments) into a PdxJson.
//
//   key = value        -> dict entry, stored as [value]
//   key = { ... }       -> dict entry, the block pushed as one element
//   { a b c }           -> list
//
// Every keyed value is stored as a List by default, so repeated keys need no
// special-casing: each occurrence just appends one element. For example
//   key = { 1 2 3 }  key = { 4 5 6 }   ->   key: [[1, 2, 3], [4, 5, 6]]
//   add_core = A  add_core = B          ->   add_core: [A, B]
//
// A '{ }' block containing '=' becomes a Dict; one containing only scalars
// becomes a List. `pos` indexes the next token to read and is advanced past the
// block; the recursion returns when it hits the matching '}' or the end.
static PdxJson ParseBlockSegments(const Vector<String>& t, SizeT& pos, Boolean expectBrace) {
    Dict dict;
    List list;
    Boolean sawKey = false;

    auto addKeyed = [&](const Key& k, PdxJson val) {
        // dict[k] is a null PdxJson on first access; push_back promotes it to a
        // List, so every key ends up mapping to a list of its assigned values.
        dict[k].push_back(std::move(val));
        sawKey = true;
    };

    while (pos < t.size()) {
        if (t[pos] == "}") {                                // end of this block
            ++pos;
            return sawKey ? PdxJson(std::move(dict)) : PdxJson(std::move(list));
        }

        // key = value ?
        if (pos + 1 < t.size() && t[pos + 1] == "=") {
            Key key = StringToCorrectTypeKey(t[pos]);
            pos += 2;                                       // consume key and '='
            if (pos >= t.size()) break;                     // malformed: '=' with no value
            if (t[pos] == "{") {
                ++pos;                                      // consume '{'
                addKeyed(key, ParseBlockSegments(t, pos, true));
            }
            else {
                addKeyed(key, StringToCorrectTypePdxJson(t[pos]));
                ++pos;
            }
            continue;
        }

        // bare scalar or anonymous block -> list element
        if (t[pos] == "{") {
            ++pos;
            list.push_back(ParseBlockSegments(t, pos, true));
        }
        else {
            list.push_back(StringToCorrectTypePdxJson(t[pos]));
            ++pos;
        }
    }

    (void)expectBrace;   // ran out of tokens (top level, or unbalanced braces)
    return sawKey ? PdxJson(std::move(dict)) : PdxJson(std::move(list));
}

PdxJson ParseSegmentsToPdxJson(const Vector<String>& segments) {
    SizeT pos = 0;
    return ParseBlockSegments(segments, pos, false);
}

PdxJson ParseFileToPdxJson(const String& fileName) {
    PdxJson returnJson;

    Boolean hasEqualsSigns = false;
    Boolean hasSquigglyBrackets = false;
    Boolean hasEqualSquigglyBracketCount = false;

    Vector<String> documentSegments = ParseStringToVectorSegments(LoadFileToString(fileName), hasEqualsSigns, hasSquigglyBrackets, hasEqualSquigglyBracketCount);

    if (!hasEqualSquigglyBracketCount) {
        std::cout << std::format("{0} will be skipped, there is an unequal number of squiggly brackets", fileName);
        return returnJson;
    }

    //Simple list
    if (!hasEqualsSigns && !hasSquigglyBrackets) {
        List segments(documentSegments.begin(), documentSegments.end());
        returnJson = std::move(segments);
        return returnJson;
    }

    const SizeT documentSegmentsSize = documentSegments.size();

    //Only key-value pairs
    if (hasEqualsSigns && !hasSquigglyBrackets) {
        if (documentSegmentsSize % 3 != 0) {
            std::cout << std::format("{0} will be skipped, there is an unequal number of key-value pairs", fileName);
            return returnJson;
        }

        Dict keyValuePairs{};

        for (SizeT i = 0; i < documentSegmentsSize; i += 3) {
            const String& segmentOne = documentSegments[i + 0];
            const String& segmentTwo = documentSegments[i + 1];
            const String& segmentThree = documentSegments[i + 2];

            if (segmentTwo != "=" || segmentOne == "=" || segmentThree == "=") {
                std::cout << std::format("{0}: Key-value pair \"{1} {2} {3}\" is invalid and will be skipped\n", fileName, segmentOne, segmentTwo, segmentThree);
                continue;
            }

            Key key = (StringToCorrectTypeKey(segmentOne));
            PdxJson value = (StringToCorrectTypePdxJson(segmentThree));

            if (keyValuePairs.contains(key)) {
                keyValuePairs[key].push_back(value);
            } else {
                keyValuePairs[key] = List{value};
            }
        }

        returnJson = std::move(keyValuePairs);
    }

    //Else
    if (hasEqualsSigns && hasSquigglyBrackets) {
        returnJson = ParseSegmentsToPdxJson(documentSegments);
    }

    return returnJson;
}

Boolean TagIsValid(const String& tag) {
    if (tag.size() != 3) { return false; }
    if (!CharIsCapital(tag[0]) || !CharIsCapitalOrNumber(tag[1]) || !CharIsCapitalOrNumber(tag[2])) { return false; }

    if (tag == "NOT" || tag == "AND" || tag == "OOB" || tag == "LOG" || tag == "NUM" || tag == "RED") { return false; }

    return true;
}
