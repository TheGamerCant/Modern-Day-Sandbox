#include "data_types.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

#include "functions.hpp"

Decimal::Decimal() : value(0) {}

Decimal::Decimal(SignedInteger32 i) : value(static_cast<SignedInteger64>(i) * 100000) {}
Decimal::Decimal(UnsignedInteger32 i) : value(static_cast<SignedInteger64>(i) * 100000) {}
Decimal::Decimal(SignedInteger64 i) : value(i * 100000) {}
Decimal::Decimal(UnsignedInteger64 i) : value(static_cast<SignedInteger64>(i) * 100000) {}
Decimal::Decimal(Float32 f) : value(static_cast<SignedInteger64>(std::round(f * 100000))) {}
Decimal::Decimal(Float64 d) : value(static_cast<SignedInteger64>(std::round(d * 100000))) {}
Decimal::Decimal(String str) {
    Boolean decimalPointFound = false;
    SizeT numbersAfterDecimalPoint = 0;

    SizeT stringLength = str.size();
    Char* charArray = new Char[stringLength + 6];
    UnsignedInteger64 charArrayLength = 0;

    if (!StringCanBecomeFloat(str)) FatalError("String \"" + str +"\" cannot become a decimal number");

    for (const auto& c : str) {
        //We can ignore the addition sign
        if (c == 43) {}
        else if (c == 46) {
            decimalPointFound = true;
        }
        else if (numbersAfterDecimalPoint < 5){
            charArray[charArrayLength++] = c;
            if (decimalPointFound) ++numbersAfterDecimalPoint;
        }
    }

    if (numbersAfterDecimalPoint < 5) {
        const UnsignedInteger8 zeroesToAdd = 5 - numbersAfterDecimalPoint;
        for (SizeT i = 0; i < zeroesToAdd; ++i) {
            charArray[charArrayLength++] = 48;
        }
    }

    charArray[charArrayLength++] = 0;

    value = std::stoi(String(charArray));
    delete[] charArray;
}
Decimal::Decimal(SignedInteger64 raw, bool) : value(raw) {}

Decimal::operator SignedInteger32() const { return static_cast<SignedInteger32>(value) / 100000; }
Decimal::operator UnsignedInteger32() const { return static_cast<UnsignedInteger32>(value) / 100000; }
Decimal::operator SignedInteger64() const { return value / 100000; }
Decimal::operator UnsignedInteger64() const { return static_cast<UnsignedInteger64>(value) / 100000; }
Decimal::operator Float64() const { return value / 100000.0; }
Decimal::operator Float32() const { return static_cast<Float32>(value / 100000.0); }

Decimal::Decimal(const char* str) : Decimal(String(str)) {}

Decimal Decimal::operator+(const Decimal& other) const { return Decimal(value + other.value, true); }
Decimal Decimal::operator-(const Decimal& other) const { return Decimal(value - other.value, true); }
Decimal Decimal::operator*(const Decimal& other) const { return Decimal((value * other.value) / 100000, true); }
Decimal Decimal::operator/(const Decimal& other) const {
    if (other.value == 0) FatalError("Division by zero");
    return Decimal((value * 100000) / other.value, true);
}

Decimal& Decimal::operator+=(const Decimal& other) { value += other.value; return *this; }
Decimal& Decimal::operator-=(const Decimal& other) { value -= other.value; return *this; }
Decimal& Decimal::operator*=(const Decimal& other) {
    value = (value * other.value) / 100000;
    return *this;
}
Decimal& Decimal::operator/=(const Decimal& other) {
    if (other.value == 0) FatalError("Division by zero");
    value = (value * 100000) / other.value;
    return *this;
}

bool Decimal::operator==(const Decimal& o) const { return value == o.value; }
bool Decimal::operator!=(const Decimal& o) const { return value != o.value; }
bool Decimal::operator<(const Decimal& o) const { return value < o.value; }
bool Decimal::operator<=(const Decimal& o) const { return value <= o.value; }
bool Decimal::operator>(const Decimal& o) const { return value > o.value; }
bool Decimal::operator>=(const Decimal& o) const { return value >= o.value; }

String Decimal::ToString(SignedInteger16 precision) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << static_cast<Float64>(*this);
    return oss.str();
}

std::ostream& operator<<(std::ostream& os, const Decimal& d) {
    std::streamsize old_prec = os.precision();
    std::ios::fmtflags old_flags = os.flags();
    os << std::fixed << std::setprecision(old_prec) << static_cast<Float64>(d);
    os.flags(old_flags);
    return os;
}

ColourRGB::ColourRGB() : r(0), g(0), b(0) {}
ColourRGB::ColourRGB(const UnsignedInteger8 r, const UnsignedInteger8 g, const UnsignedInteger8 b) : r(r), g(g), b(b) {}
ColourRGB::ColourRGB(const String& str) {
    Char* charArray = new Char[str.size() + 2];
    SizeT arraySize = 0;
    UnsignedInteger8 colour = 0;

    for (const auto& c : str) {
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

    if (colour < 2 && arraySize > 0) {
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
}

String Country::GetTag() { return String(tag); }
const String Country::GetTag() const { return String(tag); }
void Country::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 Country::GetId() { return id; }
const UnsignedInteger16 Country::GetId() const { return id; }

String GraphicalCulture::GetName() { return name; }
const String GraphicalCulture::GetName() const { return name; }
void GraphicalCulture::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 GraphicalCulture::GetId() { return id; }
const UnsignedInteger16 GraphicalCulture::GetId() const { return id; }

String Terrain::GetName() { return name; }
const String Terrain::GetName() const { return name; }
void Terrain::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 Terrain::GetId() { return id; }
const UnsignedInteger16 Terrain::GetId() const { return id; }

String Building::GetName() { return name; }
const String Building::GetName() const { return name; }
void Building::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 Building::GetId() { return id; }
const UnsignedInteger16 Building::GetId() const { return id; }
void Building::setExclusive(const SignedInteger32 exclusive) { levelCapExclusiveWith = exclusive;  }

String BuildingSpawnPoint::GetName() { return name; }
const String BuildingSpawnPoint::GetName() const { return name; }
void BuildingSpawnPoint::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 BuildingSpawnPoint::GetId() { return id; }
const UnsignedInteger16 BuildingSpawnPoint::GetId() const { return id; }

String GraphicalTerrain::GetName() { return name; }
const String GraphicalTerrain::GetName() const { return name; }
void GraphicalTerrain::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 GraphicalTerrain::GetId() { return id; }
const UnsignedInteger16 GraphicalTerrain::GetId() const { return id; }

String Resource::GetName() { return name; }
const String Resource::GetName() const { return name; }
void Resource::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 Resource::GetId() { return id; }
const UnsignedInteger16 Resource::GetId() const { return id; }

String StateCategory::GetName() { return name; }
const String StateCategory::GetName() const { return name; }
void StateCategory::UpdateId(const UnsignedInteger16 idIn) { id = idIn; }
UnsignedInteger16 StateCategory::GetId() { return id; }
const UnsignedInteger16 StateCategory::GetId() const { return id; }