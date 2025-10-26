#include "functions.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include "data_types.hpp"


[[noreturn]] void FATALERROR(const String& msg, const char* file, int line) {
    std::cerr << "Fatal error at " << file << ":" << line << ": " << msg << "\n";
    std::exit(EXIT_FAILURE);
}

//Get time elapsed since beginning of program
UnsignedInteger64 GetTimeElapsedFromStart(const Timestamp& startTime) {
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    return duration.count();
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

static Boolean CharIsCapitalOrNumber(const Char c) { return (c >= 48 && c <= 57) || (c >= 65 && c <= 90); }
static Boolean CharIsCapital(const Char c) { return c >= 65 && c <= 90; }
static Boolean CharIsLower(const Char c) { return c >= 97 && c <= 122; }
static Boolean CharIsNumber(const Char c) { return c >= 48 && c <= 57; }

String RemoveQuotes(String str) {
    Char first = str.front();
    Char last = str.back();

    // '"' & '\''
    if ((first == 34 && last == 34) || (first == 39 && last == 39)) { str = str.substr(1, str.size() - 2); }
    return str;
}

String ForwardToBackslashes(String str) { 
    for (char& c : str) { if (c == 47) { c = 92; } } 
    return str;
}

String ToUpper(String str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return str;
}
String ToLower(String str) {
    std::transform(str.begin(), str.end(), str.begin(),
        [](unsigned char c) { return std::tolower(c); });
    return str;
}

Boolean StringCanBecomeInteger(const String& str) {
    SizeT stringLength = str.size();
    for (SizeT i = 0; i < stringLength; ++i) { 
        if (i == 0 && !CharIsNumber(str[i]) && str[i] != 43 && str[i] != 45) return false; 
        else if (!CharIsNumber(str[i])) return false; 
    }
    return true;
}
Boolean StringCanBecomeFloat(const String& str) {
    if (str.ends_with(".") || str.ends_with("+") || str.ends_with("-")) return false;

    SizeT stringLength = str.size(); 
    UnsignedInteger64 dotCount = 0;
    for (SizeT i = 0; i < stringLength; ++i) { 
        if (i == 0 && !CharIsNumber(str[i]) && str[i] != 43 && str[i] != 45) return false;
        else if (i == 0 && (str[i] == 43 || str[i] == 45) && str.length() > 1) { 
            if (str[1] == 46) return false; 
        }
        else if (i > 0 && str[i] == 46) { if (dotCount > 0) { return false; } dotCount++; }
        else if (i > 0 && !CharIsNumber(str[i])) return false;
    }
    return true;
}

Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String folderPath, const Vector<String>& fileTypes, UnsignedInteger16 reserve) {
    Vector<Path> filesReturnVector;
    filesReturnVector.reserve(reserve);

    folderPath = ForwardToBackslashes(folderPath);

    const Boolean modReplacesDirectory = std::binary_search(modReplaceDirectories.begin(), modReplaceDirectories.end(), folderPath);
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
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }
    //If the mod does not replace the directory and there is no equivilent directory in our mod OR if it does exist but is empty, load solely from vanilla
    else if (!modReplacesDirectory && (!modFolderExists || (modFolderExists && std::filesystem::is_empty(modFolder)))) {
        SizeT fileCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(vanillaFolder)) { if (entry.is_regular_file()) { ++fileCount; } }
        filesReturnVector.reserve(fileCount);

        for (const auto& file : std::filesystem::directory_iterator(vanillaFolder)) {
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
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
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
                //Create a hypothetical file to check for in our mod folder - if it exists, don't add the current vanilla file
                Path fileToCheckFor = modDirectory / folderPath / file.path().filename();
                if (!std::filesystem::exists(fileToCheckFor)) {
                    filesReturnVector.emplace_back(file.path());
                }
            }
        }

        //Now load all mod files
        for (const auto& file : std::filesystem::directory_iterator(modFolder)) {
            if (file.is_regular_file() && std::find(fileTypes.begin(), fileTypes.end(), file.path().extension()) != fileTypes.end()) {
                filesReturnVector.emplace_back(file.path());
            }
        }
    }

    return filesReturnVector;
}

Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String folderPath, const String& fileType, UnsignedInteger16 reserve) {
    Vector<Path> filesReturnVector;
    filesReturnVector.reserve(reserve);

    folderPath = ForwardToBackslashes(folderPath);

    const Boolean modReplacesDirectory = std::binary_search(modReplaceDirectories.begin(), modReplaceDirectories.end(), folderPath);
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
    //If the mod does not replace the directory and there is no equivilent directory in our mod OR if it does exist but is empty, load solely from vanilla
    else if (!modReplacesDirectory && (!modFolderExists || (modFolderExists && std::filesystem::is_empty(modFolder)))) {
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

Vector<Path> GetGameFiles(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String folderPath, UnsignedInteger16 reserve) {
    Vector<Path> filesReturnVector;
    filesReturnVector.reserve(reserve);

    folderPath = ForwardToBackslashes(folderPath);

    const Boolean modReplacesDirectory = std::binary_search(modReplaceDirectories.begin(), modReplaceDirectories.end(), folderPath);
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
    //If the mod does not replace the directory and there is no equivilent directory in our mod OR if it does exist but is empty, load solely from vanilla
    else if (!modReplacesDirectory && (!modFolderExists || (modFolderExists && std::filesystem::is_empty(modFolder)))) {
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

Path GetGameFile(const Path& vanillaDirectory, const Path& modDirectory, const Vector<String>& modReplaceDirectories, String path) {
    path = ForwardToBackslashes(path);

    const Path vanillaPath = vanillaDirectory / path;
    const Path modPath = modDirectory / path;

    Path p(path);
    Path directory = p.parent_path();
    Path fileName = p.filename();

    const Boolean modReplacesDirectory = std::binary_search(modReplaceDirectories.begin(), modReplaceDirectories.end(), directory);

    if (std::filesystem::exists(modPath) && std::filesystem::is_regular_file(modPath)) { return modPath; }
    else if (modReplacesDirectory) { FatalError("File " + path + " does not exist in the mod"); }
    else if (!(std::filesystem::exists(vanillaPath) && std::filesystem::is_regular_file(vanillaPath))) { FatalError("File " + path + " does not exist"); }

    return vanillaPath;
}

String LoadFileToString(const String& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) FatalError("Failed to open file: " + filename);

    std::ostringstream result;
    std::string line;

    while (std::getline(file, line)) {
        Boolean inSingleQuotes = false;
        Boolean inDoubleQuotes = false;

        String processedLine;
        processedLine.reserve(line.size());

        for (SizeT i = 0; i < line.size(); ++i) {
            Char c = line[i];

            if (c == '\'' && !inDoubleQuotes && (i == 0 || line[i - 1] != '\\')) { inSingleQuotes = !inSingleQuotes; }
            else if (c == '"' && !inSingleQuotes && (i == 0 || line[i - 1] != '\\')) { inDoubleQuotes = !inDoubleQuotes; }

            if (c == '#' && !inSingleQuotes && !inDoubleQuotes) { break; }

            processedLine += c;
        }

        result << processedLine << '\n';
    }

    return result.str();
}

//                                        32 = ' ', 10 = '\n', 9 = '\t', 11 = '\v', 12 = '\f', 13 = '\r'
static Boolean CharIsWhitespace(Char c) { return c == 32 || (c >= 9 && c <= 13); }

static String RemoveStringWhitespace(const String& stringIn) {
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
                returnCharArray[returnStringSize++] = 32;
                inWhitespace = true;
            }
        }
        else {
            returnCharArray[returnStringSize++] = c;
            inWhitespace = false;
            started = true;
        }
    }

    if (returnStringSize > 0 && returnCharArray[returnStringSize - 1] == 32) { --returnStringSize; }

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
        //          =           {           }
        return c == 61 || c == 123 || c == 125;
    };

    for (SizeT i = 0; i < stringLength; ++i) {
        Char c = processedString[i];

        if (c == 34) inQuotation = !inQuotation;

        //If whitespace - if character to our left or right is '={}', don't write the whitespace
        if (c == 32 && !inQuotation) {
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

HashMap<String, String> ParseStringForPairsMapUnique(const String& stringIn) {
    HashMap<String, String> returnMap;

    String processingString = PrepareStringForParse(stringIn);
    SizeT stringLength = processingString.size();

    Char* keyArray = new Char[stringLength + 3]; keyArray[0] = 0;
    Char* valueArray = new Char[stringLength + 3]; valueArray[0] = 0;
    UnsignedInteger64 currentIndex = 0;
    UnsignedInteger64 bracketCount = 0;
    Boolean onValue = false;
    Boolean inQuotation = false;

    //If squiggly bracket - ignore, if space - switch, else add to string
    for (Char c : processingString) {
        if (c == 34) inQuotation = !inQuotation;

        switch(onValue) {
            case 0:
                if ((c == 32 || c == 61) && !inQuotation) {
                    keyArray[currentIndex++] = 0;
                    onValue = true;
                    currentIndex = 0; 
                }
                else {
                    keyArray[currentIndex++] = c;
                }
                break;
       
            default:
                if (c == 123 && !inQuotation) {
                    ++bracketCount;
                }
                else if (c == 125 && !inQuotation) {
                    --bracketCount;
                    
                    if (bracketCount == 0) {
                        valueArray[currentIndex++] = 0;
                        onValue = false;
                        currentIndex = 0;
                        returnMap[String(keyArray)] = String(valueArray);
                    }
                }
                else if (c == 32 && !inQuotation && bracketCount == 0) {
                    valueArray[currentIndex++] = 0;
                    onValue = false;
                    currentIndex = 0; 
                    returnMap[String(keyArray)] = String(valueArray);
                }
                else {
                    valueArray[currentIndex++] = c;
                }
        }
    }

    if (currentIndex > 0) {
        valueArray[currentIndex++] = 0;
        returnMap[String(keyArray)] = String(valueArray);
    }

    delete[] keyArray;
    delete[] valueArray;
    return returnMap;
}

HashMap<String, Vector<String>> ParseStringForPairsMapRepeat(const String& stringIn) {
    HashMap<String, Vector<String>> returnMap;

    String processingString = PrepareStringForParse(stringIn);
    SizeT stringLength = processingString.size();

    Char* keyArray = new Char[stringLength + 3]; keyArray[0] = 0;
    Char* valueArray = new Char[stringLength + 3]; valueArray[0] = 0;
    UnsignedInteger64 currentIndex = 0;
    UnsignedInteger64 bracketCount = 0;
    Boolean onValue = false;
    Boolean inQuotation = false;

    //If squiggly bracket - ignore, if space - switch, else add to string
    for (Char c : processingString) {
        if (c == 34) inQuotation = !inQuotation;

        switch(onValue) {
            case 0:
                if ((c == 32 || c == 61) && !inQuotation) {
                    keyArray[currentIndex++] = 0;
                    onValue = true;
                    currentIndex = 0; 
                }
                else {
                    keyArray[currentIndex++] = c;
                }
                break;
       
            default:
                if (c == 123 && !inQuotation) {
                    ++bracketCount;
                }
                else if (c == 125 && !inQuotation) {
                    --bracketCount;
                    
                    if (bracketCount == 0) {
                        valueArray[currentIndex++] = 0;
                        onValue = false;
                        currentIndex = 0;
                        returnMap[String(keyArray)].emplace_back(valueArray);
                    }
                }
                else if (c == 32 && !inQuotation && bracketCount == 0) {
                    valueArray[currentIndex++] = 0;
                    onValue = false;
                    currentIndex = 0; 
                    returnMap[String(keyArray)].emplace_back(valueArray);
                }
                else {
                    valueArray[currentIndex++] = c;
                }
        }
    }

    if (currentIndex > 0) {
        valueArray[currentIndex++] = 0;
        returnMap[String(keyArray)].emplace_back(valueArray);
    }

    delete[] keyArray;
    delete[] valueArray;
    return returnMap;
}

Vector<DoubleString> ParseStringForPairsArray(const String& stringIn, UnsignedInteger32 reserve) {
    Vector<DoubleString> returnArray;
    returnArray.reserve(reserve);

    String processingString = PrepareStringForParse(stringIn);
    SizeT stringLength = processingString.size();

    Char* keyArray = new Char[stringLength + 3]; keyArray[0] = 0;
    Char* valueArray = new Char[stringLength + 3]; valueArray[0] = 0;
    UnsignedInteger64 currentIndex = 0;
    UnsignedInteger64 bracketCount = 0;
    Boolean onValue = false;
    Boolean inQuotation = false;

    //If squiggly bracket - ignore, if space - switch, else add to string
    for (Char c : processingString) {
        if (c == 34) inQuotation = !inQuotation;

        switch (onValue) {
        case 0:
            if ((c == 32 || c == 61) && !inQuotation) {
                keyArray[currentIndex++] = 0;
                onValue = true;
                currentIndex = 0;
            }
            else {
                keyArray[currentIndex++] = c;
            }
            break;

        default:
            if (c == 123 && !inQuotation) {
                ++bracketCount;
            }
            else if (c == 125 && !inQuotation) {
                --bracketCount;

                if (bracketCount == 0) {
                    valueArray[currentIndex++] = 0;
                    onValue = false;
                    currentIndex = 0;
                    returnArray.emplace_back(keyArray, valueArray);
                }
            }
            else if (c == 32 && !inQuotation && bracketCount == 0) {
                valueArray[currentIndex++] = 0;
                onValue = false;
                currentIndex = 0;
                returnArray.emplace_back(keyArray, valueArray);
            }
            else {
                valueArray[currentIndex++] = c;
            }
        }
    }

    if (currentIndex > 0) {
        valueArray[currentIndex++] = 0;
        returnArray.emplace_back(keyArray, valueArray);
    }

    delete[] keyArray;
    delete[] valueArray;
    return returnArray;
}

Vector<String> ParseStringAsArray(const String& stringIn, Boolean ignoreQuotations) {
    Vector<String> returnVector;
    SizeT stringLength = stringIn.size();

    Char* currentStringArray = new Char[stringLength + 2];
    SizeT currentStringSize = 0;

    SizeT entriesCount = 1;

    for (Char c : stringIn) { if (CharIsWhitespace(c)) { ++entriesCount; } }
    returnVector.reserve(entriesCount);

    for (Char c : stringIn) { 
        if (!CharIsWhitespace(c) && (c != 34 || !ignoreQuotations)) {
            currentStringArray[currentStringSize++] = c;
        } 
        else if (CharIsWhitespace(c)){
            currentStringArray[currentStringSize++] = 0;
            currentStringSize = 0;
            returnVector.emplace_back(currentStringArray);
        }
    }

    if (currentStringSize > 0) {
        currentStringArray[currentStringSize++] = 0;
        returnVector.emplace_back(currentStringArray);
    }
    delete[] currentStringArray;
    return returnVector;
}

Boolean TagIsValid(const String& tag) {
    if (tag.size() != 3) { return false; }
    if (!CharIsCapital(tag[0]) || !CharIsCapitalOrNumber(tag[1]) || !CharIsCapitalOrNumber(tag[2])) { return false; }
    if (tag == "NOT" || tag == "AND" || tag == "TAG" || tag == "OOB" || tag == "LOG" || tag == "NUM" || tag == "RED") { return false; }

    return true;
}