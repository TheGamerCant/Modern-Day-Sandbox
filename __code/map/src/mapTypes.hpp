#pragma once

#include <string>
#include <cstdint>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cmath>
#include <fstream>
#include <tuple>
#include <algorithm>

struct RGBHash {
    std::size_t operator()(const std::tuple<uint8_t, uint8_t, uint8_t>& color) const {
        auto [r, g, b] = color;
        return (static_cast<std::size_t>(r) << 16) | (static_cast<std::size_t>(g) << 8) | static_cast<std::size_t>(b);
    }
};

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
    class weather_period;

    enum flagTypeEnum {
        flagTypeGlobal = 0,
        flagTypeCountry = 1,
        flagTypeState = 2,
        flagTypeCharacter = 3,
        flagTypeMIO = 4
    };

    class flag {
    public:
        std::string name;
        int value;
        int days;
        uint8_t type;

        flag() :
            name(""), value(0), days(0), type(flagTypeGlobal) {}

        flag(std::string& name, int value, int days, uint8_t type) :
            name(name), value(value), days(days), type(type) {}
    };

    class variable {
    public:
        std::string name;
        std::string value;	
        std::string tooltip;

        variable() :
        name (""), value(""), tooltip("") {}

        variable(std::string& name, std::string& value) :
        name (name), value(value), tooltip("") {}

        variable(std::string& name, std::string& value, std::string& tooltip) :
        name (name), value(value), tooltip(tooltip) {}
    };

    class terrain {
    public:
        bool naval;
        bool is_water;
        uint8_t red, green, blue;
        std::string name;

        terrain() :
            naval(0), is_water(0), red(0), green(0), blue(0), name("") {}

        terrain(bool naval, bool is_water, uint8_t red, uint8_t green, uint8_t blue, std::string& name) :
            naval(naval), is_water(is_water), red(red), green(green), blue(blue), name(name) {}
    };

    class building {
    public:
        std::string name;
        bool provincial;
        uint8_t show_on_map;
        uint8_t max_level;
        bool only_coastal;
        bool is_port;

        building() :
            name(""), provincial(0), show_on_map(0), max_level(0), only_coastal(0), is_port(0) {}
        building(std::string& name, bool provincial, uint8_t show_on_map, uint8_t max_level, bool only_coastal, bool is_port) :
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
        std::string tag, file;
        uint8_t red, green, blue;
        bool dynamic;
        std::vector<state*> states;

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
        uint8_t red, green, blue;
        std::string name;

        state_category() :
            building_slots(0), red(0), green(0), blue(0), name("") {}

        state_category(uint8_t building_slots, uint8_t red, uint8_t green, uint8_t blue, std::string& name) :
            building_slots(building_slots), red(red), green(green), blue(blue), name(name) {}
    };

    enum provinceTypeEnum {
        provinceTypeLand=0,
        provinceTypeLake=1,
        provinceTypeSea=2
    };

    class province {
    public:
        uint16_t id;
        uint8_t red, green, blue, type;
        bool coastal;
        uint8_t continent;
        state* state;
        uint16_t victory_points;
        strategic_region* strategic_region;
        terrain* terrain;
        std::vector<uint8_t> buildings;
        std::string name;

        province() :
            id(0), red(0), green(0), blue(0), type(0), coastal(0), continent(0), state(nullptr), victory_points(0), strategic_region(nullptr), terrain(nullptr), buildings(), name("") {}

        province(uint16_t id, uint8_t red, uint8_t green, uint8_t blue, uint8_t type, bool coastal, uint8_t continent, PDX::terrain* terrain, std::vector<uint8_t>& buildings) :
            id(id), red(red), green(green), blue(blue), type(type), coastal(coastal), continent(continent), state(nullptr), victory_points(0), strategic_region(nullptr), terrain(terrain), buildings(buildings), name("") {}

        province(uint16_t id, uint8_t red, uint8_t green, uint8_t blue, uint8_t type, bool coastal, uint8_t continent, PDX::state* state, uint16_t victory_points, PDX::strategic_region* strategic_region,
            PDX::terrain* terrain, std::vector<uint8_t>& buildings, std::string& name) :
            id(id), red(red), green(green), blue(blue), type(type), coastal(coastal), continent(continent), state(state), victory_points(victory_points), strategic_region(strategic_region), terrain(terrain),
            buildings(buildings), name(name) {}
    };

    class state {
    public:
        uint16_t id;
        uint8_t red, green, blue;
        bool impassable;
        int32_t manpower;
        PDX::country* owner;
        PDX::state_category* state_category;
        std::vector<PDX::province*> provinces;
        std::vector<uint16_t> resources;
        std::vector<std::string> dates;
        std::vector<PDX::country*> cores;
        std::vector<PDX::country*> claims;
        std::vector<PDX::flag> flags;
        std::vector<PDX::variable> variables;
        std::vector<uint8_t> buildings;
        std::string name;

        state() :
            id(0), red(0), green(0), blue(0), impassable(0), manpower(0), owner(nullptr), state_category(nullptr), provinces(), resources(), dates(), cores(), claims(), flags(), variables(), buildings(), name("") {}

        state(uint16_t id, bool impassable, int32_t manpower, country* owner, PDX::state_category* state_category, std::vector<PDX::province*>& provinces, std::vector<uint16_t>& resources,
            std::vector<std::string>& dates, std::vector<PDX::country*>& cores, std::vector<PDX::country*>& claims, std::vector<PDX::flag>& flags, std::vector<PDX::variable>& variables,
            std::vector<uint8_t>& buildings) :
            id(id), red(0), green(0), blue(0), impassable(impassable), manpower(manpower), owner(owner), state_category(state_category), provinces(provinces), resources(resources), dates(dates), cores(cores),
            claims(claims), flags(flags), variables(variables), buildings(buildings), name("") {}

        state(uint16_t id, uint8_t red, uint8_t green, uint8_t blue, bool impassable, int32_t manpower, country* owner, PDX::state_category* state_category, std::vector<PDX::province*>& provinces, std::vector<uint16_t>& resources,
            std::vector<std::string>& dates, std::vector<PDX::country*>& cores, std::vector<PDX::country*>& claims, std::vector<PDX::flag>& flags, std::vector<PDX::variable>& variables,
            std::vector<uint8_t>& buildings, std::string& name) :
            id(id), red(red), green(green), blue(blue), impassable(impassable), manpower(manpower), owner(owner), state_category(state_category), provinces(provinces), resources(resources), dates(dates), cores(cores),
            claims(claims), flags(flags), variables(variables), buildings(buildings), name(name) {}
    };

    class strategic_region {
    public:
        uint16_t id;
        std::string name;
        std::vector<PDX::province*> provinces;
        std::vector<PDX::weather_period> weather;

        strategic_region() :
            id(0), name(""), provinces(), weather() {}

        strategic_region(uint16_t id, std::vector<PDX::province*>& provinces, std::vector<PDX::weather_period>& weather) :
            id(id), name(""), provinces(provinces), weather(weather) {}

        strategic_region(uint16_t id, std::string& name, std::vector<PDX::province*>& provinces, std::vector<PDX::weather_period>& weather) :
            id(id), name(name), provinces(provinces), weather(weather) {}
    };

    class weather_period {
    public:
        double betweenL, betweenR;
        double temperatureL, temperatureR;
        double no_phenomenon;
        double rain_light;
        double rain_heavy;
        double snow;
        double blizzard;
        double artic_water;
        double mud;
        double sandstorm;
        double min_snow_level;

        weather_period () :
            betweenL(0.f), betweenR(0.f), temperatureL(0.f), temperatureR(0.f), no_phenomenon(0.f), rain_light(0.f), rain_heavy(0.f), snow(0.f), blizzard(0.f),
            artic_water(0.f), mud(0.f), sandstorm(0.f), min_snow_level(0.f) {}

        weather_period (double betweenL, double betweenR, double temperatureL, double temperatureR, double no_phenomenon, double rain_light, double rain_heavy,
            double snow, double blizzard, double artic_water, double mud, double sandstorm, double min_snow_level) :
            betweenL(betweenL), betweenR(betweenR), temperatureL(temperatureL), temperatureR(temperatureR), no_phenomenon(no_phenomenon), rain_light(rain_light),
            rain_heavy(rain_heavy), snow(snow), blizzard(blizzard), artic_water(artic_water), mud(mud), sandstorm(sandstorm), min_snow_level(min_snow_level) {}
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

namespace BMP {
    typedef struct {
        uint8_t b;
        uint8_t g;
        uint8_t r;
        uint8_t a;
    }BGRAstruct;

    static uint32_t readFourBytes(char*& data, int& parse) {
        uint32_t t = (uint8_t)data[parse + 3] << 24 | (uint8_t)data[parse + 2] << 16 | (uint8_t)data[parse + 1] << 8 | (uint8_t)data[parse + 0];
        parse += 4;
        return t;
    }

    static uint16_t readTwoBytes(char*& data, int& parse) {
        uint16_t t = (uint8_t)data[parse + 1] << 8 | (uint8_t)data[parse + 0];
        parse += 2;
        return t;
    }

    static uint8_t readOneByte(char*& data, int& parse) {
        uint8_t t = (uint8_t)data[parse + 0];
        ++parse;
        return t;
    }

    static BGRAstruct readBGRA(char*& data, int& parse) {
        BGRAstruct bgra((uint8_t)data[parse + 0], (uint8_t)data[parse + 1], (uint8_t)data[parse + 2], (uint8_t)data[parse + 3]);
        parse += 4;
        return bgra;
    }


    class bitmapImage {
    private:
        uint32_t sizeOfBitmapFile;
        uint32_t reservedBytes;
        uint32_t pixelDataOffset;
        uint32_t headerSize;
        uint32_t imgWidth;
        uint32_t imgHeight;
        uint16_t numberOfColourPlanes;
        uint16_t colourBitDepth;
        uint32_t compression;
        uint32_t imgSize;
        uint32_t pixelsPerMeterWidth;
        uint32_t pixelsPerMeterHeight;
        uint32_t colourTableEntries;
        uint32_t noOfImportantColours;

        std::vector<BGRAstruct> importantColours;
        std::vector<uint8_t> rawData;
        std::vector<uint8_t> rgbData;

    public:

        bitmapImage() :
            sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0), numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0),
            pixelsPerMeterHeight(0), colourTableEntries(0), noOfImportantColours(0), importantColours(), rawData(), rgbData() {}

        bitmapImage(const std::string& fileIn) :
            sizeOfBitmapFile(0), reservedBytes(0), pixelDataOffset(0), headerSize(0), imgWidth(0), imgHeight(0), numberOfColourPlanes(0), colourBitDepth(0), compression(0), imgSize(0), pixelsPerMeterWidth(0),
            pixelsPerMeterHeight(0), colourTableEntries(0), noOfImportantColours(0), importantColours(), rawData(), rgbData() {
            std::ifstream file(fileIn, std::ios::binary | std::ios::ate);
            if (file) {
                size_t fileSize = file.tellg();
                file.seekg(0, std::ios::beg);

                char* data = new char[fileSize];
                file.read(data, fileSize);

                int parse = 2;

                sizeOfBitmapFile = readFourBytes(data, parse);
                reservedBytes = readFourBytes(data, parse);
                pixelDataOffset = readFourBytes(data, parse);
                headerSize = readFourBytes(data, parse);
                imgWidth = readFourBytes(data, parse);
                imgHeight = readFourBytes(data, parse);

                numberOfColourPlanes = readTwoBytes(data, parse);
                colourBitDepth = readTwoBytes(data, parse);

                compression = readFourBytes(data, parse);
                imgSize = readFourBytes(data, parse);
                pixelsPerMeterWidth = readFourBytes(data, parse);
                pixelsPerMeterHeight = readFourBytes(data, parse);

                colourTableEntries = readFourBytes(data, parse);
                noOfImportantColours = readFourBytes(data, parse);

                imgSize = imgWidth * imgHeight * (colourBitDepth / 8);
                colourTableEntries = (pixelDataOffset - 54) / 4;
                noOfImportantColours = colourTableEntries;

                std::vector<char>t;
                rawData.resize(imgSize);
                t.resize(imgSize);

                if (colourTableEntries != 0) {
                    importantColours.resize(colourTableEntries);
                    for (int i = 0; i < colourTableEntries; ++i) {
                        importantColours[i] = readBGRA(data, parse);
                    }

                    fillRGBData();
                }

                file.seekg(parse, std::ios::beg);
               
                file.read(t.data(), imgSize);
                std::transform(t.begin(), t.end(), rawData.begin(),
                    [](char c) { return static_cast<unsigned char>(c); });
                file.close();

                t.clear();
                delete[] data;
            }
        }

        bitmapImage(const bitmapImage& bmpIN) :
            sizeOfBitmapFile(bmpIN.sizeOfBitmapFile), reservedBytes(bmpIN.reservedBytes), pixelDataOffset(bmpIN.pixelDataOffset), headerSize(bmpIN.headerSize), imgWidth(bmpIN.imgWidth),
            imgHeight(bmpIN.imgHeight), numberOfColourPlanes(bmpIN.numberOfColourPlanes), colourBitDepth(bmpIN.colourBitDepth), compression(bmpIN.compression), imgSize(bmpIN.imgSize),
            pixelsPerMeterWidth(bmpIN.pixelsPerMeterWidth),pixelsPerMeterHeight(bmpIN.pixelsPerMeterHeight), colourTableEntries(bmpIN.colourTableEntries),
            noOfImportantColours(bmpIN.noOfImportantColours), importantColours(bmpIN.importantColours), rawData(bmpIN.rawData), rgbData(bmpIN.rgbData) {}

        int GetWidth() const {
            return (int)imgWidth;
        }

        int GetHeight() const {
            return (int)imgHeight;
        }

        std::vector<uint8_t> returnRawData() {
            return rawData;
        }

        void* returnRawDataVoidPtr() {

            if (colourTableEntries == 0) {
                return static_cast<void*>(rawData.data());
            }
            else {
                return static_cast<void*>(rgbData.data());
            }

        }

        void updateRawData(std::vector<uint8_t>& vecIn) {
            rawData.clear();
            rawData = vecIn;
        }

        void fillRGBData() {
            rgbData.clear();

            if (colourTableEntries != 0) {
                long pixelCount = imgWidth * imgHeight;

                rgbData.resize(pixelCount * 3);
                for (long i = 0; i < pixelCount; ++i) {
                    long idx = i * 3;
                    rgbData[idx + 0] = importantColours[rawData[i]].b;
                    rgbData[idx + 1] = importantColours[rawData[i]].g;
                    rgbData[idx + 2] = importantColours[rawData[i]].r;
                }
            }
        }

        void flipImageData() {
            const unsigned int charsPerRow = imgWidth * (colourBitDepth / 8);
            const int iterations = (imgHeight / 2);

            for (int i = 0; i < iterations; ++i) {
                int start = i * charsPerRow;
                int endStart = (imgHeight - 1 - i) * charsPerRow;

                // Swap the two rows in place
                for (unsigned int j = 0; j < charsPerRow; ++j) {
                    std::swap(rawData[start + j], rawData[endStart + j]);
                }
            }

            fillRGBData();
        }

        void swapRBData() {
            if (colourTableEntries == 0) {
                int iterations = colourBitDepth / 8;

                for (unsigned long i = 0; i < imgSize; i += iterations) {
                    uint8_t r = rawData[i + 2];
                    rawData[i + 2] = rawData[i];
                    rawData[i] = r;
                }
            }
            else {
                for (auto& entry : importantColours) {
                    int r = entry.r;
                    entry.r = entry.b;
                    entry.b = r;
                }
                fillRGBData();
            }
        }

        void save(const std::string& outLocation) {
            std::ofstream file(outLocation, std::ios::binary);


            file.write("BM", 2);

            file.write((char*)&sizeOfBitmapFile, sizeof(uint32_t));
            file.write((char*)&reservedBytes, sizeof(uint32_t));
            file.write((char*)&pixelDataOffset, sizeof(uint32_t));
            file.write((char*)&headerSize, sizeof(uint32_t));
            file.write((char*)&imgWidth, sizeof(uint32_t));
            file.write((char*)&imgHeight, sizeof(uint32_t));

            file.write((char*)&numberOfColourPlanes, sizeof(uint16_t));
            file.write((char*)&colourBitDepth, sizeof(uint16_t));
            file.write((char*)&compression, sizeof(uint32_t));
            file.write((char*)&imgSize, sizeof(uint32_t));
            file.write((char*)&pixelsPerMeterWidth, sizeof(uint32_t));
            file.write((char*)&pixelsPerMeterHeight, sizeof(uint32_t));
            file.write((char*)&colourTableEntries, sizeof(uint32_t));
            file.write((char*)&importantColours, sizeof(uint32_t));

            if (colourTableEntries != 0) {
                for (const auto& bgraVal : importantColours) {
                    file.write((char*)&bgraVal.b, sizeof(char));
                    file.write((char*)&bgraVal.g, sizeof(char));
                    file.write((char*)&bgraVal.r, sizeof(char));
                    file.write((char*)&bgraVal.a, sizeof(char));
                }
            }

            for (const auto& entry : rawData) {
                file.write((char*)&entry, sizeof(char));
            }
        }
    };
}