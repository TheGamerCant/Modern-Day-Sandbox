#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace PDX {

struct PdxJson;  // forward declaration (breaks the recursive size cycle)

// Types allowed as dictionary keys. Must be comparable (std::map needs
// ordering); std::variant supplies operator< automatically when every
// alternative is comparable.
using Key = std::variant<std::int64_t, double, bool, std::string>;

using List = std::vector<PdxJson>;
using Dict = std::map<Key, PdxJson>;

using ValueVariant = std::variant<
    std::nullptr_t,   // None
    bool,
    std::int64_t,
    double,
    std::string,
    List,             // list[Any]
    Dict              // dict[Any, Any]
>;

// Helper for building overloaded visitors (used by std::visit).
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

namespace detail {

// True when T is one of ValueVariant's alternatives, i.e. when std::get<T> is a
// valid thing to ask for. Everything else has to go through a conversion.
template <class T, class V> struct IsAlternative;
template <class T, class... Ts>
struct IsAlternative<T, std::variant<Ts...>> : std::bool_constant<(std::is_same_v<T, Ts> || ...)> {};

template <class T>
inline constexpr bool isAlternative = IsAlternative<T, ValueVariant>::value;

// A number the variant does not store natively, so it has to be converted on the
// way out: every integer and floating point type except bool, std::int64_t and
// double, which take the strict std::get path instead.
template <class T>
inline constexpr bool isConvertibleNumber = std::is_arithmetic_v<T> && !isAlternative<T>;

// Used only to build readable error messages, so no <typeinfo> dependency.
template <class T>
inline std::string NumberName() {
    if constexpr (std::is_same_v<T, bool>) { return "bool"; }
    else if constexpr (std::is_floating_point_v<T>) { return std::to_string(sizeof(T) * 8) + " bit float"; }
    else { return std::string(std::is_signed_v<T> ? "signed " : "unsigned ") + std::to_string(sizeof(T) * 8) + " bit integer"; }
}

// std::int64_t -> T, refusing any conversion that would change the value. This is
// the whole point of the exercise: a province id of 70000 must not quietly become
// 4464 because the target happened to be sixteen bits wide.
template <class T>
bool TryFromInt(const std::int64_t value, T& out) {
    if constexpr (std::is_floating_point_v<T>) {
        out = static_cast<T>(value);
        return true;
    }
    else if constexpr (std::is_signed_v<T>) {
        if (value < static_cast<std::int64_t>(std::numeric_limits<T>::lowest())) { return false; }
        if (value > static_cast<std::int64_t>(std::numeric_limits<T>::max())) { return false; }
        out = static_cast<T>(value);
        return true;
    }
    else {
        // Compare as unsigned, because an unsigned 64 bit maximum does not fit in
        // the signed 64 bit value being tested against it
        if (value < 0) { return false; }
        if (static_cast<std::uint64_t>(value) > static_cast<std::uint64_t>(std::numeric_limits<T>::max())) { return false; }
        out = static_cast<T>(value);
        return true;
    }
}

// double -> T. Clausewitz script writes 1.0 where it means 1 often enough that a
// whole numbered double should satisfy an integer read, but a genuine fraction
// should not silently truncate.
template <class T>
bool TryFromDouble(const double value, T& out) {
    if constexpr (std::is_floating_point_v<T>) {
        out = static_cast<T>(value);
        return true;
    }
    else {
        if (std::isnan(value) || value != std::floor(value)) { return false; }

        // Both bounds are exactly representable as doubles, so passing this test
        // makes the cast below well defined
        if (!(value >= -9223372036854775808.0 && value < 9223372036854775808.0)) { return false; }

        return TryFromInt<T>(static_cast<std::int64_t>(value), out);
    }
}

}  // namespace detail

struct PdxJson {
    ValueVariant v;

    // The enum values line up with the order of ValueVariant's alternatives,
    // so type() can be a straight cast of v.index().
    enum class Type { Null = 0, Bool, Int, Double, String, List, Dict };

