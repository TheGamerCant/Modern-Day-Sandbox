#include "write_files.hpp"
#include <fstream>

void WriteStateAndStrategicRegionColours(const Vector<State>& statesArray, const Vector<StrategicRegion>& strategicRegionsArray) {
	std::ofstream stateOutFile("in\\stateColours.raw", std::ios::binary);
	for (const auto& state : statesArray) {
		ColourRGB colour = state.GetColour();
		stateOutFile.write(reinterpret_cast<const char*>(&colour), sizeof(ColourRGB));
	}
	stateOutFile.close();


	std::ofstream strategicRegionOutFile("in\\strategicRegionColours.raw", std::ios::binary);
	for (const auto& strategicRegion : strategicRegionsArray) {
		ColourRGB colour = strategicRegion.GetColour();
		strategicRegionOutFile.write(reinterpret_cast<const char*>(&colour), sizeof(ColourRGB));
	}
	strategicRegionOutFile.close();
}

void WriteProvinceDefinitions(const Vector<Province>& provincesArray, const VectorMap<Terrain>& landTerrainsArray,
	const VectorMap<Terrain>& seaTerrainsArray, const VectorMap<Terrain>& lakeTerrainsArray) {

	std::ofstream provinceDefinitionsOutFile("out\\map\\definitions.csv", std::ios::binary);

	ColourRGB provColour(0, 0, 0);
	const String provTypes[3] = { "land", "sea", "lake" };

	for (const auto& province : provincesArray) {
		provColour = province.GetColour();
		provinceDefinitionsOutFile << province.GetId() << ";" << static_cast<UnsignedInteger16>(provColour.r) << ";" << static_cast<UnsignedInteger16>(provColour.g) <<
			";" << static_cast<UnsignedInteger16>(provColour.b) << ";" << provTypes[province.GetProvinceType()] << (province.GetCoastal() ? ";true;" : ";false;") <<
			province.GetContinent() << "\n";
	}

	provinceDefinitionsOutFile.close();
}

