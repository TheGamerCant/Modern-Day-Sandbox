import json
from pathlib import Path

class Culture:
    def __init__(
            self,
            token: str,
            name: str = "",
            desc: str = "",
            red: int = 0,
            green: int = 0,
            blue: int = 0,
            super_culture: str | None = None
        ):
        self.token: str = token
        self.name: str = name
        self.desc: str = desc
        self.red: int = red
        self.green: int = green
        self.blue: int = blue
        self.super_culture: str | None = super_culture

    def __str__(self):
        return (
            f"Token: {self.token}\nName: {self.name}\nRed: {self.red}\n"
            f"Green: {self.green}\nBlue: {self.blue}\nSuper Culture: {self.super_culture}\n"
        )

def RgbToInteger(red: int, green: int, blue: int) -> int:
    return (red * 256 * 256) + (green * 256) + blue

def WriteOnActionsFile(cultures_list: list[Culture], on_actions_file: Path) -> None:
    with open(str(on_actions_file), "w", encoding="utf-8") as f:
        f.write(
            "on_actions = {\n\ton_startup = {\n\t\teffect = {\n\n\t\t\t"
            f"resize_array = {{ array = global.culture_colour_array size = {len(cultures_list)} value = 0 }}\n\t\t\t"
			f"resize_array = {{ array = global.culture_names_array  size = {len(cultures_list)} value = 0 }}\n\t\t\t"
			f"resize_array = {{ array = global.culture_desc_array   size = {len(cultures_list)} value = 0 }}\n\t\t\t"
        )

        for i, culture in enumerate(cultures_list):
            f.write(
                f"set_variable = {{  global.culture_names_array^{i} = token:TDA_culture_{culture.token}_token_idea }}\n\t\t\t"
                f"set_variable = {{   global.culture_desc_array^{i} = token:TDA_culture_{culture.token}_desc_token_idea }}\n\t\t\t"
                f"#{culture.red}, {culture.green}, {culture.blue}\n\t\t\t"
                f"set_variable = {{ global.culture_colour_array^{i} = {RgbToInteger(culture.red, culture.green, culture.blue)} }}\n\n\t\t\t"
            )

        f.write(
            "#Do this after setting up the colours\n\t\t\tZZZ = { update_every_state_culture = yes }"
            "\n\t\t}\n\t}\n}"
        )

def WriteIdeasFile(cultures_list: list[Culture], ideas_file: Path) -> None:
    with open(str(ideas_file), "w", encoding="utf-8") as f:
        f.write(
            "ideas = {\n\tcountry = {"
        )

        for culture in cultures_list:
            f.write(
                f"\n\t\tTDA_culture_{culture.token}_token_idea = {{}}"
                f"\n\t\tTDA_culture_{culture.token}_desc_token_idea = {{}}\n"
            )

        f.write(
            "\t}\n}"
        )

