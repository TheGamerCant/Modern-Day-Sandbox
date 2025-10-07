#pragma once
#include <string>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

struct country;
struct terrain;
struct building;
struct resource;
struct state_category;
struct province;
struct state;
struct strategic_region;

struct country {
private:
	uint16_t id;					//Country ID
	uint8_t r, g, b;				//Country colour
	char tag[3];					//Country tag

public:
	country() : id(0), r(0), g(0), b(0), tag("") {};
};

struct terrain {
private:
	uint16_t id;					//Terrain ID
	uint8_t r, g, b;				//Terrain colour
	bool naval_terrain;				//Can be used by navies
	bool is_water;					//Is a water tile
	std::string name;				//Terrain name

public:
	terrain() : id(0), r(0), g(0), b(0), naval_terrain(false), is_water(false), name("") {};
};

struct building {
private:
	uint16_t id;					//Building ID		
	bool shares_slots;				//Does this state building share slots with other buildings
	uint8_t state_max;				//Maximum number of buildings per state
	uint8_t province_max;			//Maximum number of buildings per province, also used to define a state as provincial
	uint8_t show_on_map;			//How many buildings appear per each province / state
	uint8_t show_on_map_meshes;		//How many meshes should appear per show_on_map
	bool centered;					//Should the mesh appear in the center of the province / state
	bool only_costal;				//Can only be made on coastal provinces / states. Is mispelled in the game files lol
	bool is_port;					//Is this building a port
	bool is_in_group;				//Is this building part of a group
	std::string group_by;			//What group is this building a part of
	std::string name;				//Building name

public:
	building() : id(0), shares_slots(false), state_max(255), province_max(255), show_on_map(0), show_on_map_meshes(1), centered(false), only_costal(false), is_port(false),
	is_in_group(false), group_by(""), name("") { };

	//Is this a province-level building
	bool isProvincial() const { return province_max; };
	//Is this a state-level building
	bool isState() const { return !province_max; };
};

struct resource {
private:
	uint16_t id;					//Resource ID
	std::string name;				//Resource name

public:
	resource() : name("") {};
};

struct state_category {
private:
	uint16_t id;					//Category ID	
	uint16_t building_slots;		//No. of building slots
	uint8_t r, g, b;				//State category colour
	std::string name;				//State category name

public:
	state_category() : building_slots(0), r(0), g(0), b(0) {}
};

struct province {
private:
	uint16_t id;														//Province ID
	uint8_t r, g, b;													//Province colour
	bool coastal;														//Coastal or not
	uint8_t continent;													//Province continent
	uint16_t victoryPoints;												//How many VPs this province is worth
	std::shared_ptr<state> state;										//The state we are a part of
	uint16_t;															//Province terrain
	std::vector<uint8_t> buildings;										//Provincial building count by index
	std::string name;													//Province name
};

struct state {
private:
	uint16_t id;														//State ID
	uint8_t red, green, blue;											//State colour
	bool impassable;													//Impassable or not
	int32_t manpower;													//Manpower
	std::shared_ptr<country> owner;										//State owner
	std::shared_ptr<state_category> state_category;						//State category
	std::vector<std::shared_ptr<province>> provinces;					//Provinces in the state
	std::unordered_map<std::shared_ptr<resource>, uint16_t> resources;	//Resources in the state
	std::vector<std::shared_ptr<country>> cores;						//Countries that have this state as a core
	std::vector<std::shared_ptr<country>> claims;						//Countries that have this state as a claim
	std::vector<std::string> data_structures;							//Arguments related to flags, variables, arrays, etc.
	std::unordered_map<std::shared_ptr<building>, uint8_t> buildings;	//State buildings and their count
	std::shared_ptr<strategic_region> strategic_region;					//The strategic region this state is a part of
	std::string name;													//State name
	std::string loc_name;												//Localised name
};

struct strategic_region {
	uint16_t id;									//Strategic region ID
	std::vector<std::shared_ptr<state>> states;		//States that are part of this strategic region
	std::vector<std::string> weather;				//Weather arguments
	std::string name;								//Strategic region name
	std::string loc_name;							//Localised name
};

template<typename T>
struct data_manager {
private:
	std::vector<T> objects;
	std::unordered_map<std::string, std::shared_ptr<T>> lookup_map;

public:


	//Rebuilds the map from objects vector
	void rebuildMap() {
		lookup_map.clear();
		for (auto& obj : objects) {
			lookup_map[obj.name] = std::make_shared<T>(obj);
		}
	}

	//Add new object (also updates vector and map)
	void push_back(const T& obj) {
		objects.push_back(obj);
		lookup_map[obj.name] = std::make_shared<T>(objects.back());
	}
	void emplace_back(const T& obj) {
		objects.emplace_back(obj);
		lookup_map[obj.name] = std::make_shared<T>(objects.back());
	}

	//Reserve data in the vector
	void reserve(const int64_t reserveCount) {
		objects.reserve(reserveCount);
	}

	//Look up by name, return shared pointer
	std::shared_ptr<T> get(const std::string& name) const {
		auto it = lookup_map.find(name);
		return (it != lookup_map.end()) ? it->second : nullptr;
	}
};