    // --- Construction (explicit overloads avoid the variant bool/char* trap) ---
    PdxJson(std::nullptr_t = nullptr) : v(nullptr) {}
    PdxJson(bool b)             : v(b) {}
    PdxJson(int i)              : v(static_cast<std::int64_t>(i)) {}
    PdxJson(std::int64_t i)     : v(i) {}
    PdxJson(double d)           : v(d) {}
    PdxJson(const char* s)      : v(std::string(s)) {}
    PdxJson(std::string s)      : v(std::move(s)) {}
    PdxJson(List a)             : v(std::move(a)) {}
    PdxJson(Dict m)             : v(std::move(m)) {}

    // ======================= Type introspection =======================
    Type type() const { return static_cast<Type>(v.index()); }

    const char* typeName() const {
        switch (type()) {
            case Type::Null:   return "null";
            case Type::Bool:   return "bool";
            case Type::Int:    return "int";
            case Type::Double: return "double";
            case Type::String: return "string";
            case Type::List:   return "list";
            case Type::Dict:   return "dict";
        }
        return "unknown";
    }

    bool isNull()   const { return type() == Type::Null; }
    bool isBool()   const { return type() == Type::Bool; }
    bool isInt()    const { return type() == Type::Int; }
    bool isDouble() const { return type() == Type::Double; }
    bool isNumber() const { return isInt() || isDouble(); }
    bool isString() const { return type() == Type::String; }
    bool isList()   const { return type() == Type::List; }
    bool isDict()   const { return type() == Type::Dict; }

    // Asks what is STORED, so only the stored types are meaningful here. To ask
    // whether a value could be read as some other number, use fitsIn<T>().
    template <class T> bool is() const {
        static_assert(detail::isAlternative<T>,
            "PdxJson::is<T>() only tests the stored types; use fitsIn<T>() to test a converted number");
        return std::holds_alternative<T>(v);
    }

    // ======================= Strict typed access =======================
    // as<T>() has two behaviours picked at compile time from T:
    //
    //   * T is one of the stored types (bool, std::int64_t, double, std::string,
    //     List, Dict, std::nullptr_t) -> returns a REFERENCE to the stored value,
    //     throwing std::bad_variant_access if the stored type does not match.
    //     This is the original behaviour and is unchanged.
    //
    //   * T is any other integer or floating point type (std::uint16_t,
    //     std::int32_t, float, ...) -> returns the value BY VALUE, converted from
    //     whichever numeric type is stored, throwing std::runtime_error if the
    //     value is not a number or does not fit in T without being altered.
    //
    // So as<std::int64_t>() still hands back a reference, while as<std::uint16_t>()
    // hands back a checked copy. Bind with `auto` rather than `auto&` when T is a
    // converted type, since there is no stored object to refer to.
    template <class T>
    decltype(auto) as() {
        if constexpr (detail::isAlternative<T>) { return std::get<T>(v); }
        else {
            static_assert(detail::isConvertibleNumber<T>,
                "PdxJson::as<T>(): T must be a stored type (bool, std::int64_t, double, std::string, List, Dict) "
                "or any integer / floating point type");
            return toNumber<T>();
        }
    }
    template <class T>
    decltype(auto) as() const {
        if constexpr (detail::isAlternative<T>) { return std::get<T>(v); }
        else {
            static_assert(detail::isConvertibleNumber<T>,
                "PdxJson::as<T>(): T must be a stored type (bool, std::int64_t, double, std::string, List, Dict) "
                "or any integer / floating point type");
            return toNumber<T>();
        }
    }

    List&       asList()       { return std::get<List>(v); }
    const List& asList() const { return std::get<List>(v); }
    Dict&       asDict()       { return std::get<Dict>(v); }
    const Dict& asDict() const { return std::get<Dict>(v); }

