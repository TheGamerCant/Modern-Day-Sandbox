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

void HSVToRGB(UnsignedInteger8& red, UnsignedInteger8& green, UnsignedInteger8& blue, Float64 H, Float64 S, Float64 V) {
    Float64 C = V * S;
    Float64 X = C * (1 - fabs(fmod(H * 6, 2) - 1));
    Float64 m = V - C;

    Float64 rPrime, gPrime, bPrime;

    if (0 <= H && H < 1.0f / 6) { rPrime = C; gPrime = X; bPrime = 0; }
    else if (1.0f / 6 <= H && H < 2.0f / 6) { rPrime = X; gPrime = C; bPrime = 0; }
    else if (2.0f / 6 <= H && H < 3.0f / 6) { rPrime = 0; gPrime = C; bPrime = X; }
    else if (3.0f / 6 <= H && H < 4.0f / 6) { rPrime = 0; gPrime = X; bPrime = C; }
    else if (4.0f / 6 <= H && H < 5.0f / 6) { rPrime = X; gPrime = 0; bPrime = C; }
    else { rPrime = C; gPrime = 0; bPrime = X; }

    //Convert the normalized [0,1] RGB to [0,255] integer RGB values
    red = static_cast<UnsignedInteger8>((rPrime + m) * 255);
    green = static_cast<UnsignedInteger8>((gPrime + m) * 255);
    blue = static_cast<UnsignedInteger8>((bPrime + m) * 255);
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

Boolean ValidDateMonth(const UnsignedInteger8 month, const UnsignedInteger8 date) {
    switch (month) {
    case 1:
        if (date < 1 || date > 31) return false;
        break;

    case 2:
        if (date < 1 || date > 28) return false;        //Paradox don't do leap years
        break;

    case 3:
        if (date < 1 || date > 31) return false;
        break;

    case 4:
        if (date < 1 || date > 30) return false;
        break;

    case 5:
        if (date < 1 || date > 31) return false;
        break;

    case 6:
        if (date < 1 || date > 30) return false;
        break;

    case 7:
        if (date < 1 || date > 31) return false;
        break;

    case 8:
        if (date < 1 || date > 31) return false;
        break;

    case 9:
        if (date < 1 || date > 30) return false;
        break;

    case 10:
        if (date < 1 || date > 31) return false;
        break;

    case 11:
        if (date < 1 || date > 30) return false;
        break;

    case 12:
        if (date < 1 || date > 31) return false;
        break;


    default:
        return false;
        break;
    }

    return true;
}

Boolean StringCanBecomeDate(const String& str) {
    Char* charArray = new Char[str.size() + 2];
    SizeT arraySize = 0;
    UnsignedInteger64 stringColumn = 0;
    UnsignedInteger8 dateColumn = 0;

    SignedInteger32 year, month, date, hour;

    for (const auto& c : str) {
        if (c == '.') {
            charArray[arraySize++] = 0;
            String currentStr = String(charArray);

            if (!StringCanBecomeInteger(currentStr)) { delete[] charArray; return false; }
            SignedInteger32 entry = std::stoi(currentStr);

            switch (dateColumn) {
            case 0:
                if (entry < -5000) { delete[] charArray; return false; }
                year = entry;
                break;
            case 1:
                if (entry < 1 || entry > 12) { delete[] charArray; return false; }
                month = entry;
                break;
            case 2:
                if (!ValidDateMonth(month, entry)) { delete[] charArray; return false; }
                date = entry;
                break;
            case 3:
                if (entry < 0 || entry > 23) { delete[] charArray; return false; }
                hour = entry;
                break;
            default:
                break;
            }

            arraySize = 0; dateColumn++;
        }
        else { charArray[arraySize++] = c; }
    }

    if (dateColumn == 2) {
        charArray[arraySize++] = 0;
        String currentStr = String(charArray);

        if (!StringCanBecomeInteger(currentStr)) { delete[] charArray; return false; }
        SignedInteger32 entry = std::stoi(currentStr);

        if (!ValidDateMonth(month, entry)) { delete[] charArray; return false; }
        date = entry;
        hour = 1;
        arraySize = 0;
    }
    else if (dateColumn == 3) {
        charArray[arraySize++] = 0;
        String currentStr = String(charArray);

        if (!StringCanBecomeInteger(currentStr)) { delete[] charArray; return false; }
        SignedInteger32 entry = std::stoi(currentStr);

        if (entry < 1 || entry > 24) { delete[] charArray; return false; }
        hour = entry;
        arraySize = 0;
    }

    if (arraySize != 0) { delete[] charArray; return false; }

    delete[] charArray;
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
            if (file.is_regular_file() && fileTypes.contains(file.path().extension())) {
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
            if (file.is_regular_file() && fileTypes.contains(file.path().extension())) {
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
            if (file.is_regular_file() && fileTypes.contains(file.path().extension())) {
                //Create a hypothetical file to check for in our mod folder - if it exists, don't add the current vanilla file
                Path fileToCheckFor = modDirectory / folderPath / file.path().filename();
                if (!std::filesystem::exists(fileToCheckFor)) {
                    filesReturnVector.emplace_back(file.path());
                }
            }
        }

        //Now load all mod files
        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file() && fileTypes.contains(file.path().extension())) {
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

    const Boolean modReplacesDirectory = modReplaceDirectories.contains(directory);

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

Vector<ColourRGB> GenerateRandomColours(const UnsignedInteger32 newColourCount) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<SignedInteger32> dist(0, 255);

    Set<UnsignedInteger32> usedColours;
    Vector<ColourRGB> newColours; newColours.reserve(newColourCount);

    while (newColours.size() < newColourCount) {
        ColourRGB colour{ static_cast<UnsignedInteger8>(dist(gen)),
              static_cast<UnsignedInteger8>(dist(gen)),
              static_cast<UnsignedInteger8>(dist(gen))};

        if (usedColours.insert(colour.ToInteger()).second)
            newColours.push_back(colour);
    }

    return newColours;
}

Vector<ColourRGB> GenerateRandomColours(Set<UnsignedInteger32>& usedColours, const UnsignedInteger32 newColourCount) {
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<SignedInteger32> dist(0, 255);

    Vector<ColourRGB> newColours; newColours.reserve(newColourCount);

    while (newColours.size() < newColourCount) {
        ColourRGB colour{ static_cast<UnsignedInteger8>(dist(gen)),
              static_cast<UnsignedInteger8>(dist(gen)),
              static_cast<UnsignedInteger8>(dist(gen))};

        if (usedColours.insert(colour.ToInteger()).second)
            newColours.push_back(colour);
    }

    return newColours;
}

Vector<ColourRGB> GenerateRandomColoursInRange(Set<UnsignedInteger32>& usedColours, const UnsignedInteger32 newColourCount, 
    const ColourRGB colour, const UnsignedInteger8 range) {
    SignedInteger16 r0 = (colour.r > range) ? colour.r - range : 0;
    SignedInteger16 r1 = (colour.r + range < 255) ? colour.r + range : 255;
    SignedInteger16 g0 = (colour.g > range) ? colour.g - range : 0;
    SignedInteger16 g1 = (colour.g + range < 255) ? colour.g + range : 255;
    SignedInteger16 b0 = (colour.b > range) ? colour.b - range : 0;
    SignedInteger16 b1 = (colour.b + range < 255) ? colour.b + range : 255;

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<SignedInteger16> dist_r(r0, r1);
    std::uniform_int_distribution<SignedInteger16> dist_g(g0, g1);
    std::uniform_int_distribution<SignedInteger16> dist_b(b0, b1);

    Vector<ColourRGB> newColours; newColours.reserve(newColourCount);

    while (newColours.size() < newColourCount) {
        ColourRGB colour{ 
            static_cast<UnsignedInteger8>(dist_r(gen)), 
            static_cast<UnsignedInteger8>(dist_g(gen)),
            static_cast<UnsignedInteger8>(dist_b(gen)) 
        };

        if (usedColours.insert(colour.ToInteger()).second)
            newColours.push_back(colour);
    }

    return newColours;
}