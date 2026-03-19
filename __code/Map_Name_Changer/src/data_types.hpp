#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <filesystem>
#include <chrono>
#include <array>

//  ----Rename Types----

using Char = char;
using UnsignedChar = unsigned char;

using UnsignedInteger8 = uint8_t;
using UnsignedInteger16 = uint16_t;
using UnsignedInteger32 = uint32_t;
using UnsignedInteger64 = uint64_t;

using SignedInteger8 = int8_t;
using SignedInteger16 = int16_t;
using SignedInteger32 = int32_t;
using SignedInteger64 = int64_t;

using SizeT = size_t;

using Float32 = float;
using Float64 = double;

using Boolean = bool;

using String = std::string;

using Path = std::filesystem::path;

using Timestamp = std::chrono::steady_clock::time_point;

template<typename HashKey, typename HashValue>
using HashMap = std::unordered_map<HashKey, HashValue>;

template<typename SetType>
using Set = std::unordered_set<SetType>;

template<typename VectorType>
using Vector = std::vector<VectorType>;

template<typename ArrayType, SizeT amount>
using Array = std::array<ArrayType, amount>;

// ----Custom Data Structures----
struct DoubleString {
public :
    String a, b;

    DoubleString() : a(""), b("") {};
    DoubleString(const String& x, const String& y) : a(x), b(y) {};
};

// ----Decimal----
//HoI4 uses a 32-bit number to 3 decimal points but Vicky 3 uses a 64-bit to 5, so let's use that instead because why not

struct Decimal {
private:
    SignedInteger64 value;

public:
    Decimal();
    Decimal(SignedInteger32 i);
    Decimal(UnsignedInteger32 i);
    Decimal(SignedInteger64 i);
    Decimal(UnsignedInteger64 i);
    Decimal(Float32 f);
    Decimal(Float64 d);
    Decimal(String str);
    explicit Decimal(SignedInteger64 raw, bool);

    Decimal(const char* str);

    operator SignedInteger32() const;
    operator UnsignedInteger32() const;
    operator SignedInteger64() const;
    operator UnsignedInteger64() const;
    operator Float64() const;
    operator Float32() const;

    Decimal operator+(const Decimal& other) const;
    Decimal operator-(const Decimal& other) const;
    Decimal operator*(const Decimal& other) const;
    Decimal operator/(const Decimal& other) const;

    Decimal& operator+=(const Decimal& other);
    Decimal& operator-=(const Decimal& other);
    Decimal& operator*=(const Decimal& other);
    Decimal& operator/=(const Decimal& other);

    bool operator==(const Decimal& o) const;
    bool operator!=(const Decimal& o) const;
    bool operator<(const Decimal& o) const;
    bool operator<=(const Decimal& o) const;
    bool operator>(const Decimal& o) const;
    bool operator>=(const Decimal& o) const;

    SignedInteger64 GetRawValue();
    const SignedInteger64 GetRawValue() const;

    String ToString(SignedInteger16 precision = 3) const;
    friend std::ostream& operator<<(std::ostream& os, const Decimal& d);
};

struct ChangeableName {
public:
    String name;
    Vector<String> nameRequirements;

    ChangeableName() : name(""), nameRequirements() {};
    ChangeableName(const String& name, const Vector<String>& nameRequirements) :
		name(name), nameRequirements(nameRequirements) {
	};
};

struct Province {
private:
    UnsignedInteger32 id;

    String defaultName;
    Vector<ChangeableName> nameEntries;

public:
    Province() : id(0), defaultName(""), nameEntries() {};
    Province(const UnsignedInteger32 id) : id(id), defaultName(""), nameEntries() {};

    void SetId(const UnsignedInteger32 idIn);
    UnsignedInteger32 GetId();
    const UnsignedInteger32 GetId() const;

    String GetDefaultName();
	const String GetDefaultName() const;
	void SetDefaultName(const String& name);
    Vector<ChangeableName>& GetNameEntries();
    const Vector<ChangeableName>& GetNameEntries() const;
	void SetNameEntries(const Vector<ChangeableName>& entries);
    SizeT GetChangeableNameCount();
    const SizeT GetChangeableNameCount() const;
};

struct State {
private:
    UnsignedInteger32 id;
    Vector<UnsignedInteger32> provinces;
    String defaultName;
    Vector<ChangeableName> nameEntries;

public:
    State(const UnsignedInteger32 id) : id(id), provinces(), defaultName(""), nameEntries() {}
    State(const UnsignedInteger32 id, const Vector<UnsignedInteger16>& provinces) : id(id), provinces(provinces), defaultName(""), nameEntries() {}

    void SetId(const UnsignedInteger32 idIn);
    UnsignedInteger32 GetId();
    const UnsignedInteger32 GetId() const;

    const Vector<UnsignedInteger32>& GetProvinces() const;
    Vector<UnsignedInteger32>& GetProvinces();
    void SortProvinces();

    String GetDefaultName();
    const String GetDefaultName() const;
    void SetDefaultName(const String& name);
    Vector<ChangeableName>& GetNameEntries();
    const Vector<ChangeableName>& GetNameEntries() const;
    void SetNameEntries(const Vector<ChangeableName>& entries);
	SizeT GetChangeableNameCount();
	const SizeT GetChangeableNameCount() const;
};