    // Non-throwing pointer access: returns nullptr when the type doesn't match.
    // Only meaningful for the stored types, since a converted number has no
    // address to hand out - use tryNumber() for those.
    template <class T> T* tryAs() {
        static_assert(detail::isAlternative<T>,
            "PdxJson::tryAs<T>() can only point at a stored type; use tryNumber<T>(out) for converted numbers");
        return std::get_if<T>(&v);
    }
    template <class T> const T* tryAs() const {
        static_assert(detail::isAlternative<T>,
            "PdxJson::tryAs<T>() can only point at a stored type; use tryNumber<T>(out) for converted numbers");
        return std::get_if<T>(&v);
    }

    // ================= Coercing scalar getters (never throw) =================
    // Retrieve as a plain C++ type, converting between numeric kinds where it
    // makes sense, and falling back to `def` if the value can't be represented.
    std::int64_t getInt(std::int64_t def = 0) const {
        if (auto p = std::get_if<std::int64_t>(&v)) return *p;
        if (auto p = std::get_if<double>(&v))       return static_cast<std::int64_t>(*p);
        if (auto p = std::get_if<bool>(&v))         return *p ? 1 : 0;
        return def;
    }
    double getDouble(double def = 0.0) const {
        if (auto p = std::get_if<double>(&v))       return *p;
        if (auto p = std::get_if<std::int64_t>(&v)) return static_cast<double>(*p);
        if (auto p = std::get_if<bool>(&v))         return *p ? 1.0 : 0.0;
        return def;
    }
    bool getBool(bool def = false) const {
        if (auto p = std::get_if<bool>(&v))         return *p;
        if (auto p = std::get_if<std::int64_t>(&v)) return *p != 0;
        return def;
    }
    std::string getString(const std::string& def = "") const {
        if (auto p = std::get_if<std::string>(&v)) return *p;
        return def;
    }

    // ================= Generic numeric access =================
    // The dynamic counterparts to getInt / getDouble: these work for ANY integer
    // or floating point type, reading whichever numeric type is actually stored
    // and range checking the conversion.

    // Non-throwing: writes to `out` and returns true only when the stored value
    // is a number that survives the conversion to T unchanged. `out` is left
    // untouched on failure.
    template <class T>
    bool tryNumber(T& out) const {
        static_assert(std::is_arithmetic_v<T>, "PdxJson::tryNumber<T>() requires an integer or floating point type");
        if (auto p = std::get_if<std::int64_t>(&v)) { return detail::TryFromInt<T>(*p, out); }
        if (auto p = std::get_if<double>(&v))       { return detail::TryFromDouble<T>(*p, out); }
        if (auto p = std::get_if<bool>(&v))         { return detail::TryFromInt<T>(*p ? 1 : 0, out); }
        return false;
    }

    // Throwing: the version as<T>() uses for converted numbers. The message names
    // both the stored value and the target type, which beats bad_variant_access.
    template <class T>
    T toNumber() const {
        static_assert(std::is_arithmetic_v<T>, "PdxJson::toNumber<T>() requires an integer or floating point type");
        T out{};
        if (!tryNumber<T>(out)) {
            throw std::runtime_error("PdxJson: cannot read " + toString() + " (" + typeName()
                                     + ") as a " + detail::NumberName<T>());
        }
        return out;
    }

    // Never throws: falls back to `def` when the value is not a number or will
    // not fit. The generic form of getInt(def) / getDouble(def).
    template <class T>
    T getNumber(const T def = T{}) const {
        static_assert(std::is_arithmetic_v<T>, "PdxJson::getNumber<T>() requires an integer or floating point type");
        T out{};
        return tryNumber<T>(out) ? out : def;
    }

    // True when this value is a number that fits in T without being altered.
    // Stricter than isInt() / isNumber(), which say nothing about range.
    template <class T>
    bool fitsIn() const {
        static_assert(std::is_arithmetic_v<T>, "PdxJson::fitsIn<T>() requires an integer or floating point type");
        T out{};
        return tryNumber<T>(out);
    }

