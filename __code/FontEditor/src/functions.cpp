#include "functions.hpp"

#include <fstream>
#include <iostream>

[[noreturn]] void FATALERROR(const String& msg, const char* file, int line) {
    std::cerr << "Fatal error at " << file << ":" << line << ": " << msg << "\n";
    std::exit(EXIT_FAILURE);
}

Boolean CharIsCapitalOrNumber(const Char c) { return (c >= 48 && c <= 57) || (c >= 65 && c <= 90); }
Boolean CharIsCapital(const Char c) { return c >= 65 && c <= 90; }
Boolean CharIsLower(const Char c) { return c >= 97 && c <= 122; }
Boolean CharIsNumber(const Char c) { return c >= 48 && c <= 57; }

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

//                                    32 = ' ', 10 = '\n', 9 = '\t', 11 = '\v', 12 = '\f', 13 = '\r'
Boolean CharIsWhitespace(Char c) { return c == 32 || (c >= 9 && c <= 13); }

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
                if (bracketCount > 1) {
                    valueArray[currentIndex++] = c;
                }
            }
            else if (c == 125 && !inQuotation) {
                --bracketCount;

                if (bracketCount == 0) {
                    valueArray[currentIndex++] = 0;
                    onValue = false;
                    currentIndex = 0;
                    returnMap[String(keyArray)] = String(valueArray);
                }
                else {
                    valueArray[currentIndex++] = c;
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
                if (bracketCount > 1) {
                    valueArray[currentIndex++] = c;
                }
            }
            else if (c == 125 && !inQuotation) {
                --bracketCount;

                if (bracketCount == 0) {
                    valueArray[currentIndex++] = 0;
                    onValue = false;
                    currentIndex = 0;
                    returnMap[String(keyArray)].emplace_back(valueArray);
                }
                else {
                    valueArray[currentIndex++] = c;
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

Vector<String> ParseStringAsStringArray(const String& stringIn, Boolean ignoreQuotations) {
    Vector<String> returnVector;
    SizeT stringLength = stringIn.size();

    Char* currentStringArray = new Char[stringLength + 2];
    SizeT currentStringSize = 0;
    Boolean inQuotations = false;

    SizeT entriesCount = 1;

    for (Char c : stringIn) { if (CharIsWhitespace(c)) { ++entriesCount; } }
    returnVector.reserve(entriesCount);

    for (Char c : stringIn) {
        if (c == '\"' && !ignoreQuotations) {
            inQuotations = !inQuotations;
        }
        else if (!CharIsWhitespace(c) || inQuotations == true) {
            currentStringArray[currentStringSize++] = c;
        }
        else if (CharIsWhitespace(c) && inQuotations == false) {
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

Vector<SignedInteger64> ParseStringAsSignedInteger64Array(const String& stringIn, Boolean ignoreQuotations) {
    Vector<SignedInteger64> returnVector;
    SizeT stringLength = stringIn.size();

    Char* currentStringArray = new Char[stringLength + 2];
    SizeT currentStringSize = 0;
    Boolean inQuotations = false;

    String currentNumber;

    SizeT entriesCount = 1;

    for (Char c : stringIn) { if (CharIsWhitespace(c)) { ++entriesCount; } }
    returnVector.reserve(entriesCount);

    for (Char c : stringIn) {
        if (c == '\"' && !ignoreQuotations) {
            inQuotations = !inQuotations;
        }
        else if (!CharIsWhitespace(c) || inQuotations == true) {
            currentStringArray[currentStringSize++] = c;
        }
        else if (CharIsWhitespace(c) && inQuotations == false) {
            currentStringArray[currentStringSize++] = 0;
            currentStringSize = 0;
            currentNumber = String(currentStringArray);
            if (StringCanBecomeInteger(currentNumber)) { returnVector.push_back(std::stoll(currentNumber)); }
        }
    }

    if (currentStringSize > 0) {
        currentStringArray[currentStringSize++] = 0;
        currentNumber = String(currentStringArray);
        if (StringCanBecomeInteger(currentNumber)) { returnVector.push_back(std::stoll(currentNumber)); }
    }
    delete[] currentStringArray;
    return returnVector;
}

Vector<Float64> ParseStringAsFloat64Array(const String& stringIn, Boolean ignoreQuotations) {
    Vector<Float64> returnVector;
    SizeT stringLength = stringIn.size();

    Char* currentStringArray = new Char[stringLength + 2];
    SizeT currentStringSize = 0;
    Boolean inQuotations = false;

    String currentNumber;

    SizeT entriesCount = 1;

    for (Char c : stringIn) { if (CharIsWhitespace(c)) { ++entriesCount; } }
    returnVector.reserve(entriesCount);

    for (Char c : stringIn) {
        if (c == '\"' && !ignoreQuotations) {
            inQuotations = !inQuotations;
        }
        else if (!CharIsWhitespace(c) || inQuotations == true) {
            currentStringArray[currentStringSize++] = c;
        }
        else if (CharIsWhitespace(c) && inQuotations == false) {
            currentStringArray[currentStringSize++] = 0;
            currentStringSize = 0;
            currentNumber = String(currentStringArray);
            if (StringCanBecomeFloat(currentNumber)) { returnVector.push_back(std::stod(currentNumber)); }
        }
    }

    if (currentStringSize > 0) {
        currentStringArray[currentStringSize++] = 0;
        currentNumber = String(currentStringArray);
        if (StringCanBecomeFloat(currentNumber)) { returnVector.push_back(std::stod(currentNumber)); }
    }
    delete[] currentStringArray;
    return returnVector;
}

Vector<UnsignedInteger16> ParseStringAsUnsignedInteger16Array(const String& stringIn, Boolean ignoreQuotations) {
    Vector<UnsignedInteger16> returnVector;
    SizeT stringLength = stringIn.size();

    Char* currentStringArray = new Char[stringLength + 2];
    SizeT currentStringSize = 0;
    Boolean inQuotations = false;

    String currentNumber;

    SizeT entriesCount = 1;

    for (Char c : stringIn) { if (CharIsWhitespace(c)) { ++entriesCount; } }
    returnVector.reserve(entriesCount);

    for (Char c : stringIn) {
        if (c == '\"' && !ignoreQuotations) {
            inQuotations = !inQuotations;
        }
        else if (!CharIsWhitespace(c) || inQuotations == true) {
            currentStringArray[currentStringSize++] = c;
        }
        else if (CharIsWhitespace(c) && inQuotations == false) {
            currentStringArray[currentStringSize++] = 0;
            currentStringSize = 0;
            currentNumber = String(currentStringArray);
            if (StringCanBecomeInteger(currentNumber)) { returnVector.push_back(std::stoi(currentNumber)); }
        }
    }

    if (currentStringSize > 0) {
        currentStringArray[currentStringSize++] = 0;
        currentNumber = String(currentStringArray);
        if (StringCanBecomeInteger(currentNumber)) { returnVector.push_back(std::stoi(currentNumber)); }
    }
    delete[] currentStringArray;
    return returnVector;
}