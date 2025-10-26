#include "data_types.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

#include "functions.hpp"

Decimal::Decimal() : value(0) {}

Decimal::Decimal(int i) : value(static_cast<SignedInteger64>(i) * 100000) {}
Decimal::Decimal(Float32 f) : value(static_cast<SignedInteger64>(std::round(f * 100000))) {}
Decimal::Decimal(Float64 d) : value(static_cast<SignedInteger64>(std::round(d * 100000))) {}
Decimal::Decimal(SignedInteger64 raw, bool) : value(raw) {}

Decimal::operator double() const { return value / 100000.0; }
Decimal::operator float() const { return static_cast<float>(value / 100000.0); }
Decimal::operator int() const { return static_cast<int>(value / 100000); }

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