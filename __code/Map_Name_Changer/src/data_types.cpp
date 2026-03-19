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

    value = std::stoll(String(charArray));
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

SignedInteger64 Decimal::GetRawValue() { return value; }
const SignedInteger64 Decimal::GetRawValue() const { return value; }

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

void Province::SetId(const UnsignedInteger32 idIn) { id = idIn; }
UnsignedInteger32 Province::GetId() { return id; }
const UnsignedInteger32 Province::GetId() const { return id; }

String Province::GetDefaultName() { return defaultName; }
const String Province::GetDefaultName() const { return defaultName; }
void Province::SetDefaultName(const String& name) { defaultName = name; }
Vector<ChangeableName>& Province::GetNameEntries() { return nameEntries; }
const Vector<ChangeableName>& Province::GetNameEntries() const { return nameEntries; }
void Province::SetNameEntries(const Vector<ChangeableName>& entries) { nameEntries = entries; }
SizeT Province::GetChangeableNameCount() { return nameEntries.size(); }
const SizeT Province::GetChangeableNameCount() const { return nameEntries.size(); }

void State::SetId(const UnsignedInteger32 idIn) { id = idIn; }
UnsignedInteger32 State::GetId() { return id; }
const UnsignedInteger32 State::GetId() const { return id; }

const Vector<UnsignedInteger32>& State::GetProvinces() const { return provinces; }
Vector<UnsignedInteger32>& State::GetProvinces() { return provinces; }
void State::SortProvinces() { std::sort(provinces.begin(), provinces.end()); }

String State::GetDefaultName() { return defaultName; }
const String State::GetDefaultName() const { return defaultName; }
void State::SetDefaultName(const String& name) { defaultName = name; }
Vector<ChangeableName>& State::GetNameEntries() { return nameEntries; }
const Vector<ChangeableName>& State::GetNameEntries() const { return nameEntries; }
void State::SetNameEntries(const Vector<ChangeableName>& entries) { nameEntries = entries; }
SizeT State::GetChangeableNameCount() { return nameEntries.size(); }
const SizeT State::GetChangeableNameCount() const { return nameEntries.size(); }