    // Convert a stored List into a std::vector<T> of unwrapped values.
    // Non-list values yield an empty vector; elements are read with as<T>(), so T
    // may be a stored type OR any integer / floating point type, and an element
    // that is the wrong type or out of range throws. Use asVectorOr for a lenient
    // version.
    template <class T>
    std::vector<T> asVector() const {
        std::vector<T> out;
        if (auto p = std::get_if<List>(&v)) {
            out.reserve(p->size());
            for (const PdxJson& e : *p) out.push_back(e.as<T>());
        }
        return out;
    }

    // Lenient variant: silently skips elements that aren't a T, or that are
    // numbers which will not fit in T.
    template <class T>
    std::vector<T> asVectorOr() const {
        std::vector<T> out;
        if (auto p = std::get_if<List>(&v)) {
            out.reserve(p->size());
            for (const PdxJson& e : *p) {
                if constexpr (detail::isAlternative<T>) {
                    if (auto q = e.tryAs<T>()) out.push_back(*q);
                }
                else {
                    T value{};
                    if (e.tryNumber<T>(value)) out.push_back(value);
                }
            }
        }
        return out;
    }

    // ======================= Size / emptiness =======================
    // Number of elements for a list/dict; 1 for a scalar; 0 for null.
    std::size_t size() const {
        if (auto p = std::get_if<List>(&v)) return p->size();
        if (auto p = std::get_if<Dict>(&v)) return p->size();
        return isNull() ? 0 : 1;
    }
    bool empty() const { return size() == 0; }

    // ======================= Dict membership / lookup =======================
    bool contains(const Key& k) const {
        auto p = std::get_if<Dict>(&v);
        return p && p->find(k) != p->end();
    }
    bool contains(const std::string& k) const { return contains(Key(k)); }
    bool contains(const char* k)        const { return contains(Key(std::string(k))); }

    // Non-inserting lookup returning a pointer (nullptr if not a dict or key
    // absent). Preferred for read-only access since it never mutates.
    PdxJson* find(const Key& k) {
        if (auto p = std::get_if<Dict>(&v)) { auto it = p->find(k); if (it != p->end()) return &it->second; }
        return nullptr;
    }
    const PdxJson* find(const Key& k) const {
        if (auto p = std::get_if<Dict>(&v)) { auto it = p->find(k); if (it != p->end()) return &it->second; }
        return nullptr;
    }
    PdxJson*       find(const std::string& k)       { return find(Key(k)); }
    const PdxJson* find(const std::string& k) const { return find(Key(k)); }
    PdxJson*       find(const char* k)              { return find(Key(std::string(k))); }
    const PdxJson* find(const char* k)        const { return find(Key(std::string(k))); }

    // Keys of a dict (empty if not a dict).
    std::vector<Key> keys() const {
        std::vector<Key> out;
        if (auto p = std::get_if<Dict>(&v)) {
            out.reserve(p->size());
            for (const auto& [k, _] : *p) out.push_back(k);
        }
        return out;
    }

    // ======================= Mutating dict access =======================
    // operator[] with a key INSERTS a null entry if the key is missing (like
    // std::map / Python d[k] = v). For read-only access prefer find()/at().
    PdxJson& operator[](const Key& k) {
        if (isNull()) v = Dict{};
        return std::get<Dict>(v)[k];
    }
    PdxJson& operator[](const char* k)        { return (*this)[Key(std::string(k))]; }
    PdxJson& operator[](const std::string& k) { return (*this)[Key(k)]; }