void WriteNames(const Vector<Province>& provincesArray, const Vector<State>& statesArray) {
	std::ofstream scriptedEffectsOutFile("out\\common\\scripted_effects\\TDA_name_changes_scripted_effects.txt", std::ios::binary);

	UnsignedInteger16 stateId{};
	String stateIdString{};
	String provinceIdString{};
	String stateNameChangesString{};
	String changeAllCityNamesString{};

	for (const auto& state : statesArray) {
		stateId = state.GetId();
		stateIdString = std::to_string(stateId);

		if (stateId == 0) { continue; }

		stateNameChangesString = "#" + state.GetDefaultName() + "\nTDA_update_state_" + stateIdString + "_names = {\n";

		if (state.GetChangeableNameCount() != 0) {
			String prefix = "";
			stateNameChangesString += "\t" + stateIdString + " = {\n";

			SizeT i = 0;
			for (const auto& nameChange : state.GetNameEntries()) {
				stateNameChangesString += "\t\t#" + nameChange.name + "\n\t\t" + prefix + "if = {\n\t\t\tlimit = { CONTROLLER = {";
				for (const auto& requirement : nameChange.nameRequirements) {
					stateNameChangesString += " " + requirement;
				}
				stateNameChangesString += " } }\n\t\t\tset_state_name = " + state.GetName() + "_" + std::to_string(i) + "\n\t\t}\n";

				prefix = "else_";
				++i;
			}

			stateNameChangesString += "\t\telse = {\n\t\t\treset_state_name = yes\n\t\t}\n\t}\n";
		}

		for (const auto& provId : state.GetProvinces()) {
			const Province& prov = provincesArray[provId];

			if (prov.GetChangeableNameCount() == 0) { continue; }

			provinceIdString = std::to_string(provId);
			String prefix = "";

			SizeT i = 0;
			for (const auto& nameChange : prov.GetNameEntries()) {
				stateNameChangesString += "\t#" + nameChange.name + "\n\t" + prefix + "if = {\n\t\tlimit = { any_country = { controls_province = " +
					provinceIdString;
				for (const auto& requirement : nameChange.nameRequirements) {
					stateNameChangesString += " " + requirement;
				}
				stateNameChangesString += " } }\n\t\tset_province_name = { id = " + provinceIdString + " name = VICTORY_POINTS_" + provinceIdString +
					"_" + std::to_string(i) + " }\n\t}\n";

				prefix = "else_";
				++i;
			}

			stateNameChangesString += "\t#" + prov.GetDefaultName() + "\n\telse = {\n\t\treset_province_name = " + provinceIdString +"\n\t}\n";
		}

		stateNameChangesString += "}\n\n";

		scriptedEffectsOutFile << stateNameChangesString;

		changeAllCityNamesString += "\tTDA_update_state_" + stateIdString + "_names = yes\n";
	}


	scriptedEffectsOutFile << "\n\nTDA_change_all_city_names = {\n" << changeAllCityNamesString + 
		"}\n\nTDA_toggle_change_city_names = {\n\tif = {\n\t\tlimit = { has_global_flag = TDA_city_name_changes_active_flag }\
		\n\t\tclr_global_flag = TDA_city_name_changes_active_flag\n\t}\n\telse = {\n\t\tset_global_flag = TDA_city_name_changes_active_flag\n\t}\
		\n\tTDA_change_all_city_names = yes\n}";
	scriptedEffectsOutFile.close();

	stateNameChangesString = "";
	changeAllCityNamesString = "";

	std::ofstream stateNamesYmlOutFile("out\\localisation\\english\\state_names_l_english.yml", std::ios::binary);
	std::ofstream victoryPointNamesYmlOutFile("out\\localisation\\english\\victory_points_l_english.yml", std::ios::binary);

	const UnsignedChar bom_l_english[13] = { 
		0xEF, 0xBB, 0xBF, 0x6C, 0x5F, 0x65, 0x6E, 0x67, 0x6C, 0x69, 0x73, 0x68, 0x3A 
	};
	stateNamesYmlOutFile.write(reinterpret_cast<const Char*>(bom_l_english), 13);

	const UnsignedChar victory_points_tooltip[73] = {
		0x0A, 0x20, 0x56, 0x49, 0x43, 0x54, 0x4F, 0x52, 0x59, 0x5F, 0x50, 0x4F, 0x49, 0x4E, 0x54, 0x53, 0x5F, 0x54, 0x4F, 0x4F, 
		0x4C, 0x54, 0x49, 0x50, 0x3A, 0x30, 0x20, 0x22, 0xC2, 0xA7, 0x47, 0x24, 0x4E, 0x41, 0x4D, 0x45, 0x24, 0xC2, 0xA7, 0x21,
		0x20, 0x76, 0x69, 0x63, 0x74, 0x6F, 0x72, 0x79, 0x20, 0x70, 0x6F, 0x69, 0x6E, 0x74, 0x73, 0x20, 0x3D, 0x20, 0xC2, 0xA7,
		0x59, 0x24, 0x50, 0x4F, 0x49, 0x4E, 0x54, 0x53, 0x24, 0xC2, 0xA7, 0x21, 0x22 };
	victoryPointNamesYmlOutFile.write(reinterpret_cast<const Char*>(bom_l_english), 13);
	victoryPointNamesYmlOutFile.write(reinterpret_cast<const Char*>(victory_points_tooltip), 73);

	String ymlOutString = "";
	UnsignedInteger16 id = 0;
	String idString = "";
	SizeT customNameIndex = 0;

	for (const auto& state : statesArray) {
		id = state.GetId();
		if (id == 0) { continue; }


		ymlOutString += "\n " + state.GetName() + ": \"" + state.GetDefaultName() + "\"";
		customNameIndex = 0;
		for (const auto& nameChange : state.GetNameEntries()) {
			ymlOutString += "\n " + state.GetName() + "_" + std::to_string(customNameIndex) + ": \"" + nameChange.name + "\"";
			++customNameIndex;
		}
	}

	stateNamesYmlOutFile << ymlOutString;
	stateNamesYmlOutFile.close();

	ymlOutString = "";
	for (const auto& province : provincesArray) {
		if (province.GetDefaultName() != "") {
			id = province.GetId();
			idString = std::to_string(id);


			ymlOutString += "\n VICTORY_POINTS_" + idString + ": \"" + province.GetDefaultName() + "\"";
			customNameIndex = 0;
			for (const auto& nameChange : province.GetNameEntries()) {
				ymlOutString += "\n VICTORY_POINTS_" + idString + "_" + std::to_string(customNameIndex) + ": \"" + nameChange.name + "\"";
				++customNameIndex;
			}
		}
	}


	victoryPointNamesYmlOutFile << ymlOutString;
	victoryPointNamesYmlOutFile.close();
}