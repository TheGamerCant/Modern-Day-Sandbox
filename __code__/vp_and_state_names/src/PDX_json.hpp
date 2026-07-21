#pragma once

#include <cstdint>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
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

    template <class T> bool is() const { return std::holds_alternative<T>(v); }

    // ======================= Strict typed access =======================
    // Throw std::bad_variant_access if the stored type does not match.
    template <class T>       T& as()       { return std::get<T>(v); }
    template <class T> const T& as() const { return std::get<T>(v); }

    List&       asList()       { return std::get<List>(v); }
    const List& asList() const { return std::get<List>(v); }
    Dict&       asDict()       { return std::get<Dict>(v); }
    const Dict& asDict() const { return std::get<Dict>(v); }

    // Non-throwing pointer access: returns nullptr when the type doesn't match.
    template <class T>       T* tryAs()       { return std::get_if<T>(&v); }
    template <class T> const T* tryAs() const { return std::get_if<T>(&v); }

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

    // Convert a stored List into a std::vector<T> of unwrapped values.
    // Non-list values yield an empty vector; elements are read with as<T>(),
    // which throws if an element isn't a T. Use asVectorOr for a lenient version.
    template <class T>
    std::vector<T> asVector() const {
        std::vector<T> out;
        if (auto p = std::get_if<List>(&v)) {
            out.reserve(p->size());
            for (const PdxJson& e : *p) out.push_back(e.as<T>());
        }
        return out;
    }

    // Lenient variant: silently skips elements that aren't a T.
    template <class T>
    std::vector<T> asVectorOr() const {
        std::vector<T> out;
        if (auto p = std::get_if<List>(&v)) {
            out.reserve(p->size());
            for (const PdxJson& e : *p)
                if (auto q = e.tryAs<T>()) out.push_back(*q);
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
