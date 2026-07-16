#pragma once

#include <cstdint>
#include <map>
#include <sstream>
#include <string>
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

struct PdxJson {
    ValueVariant v;

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

    // --- Type queries ---
    bool isNull()   const { return std::holds_alternative<std::nullptr_t>(v); }
    bool isBool()   const { return std::holds_alternative<bool>(v); }
    bool isInt()    const { return std::holds_alternative<std::int64_t>(v); }
    bool isDouble() const { return std::holds_alternative<double>(v); }
    bool isString() const { return std::holds_alternative<std::string>(v); }
    bool isList()   const { return std::holds_alternative<List>(v); }
    bool isDict()   const { return std::holds_alternative<Dict>(v); }

    template <class T> bool is() const { return std::holds_alternative<T>(v); }

    // --- Typed access (throws std::bad_variant_access on type mismatch) ---
    template <class T>       T& as()       { return std::get<T>(v); }
    template <class T> const T& as() const { return std::get<T>(v); }

    List&       asList()       { return std::get<List>(v); }
    const List& asList() const { return std::get<List>(v); }
    Dict&       asDict()       { return std::get<Dict>(v); }
    const Dict& asDict() const { return std::get<Dict>(v); }

    // --- Dict access. Auto-promotes an empty PdxJson into a dict on first use,
    //     mirroring Python's d[k] = v assignment. ---
    PdxJson& operator[](const Key& k) {
        if (isNull()) v = Dict{};
        return std::get<Dict>(v)[k];
    }
    PdxJson& operator[](const char* k)     { return (*this)[Key(std::string(k))]; }
    PdxJson& operator[](const std::string& k) { return (*this)[Key(k)]; }

    // --- List access ---
    PdxJson&       at(std::size_t i)       { return std::get<List>(v).at(i); }
    const PdxJson& at(std::size_t i) const { return std::get<List>(v).at(i); }

    void push_back(PdxJson item) {
        if (isNull()) v = List{};
        std::get<List>(v).push_back(std::move(item));
    }

    // --- Pretty printing (Python-ish repr) ---
    std::string toString() const {
        std::ostringstream os;
        write(os);
        return os.str();
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

}