    // Const keyed access is READ-ONLY and non-inserting: operator[] on a const
    // object cannot create missing keys, so it throws std::out_of_range if the
    // key is absent. Use find()/contains() to probe optional keys instead.
    const PdxJson& operator[](const Key& k) const {
        const auto& d = std::get<Dict>(v);
        auto it = d.find(k);
        if (it == d.end()) throw std::out_of_range("PdxJson: key not found");
        return it->second;
    }
    const PdxJson& operator[](const char* k)        const { return (*this)[Key(std::string(k))]; }
    const PdxJson& operator[](const std::string& k) const { return (*this)[Key(k)]; }

    // ======================= List / index access =======================
    // Numeric subscript indexes into a List (bounds-checked). Providing both
    // int and size_t overloads means a literal like json[0] resolves here
    // instead of silently becoming a null const char* dict lookup.
    PdxJson&       operator[](int i)               { return at(static_cast<std::size_t>(i)); }
    const PdxJson& operator[](int i)         const { return at(static_cast<std::size_t>(i)); }
    PdxJson&       operator[](std::size_t i)       { return at(i); }
    const PdxJson& operator[](std::size_t i) const { return at(i); }

    // List element by position (throws std::out_of_range / bad_variant_access).
    PdxJson&       at(std::size_t i)       { return std::get<List>(v).at(i); }
    const PdxJson& at(std::size_t i) const { return std::get<List>(v).at(i); }

    // Dict element by string key, NON-inserting: throws std::out_of_range if the
    // key is missing. Distinct from operator[], which would insert.
    PdxJson& at(const std::string& k) {
        auto& d = std::get<Dict>(v);
        auto it = d.find(Key(k));
        if (it == d.end()) throw std::out_of_range("PdxJson: key '" + k + "' not found");
        return it->second;
    }
    const PdxJson& at(const std::string& k) const {
        auto& d = std::get<Dict>(v);
        auto it = d.find(Key(k));
        if (it == d.end()) throw std::out_of_range("PdxJson: key '" + k + "' not found");
        return it->second;
    }

    // Non-throwing list access: nullptr if not a list or index out of range.
    PdxJson* tryAt(std::size_t i) {
        if (auto p = std::get_if<List>(&v)) if (i < p->size()) return &(*p)[i];
        return nullptr;
    }
    const PdxJson* tryAt(std::size_t i) const {
        if (auto p = std::get_if<List>(&v)) if (i < p->size()) return &(*p)[i];
        return nullptr;
    }

    void push_back(PdxJson item) {
        if (isNull()) v = List{};
        std::get<List>(v).push_back(std::move(item));
    }

    // ======================= Comparison =======================
    bool operator==(const PdxJson& o) const { return v == o.v; }
    bool operator!=(const PdxJson& o) const { return v != o.v; }

    // ======================= Pretty printing =======================
    std::string toString() const {
        std::ostringstream os;
        write(os);
        return os.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const PdxJson& j) {
        j.write(os);
        return os;
    }

private:
    static void writeKey(std::ostream& os, const Key& k) {
        std::visit(overloaded{
            [&](std::int64_t i)      { os << i; },
            [&](double d)            { os << d; },
            [&](bool b)              { os << (b ? "True" : "False"); },
            [&](const std::string& s){ os << '"' << s << '"'; },
        }, k);
    }

    void write(std::ostream& os) const {
        std::visit(overloaded{
            [&](std::nullptr_t)       { os << "None"; },
            [&](bool b)               { os << (b ? "True" : "False"); },
            [&](std::int64_t i)       { os << i; },
            [&](double d)             { os << d; },
            [&](const std::string& s) { os << '"' << s << '"'; },
            [&](const List& arr) {
                os << '[';
                for (std::size_t i = 0; i < arr.size(); ++i) {
                    if (i) os << ", ";
                    arr[i].write(os);
                }
                os << ']';
            },
            [&](const Dict& obj) {
                os << '{';
                bool first = true;
                for (const auto& [k, val] : obj) {
                    if (!first) os << ", ";
                    first = false;
                    writeKey(os, k);
                    os << ": ";
                    val.write(os);
                }
                os << '}';
            },
        }, v);
    }
};

}  // namespace PDX