def WriteLocalisationFile(cultures_list: list[Culture], localisation_file: Path) -> None:
    with open(str(localisation_file), "w", encoding="utf-8") as f:
        f.write(
"""l_english:
 MAPMODE_TDA_CULTURE_MAP_MODE:0 "§YCulture§! map mode"
 MAPMODE_TDA_CULTURE_MAP_MODE_NAME:0 "Culture"
 MAPMODE_TDA_CULTURE_MAP_MODE_DESCRIPTION:0 ""

 TDA_culture_map_mode_tooltip:0 "[GetCultureMapModeTT]"
 TDA_culture_map_mode_tooltip_delayed:0 "[GetCultureMapModeTTDelayed]"
 
 TDA_culture_state_is_homogenous:0 "The culture of §Y[FROM.GetName]§! is [?global.culture_names_array^dem1.GetTokenLocalizedKey]."
 TDA_culture_state_is_nearly_homogenous:0 "The culture of §Y[FROM.GetName]§! is almost entirely [?global.culture_names_array^dem1.GetTokenLocalizedKey]."
 TDA_culture_state_has_large_majority_no_minority:0 "The culture of §Y[FROM.GetName]§! is overwhelmingly [?global.culture_names_array^dem1.GetTokenLocalizedKey]."
 TDA_culture_state_has_large_majority_one_minority:0 "The culture of §Y[FROM.GetName]§! is overwhelmingly [?global.culture_names_array^dem1.GetTokenLocalizedKey], with a large [?global.culture_names_array^dem2.GetTokenLocalizedKey] minority."
 TDA_culture_state_has_large_majority_two_minorities:0 "The culture of §Y[FROM.GetName]§! is overwhelmingly [?global.culture_names_array^dem1.GetTokenLocalizedKey], with large [?global.culture_names_array^dem2.GetTokenLocalizedKey] and [?global.culture_names_array^dem3.GetTokenLocalizedKey] minorities."
 TDA_culture_state_has_simple_majority_no_minority:0 "The culture of §Y[FROM.GetName]§! is majority [?global.culture_names_array^dem1.GetTokenLocalizedKey]."
 TDA_culture_state_has_simple_majority_one_minority:0 "The culture of §Y[FROM.GetName]§! is majority [?global.culture_names_array^dem1.GetTokenLocalizedKey], with a large [?global.culture_names_array^dem2.GetTokenLocalizedKey] minority."
 TDA_culture_state_has_simple_majority_two_minorities:0"The culture of §Y[FROM.GetName]§! is majority [?global.culture_names_array^dem1.GetTokenLocalizedKey], with large [?global.culture_names_array^dem2.GetTokenLocalizedKey] and [?global.culture_names_array^dem3.GetTokenLocalizedKey] minorities."
 TDA_culture_state_has_simple_majority_three_minorities:0"The culture of §Y[FROM.GetName]§! is majority [?global.culture_names_array^dem1.GetTokenLocalizedKey], with large [?global.culture_names_array^dem2.GetTokenLocalizedKey], [?global.culture_names_array^dem3.GetTokenLocalizedKey] and [?global.culture_names_array^dem4.GetTokenLocalizedKey] minorities."
 TDA_culture_state_has_plurality_no_minority:0 "The culture of §Y[FROM.GetName]§! is plurality [?global.culture_names_array^dem1.GetTokenLocalizedKey]."
 TDA_culture_state_has_plurality_one_minority:0 "The culture of §Y[FROM.GetName]§! is plurality [?global.culture_names_array^dem1.GetTokenLocalizedKey], with a large [?global.culture_names_array^dem2.GetTokenLocalizedKey] minority."
 TDA_culture_state_has_plurality_two_minorities:0"The culture of §Y[FROM.GetName]§! is plurality [?global.culture_names_array^dem1.GetTokenLocalizedKey], with large [?global.culture_names_array^dem2.GetTokenLocalizedKey] and [?global.culture_names_array^dem3.GetTokenLocalizedKey] minorities."
 TDA_culture_state_has_plurality_three_minorities:0"The culture of §Y[FROM.GetName]§! is plurality [?global.culture_names_array^dem1.GetTokenLocalizedKey], with large [?global.culture_names_array^dem2.GetTokenLocalizedKey], [?global.culture_names_array^dem3.GetTokenLocalizedKey] and [?global.culture_names_array^dem4.GetTokenLocalizedKey] minorities."
 TDA_culture_state_mostly_other:0 "The culture of §Y[FROM.GetName]§! has no clear cultural majority or plurality."
 
 TDA_culture_map_mode_1_row:0 "[?global.culture_names_array^loc_row_1.GetTokenLocalizedKey] - [?value_row_1|2%%]"
 TDA_culture_map_mode_2_row:0 "[?global.culture_names_array^loc_row_1.GetTokenLocalizedKey] - [?value_row_1|2%%]\\n[?global.culture_names_array^loc_row_2.GetTokenLocalizedKey] - [?value_row_2|2%%]"
 TDA_culture_map_mode_3_row:0 "[?global.culture_names_array^loc_row_1.GetTokenLocalizedKey] - [?value_row_1|2%%]\\n[?global.culture_names_array^loc_row_2.GetTokenLocalizedKey] - [?value_row_2|2%%]\\n[?global.culture_names_array^loc_row_3.GetTokenLocalizedKey] - [?value_row_3|2%%]"
 TDA_culture_map_mode_4_row:0 "[?global.culture_names_array^loc_row_1.GetTokenLocalizedKey] - [?value_row_1|2%%]\\n[?global.culture_names_array^loc_row_2.GetTokenLocalizedKey] - [?value_row_2|2%%]\\n[?global.culture_names_array^loc_row_3.GetTokenLocalizedKey] - [?value_row_3|2%%]\\n[?global.culture_names_array^loc_row_4.GetTokenLocalizedKey] - [?value_row_4|2%%]"
 TDA_culture_map_mode_5_row:0 "[?global.culture_names_array^loc_row_1.GetTokenLocalizedKey] - [?value_row_1|2%%]\\n[?global.culture_names_array^loc_row_2.GetTokenLocalizedKey] - [?value_row_2|2%%]\\n[?global.culture_names_array^loc_row_3.GetTokenLocalizedKey] - [?value_row_3|2%%]\\n[?global.culture_names_array^loc_row_4.GetTokenLocalizedKey] - [?value_row_4|2%%]\\n[?global.culture_names_array^loc_row_5.GetTokenLocalizedKey] - [?value_row_5|2%%]"
 TDA_culture_map_mode_6_row:0 "[?global.culture_names_array^loc_row_1.GetTokenLocalizedKey] - [?value_row_1|2%%]\\n[?global.culture_names_array^loc_row_2.GetTokenLocalizedKey] - [?value_row_2|2%%]\\n[?global.culture_names_array^loc_row_3.GetTokenLocalizedKey] - [?value_row_3|2%%]\\n[?global.culture_names_array^loc_row_4.GetTokenLocalizedKey] - [?value_row_4|2%%]\\n[?global.culture_names_array^loc_row_5.GetTokenLocalizedKey] - [?value_row_5|2%%]\\n[?global.culture_names_array^loc_row_6.GetTokenLocalizedKey] - [?value_row_6|2%%]"
 TDA_culture_map_mode_7_row:0 "[?global.culture_names_array^loc_row_1.GetTokenLocalizedKey] - [?value_row_1|2%%]\\n[?global.culture_names_array^loc_row_2.GetTokenLocalizedKey] - [?value_row_2|2%%]\\n[?global.culture_names_array^loc_row_3.GetTokenLocalizedKey] - [?value_row_3|2%%]\\n[?global.culture_names_array^loc_row_4.GetTokenLocalizedKey] - [?value_row_4|2%%]\\n[?global.culture_names_array^loc_row_5.GetTokenLocalizedKey] - [?value_row_5|2%%]\\n[?global.culture_names_array^loc_row_6.GetTokenLocalizedKey] - [?value_row_6|2%%]\\n[?global.culture_names_array^loc_row_7.GetTokenLocalizedKey] - [?value_row_7|2%%]\""""
)
        for culture in cultures_list:
            f.write(
                f"\n\n TDA_culture_{culture.token}_token_idea:0 \"{culture.name}\""
                f"\n TDA_culture_{culture.token}_desc_token_idea:0 \"{culture.desc}\""
            )

