#include "write_files.hpp"
#include <fstream>

void WriteStateAndStrategicRegionColours(const Vector<State>& statesArray, const Vector<StrategicRegion>& strategicRegionsArray) {
	std::ofstream stateOutFile("stateColours.raw", std::ios::binary);

	for (const auto& state : statesArray) {
		ColourRGB colour = state.GetColour();
		stateOutFile.write(reinterpret_cast<const char*>(&colour), sizeof(ColourRGB));
	}
	std::ofstream strategicRegionOutFile("strategicRegionColours.raw", std::ios::binary);

	for (const auto& strategicRegion : strategicRegionsArray) {
		ColourRGB colour = strategicRegion.GetColour();
		strategicRegionOutFile.write(reinterpret_cast<const char*>(&colour), sizeof(ColourRGB));
	}
}