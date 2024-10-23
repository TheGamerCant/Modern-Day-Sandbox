#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cmath>

namespace PDX {
    class terrain;
    class building;
    class resource;
    class country;
    class variable;
    class state_category;
    class flag;
    class province;
    class state;
    class strategic_region;

    class terrain {
    public:
        bool naval;
        bool is_water;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        std::string name;

        terrain():
            naval(0), is_water(0), red(0), green(0), blue(0), name("") {}

        terrain(bool naval, bool is_water, uint8_t red, uint8_t green, uint8_t blue, std::string& name):
            naval(naval), is_water(is_water), red (red), green(green), blue(blue), name(name){}
    };

    class building {
    public:
        std::string name;
        bool provincial;
        uint8_t show_on_map;
        uint8_t max_level;
        bool only_coastal;
        bool is_port;

        building():
            name(""), provincial(0), show_on_map(0), max_level(0), only_coastal(0), is_port(0) {}
        building(std::string& name, bool provincial, uint8_t show_on_map, uint8_t max_level, bool only_coastal, bool is_port):
            name(name), provincial(provincial), show_on_map(0), max_level(show_on_map), only_coastal(only_coastal), is_port(is_port) {}
    };

    class resource {
    public:
        std::string name;

        resource() :
            name("") {}
        resource(std::string& name) :
            name(name) {}
    };

    class country {
    public:
        std::string tag;
        std::string file;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        bool dynamic;
        std::vector<state *> states;

        country() :
        tag("ZZZ"), red(0), green(0), blue(0), dynamic(0) {}

        country(std::string& tag, uint8_t red, uint8_t green, uint8_t blue, bool dynamic) :
        tag(tag), red(red), green(green), blue(blue), dynamic(dynamic) {}

        country(std::string& tag, uint8_t red, uint8_t green, uint8_t blue, bool dynamic, std::vector<state*> states) :
        tag(tag), red(red), green(green), blue(blue), dynamic(dynamic), states(states) {}

        void HSVToRGB(double H, double S, double V) {
            double C = V * S;
            double X = C * (1 - fabs(fmod(H * 6, 2) - 1));
            double m = V - C;

            double rPrime, gPrime, bPrime;

            if (0 <= H && H < 1.0f / 6) {
                rPrime = C;
                gPrime = X;
                bPrime = 0;
            }
            else if (1.0f / 6 <= H && H < 2.0f / 6) {
                rPrime = X;
                gPrime = C;
                bPrime = 0;
            }
            else if (2.0f / 6 <= H && H < 3.0f / 6) {
                rPrime = 0;
                gPrime = C;
                bPrime = X;
            }
            else if (3.0f / 6 <= H && H < 4.0f / 6) {
                rPrime = 0;
                gPrime = X;
                bPrime = C;
            }
            else if (4.0f / 6 <= H && H < 5.0f / 6) {
                rPrime = X;
                gPrime = 0;
                bPrime = C;
            }
            else {
                rPrime = C;
                gPrime = 0;
                bPrime = X;
            }

            // Convert the normalized [0,1] RGB to [0,255] integer RGB values
            red = static_cast<int>((rPrime + m) * 255);
            green = static_cast<int>((gPrime + m) * 255);
            blue = static_cast<int>((bPrime + m) * 255);
        }
    };

    class state_category {
    public:
        uint8_t building_slots;
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        std::string name;

        state_category() :
            building_slots(0), red(0), green(0), blue(0), name(""){}

        state_category(uint8_t building_slots, uint8_t red, uint8_t green, uint8_t blue, std::string& name) :
            building_slots(building_slots), red(red), green(green), blue(blue), name(name) {}
    };

    class state {
    public:
        uint16_t id;

        state() :
        id(0) {}

        state(int id) :
        id(id) {}
    };






    // Tag or Name?
    template <typename T>
    concept HasTag = requires(T t) {
        { t.tag } -> std::convertible_to<std::string>;
    };

    template <typename T>
    concept HasName = requires(T t) {
        { t.name } -> std::convertible_to<std::string>;
    };

    template<typename Ty>
    class vectorStringIndexMap {
    public:
        std::vector<Ty> vector;
        std::unordered_map<std::string, unsigned int> hashMap;

        vectorStringIndexMap() {}

        void mapFromVector() requires HasTag<Ty> {
            hashMap.clear();
            for (size_t i = 0; i < vector.size(); ++i) {
                hashMap[vector[i].tag] = i;
            }
        }

        void mapFromVector() requires HasName<Ty> {
            hashMap.clear();
            for (size_t i = 0; i < vector.size(); ++i) {
                hashMap[vector[i].name] = i;
            }
        }
    };
}