def LoadCulturesFromJson() -> list[Culture]:
    culture_list: list[Culture] = []

    with open("cultures.json", "r", encoding="utf-8") as json_file:
        json_data = json.load(json_file)

        for culture in json_data.get("cultures"):
            culture_list.append(Culture(
                token = culture.get("token"),
                red = culture.get("red", 0),
                green = culture.get("green", 0),
                blue = culture.get("blue", 0),
                name = culture.get("name", ""),
                desc = culture.get("desc", ""),
                super_culture = culture.get("super_culture", None)
            ))

    return culture_list

def main():
    mod_directory: Path = Path.cwd()
    mod_directory = mod_directory.parents[1]

    on_actions_file: Path = mod_directory / "common/on_actions/TDA_culture_on_actions.txt"
    idea_tokens_file: Path = mod_directory / "common/ideas/TDA_culture_tokens.txt"
    localisation_file: Path = mod_directory / "localisation/english/TDA_culture_l_english.yml"
    states_folder: Path = mod_directory / "history/states"

    cultures_list: list[Culture] = LoadCulturesFromJson()

    WriteOnActionsFile(cultures_list, on_actions_file)
    WriteIdeasFile(cultures_list, idea_tokens_file)
    WriteLocalisationFile(cultures_list, localisation_file)


if __name__ == "__main__":
    main()