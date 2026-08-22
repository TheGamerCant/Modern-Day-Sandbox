#include "data_types.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

#include "functions.hpp"

ColourRGB::ColourRGB() : r(0), g(0), b(0) {}
ColourRGB::ColourRGB(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b) : r(r), g(g), b(b) {}
ColourRGB::ColourRGB(const String& str) : r(0), g(0), b(0) {
    Char* charArray = new Char[str.size() + 2];
    SizeT arraySize = 0;
    UnsignedInteger8 colour = 0;

    for (const auto& c : str) {
        if (colour > 2) break;

        if (CharIsNumber(c)) {
            charArray[arraySize++] = c;
        }
        else if (CharIsWhitespace(c) && (colour > 0 || arraySize > 0)) {
            charArray[arraySize++] = 0;
            switch (colour) {
                case 0:
                    r = std::stoi(String(charArray));
                    break;
                case 1:
                    g = std::stoi(String(charArray));
                    break;
                default:
                    b = std::stoi(String(charArray));
            }
            arraySize = 0;
            ++colour;
        }
        else if (colour > 0 || arraySize > 0) {
            FatalError("Bad character in ColourRGB intialisation string \"" + str + "\"");
        }
    }

    if (colour < 3 && arraySize > 0) {
        charArray[arraySize++] = 0;
        switch (colour) {
        case 0:
            r = std::stoi(String(charArray));
            break;
        case 1:
            g = std::stoi(String(charArray));
            break;
        default:
            b = std::stoi(String(charArray));
        }
    }

    delete[] charArray;
}
ColourRGB::ColourRGB(const ColourRGBA rgba) : r(rgba.r), g(rgba.g), b(rgba.b) {}
UnsignedInteger32 ColourRGB::ToInteger() {
    return (static_cast<UnsignedInteger32>(r) << 16) |
        (static_cast<UnsignedInteger32>(g) << 8) |
        (static_cast<UnsignedInteger32>(b));
}
const UnsignedInteger32 ColourRGB::ToInteger() const {
    return (static_cast<UnsignedInteger32>(r) << 16) |
        (static_cast<UnsignedInteger32>(g) << 8) |
        (static_cast<UnsignedInteger32>(b));
}
String ColourRGB::ToHex() {
    std::stringstream stream;
    stream << std::hex << ToInteger();
    return stream.str();
}
const String ColourRGB::ToHex() const {
    std::stringstream stream;
    stream << std::hex << ToInteger();
    return stream.str();
}

ColourRGBA::ColourRGBA() : r(0), g(0), b(0), a(255) {}
ColourRGBA::ColourRGBA(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b) : r(r), g(g), b(b), a(255) {}
ColourRGBA::ColourRGBA(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b, const UnsignedInteger8 a) : r(r), g(g), b(b), a(a) {}
ColourRGBA::ColourRGBA(const String& str) : r(0), g(0), b(0), a(255) {
    Char* charArray = new Char[str.size() + 2];
    SizeT arraySize = 0;
    UnsignedInteger8 colour = 0;

    for (const auto& c : str) {
        if (colour > 3) break;

        if (CharIsNumber(c)) {
            charArray[arraySize++] = c;
        }
        else if (CharIsWhitespace(c) && (colour > 0 || arraySize > 0)) {
            charArray[arraySize++] = 0;
            switch (colour) {
                case 0:
                    r = std::stoi(String(charArray));
                    break;
                case 1:
                    g = std::stoi(String(charArray));
                    break;
                case 2:
                    b = std::stoi(String(charArray));
                    break;
                default:
                    a = std::stoi(String(charArray));
            }
            arraySize = 0;
            ++colour;
        }
        else if (colour > 0 || arraySize > 0) {
            FatalError("Bad character in ColourRGBA intialisation string \"" + str + "\"");
        }
    }

    if (colour < 4 && arraySize > 0) {
        charArray[arraySize++] = 0;
        switch (colour) {
        case 0:
            r = std::stoi(String(charArray));
            break;
        case 1:
            g = std::stoi(String(charArray));
            break;
        case 2:
            b = std::stoi(String(charArray));
            break;
        default:
            a = std::stoi(String(charArray));
        }
    }

    delete[] charArray;
}
ColourRGBA::ColourRGBA(const ColourRGB rgb) : r(rgb.r), g(rgb.g), b(rgb.b), a(255) {}
UnsignedInteger32 ColourRGBA::ToInteger() {
    return (static_cast<UnsignedInteger32>(r) << 24) |
        (static_cast<UnsignedInteger32>(g) << 16) |
        (static_cast<UnsignedInteger32>(b) << 8) |
        (static_cast<UnsignedInteger32>(a));
}
const UnsignedInteger32 ColourRGBA::ToInteger() const {
    return (static_cast<UnsignedInteger32>(r) << 24) |
        (static_cast<UnsignedInteger32>(g) << 16) |
        (static_cast<UnsignedInteger32>(b) << 8) |
        (static_cast<UnsignedInteger32>(a));
}
String ColourRGBA::ToHex() {
    std::stringstream stream;
    stream << std::hex << ToInteger();
    return stream.str();
}
const String ColourRGBA::ToHex() const {
    std::stringstream stream;
    stream << std::hex << ToInteger();
    return stream.str();
}