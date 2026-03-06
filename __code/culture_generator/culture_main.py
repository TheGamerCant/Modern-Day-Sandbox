import json
import math
from pathlib import Path
from typing import Any
import re

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
        self.old_index: int | None = None
        self.new_index: int | None = None

    def __str__(self):
        return (
            f"Token: {self.token}\nName: {self.name}\nRed: {self.red}\n"
            f"Green: {self.green}\nBlue: {self.blue}\nSuper Culture: {self.super_culture}\n"
            f"Old Index: {self.old_index}\nNew Index: {self.new_index}\n"
        )

class SuperCulture:
    def __init__(
            self,
            token: str,
            name: str = "",
            desc: str = "",
            red: int = 0,
            green: int = 0,
            blue: int = 0,
            subcultures: list[str] | None = None
        ):
        self.token: str = token
        self.name: str = name
        self.desc: str = desc
        self.red: int = red
        self.green: int = green
        self.blue: int = blue
        self.subcultures: list[str] | None = subcultures
        self.subcultures_count: int = len(subcultures) if subcultures else 0
        self.index : int | None = None

def RgbToInteger(red: int, green: int, blue: int) -> int:
    return (red * 256 * 256) + (green * 256) + blue

def WriteOnActionsFile(
        cultures_list: list[Culture],
        super_cultures_list: list[SuperCulture], 
        on_actions_file: Path, 
        bucket_size: int, 
        largest_bucket: int, 
        max_culture_index: int, 
        max_super_culture_index: int
    ) -> None:
    with open(str(on_actions_file), "w", encoding="utf-8") as f:
        f.write(
            "on_actions = {\n\ton_startup = {\n\t\teffect = {\n\t\t\t"
            f"set_variable = {{ global.culture_bucket_size = {bucket_size} }}\n\t\t\t"
            f"set_variable = {{ global.culture_largest_bucket = {largest_bucket} }}\n\n\t\t\t"
            f"resize_array = {{ array = global.culture_colour_array size = {max_culture_index + 1} value = 0 }}\n\t\t\t"
			f"resize_array = {{ array = global.culture_names_array  size = {max_culture_index + 1} value = 0 }}\n\t\t\t"
			f"resize_array = {{ array = global.culture_desc_array   size = {max_culture_index + 1} value = 0 }}\n\n\t\t\t"
        )

        for culture in cultures_list:
            f.write(
                f"set_variable = {{  global.culture_names_array^{culture.new_index} = token:TDA_culture_{culture.token}_token_idea }}\n\t\t\t"
                f"set_variable = {{   global.culture_desc_array^{culture.new_index} = token:TDA_culture_{culture.token}_desc_token_idea }}\n\t\t\t"
                f"#{culture.red}, {culture.green}, {culture.blue}\n\t\t\t"
                f"set_variable = {{ global.culture_colour_array^{culture.new_index} = {RgbToInteger(culture.red, culture.green, culture.blue)} }}\n\n\t\t\t"
            )

        f.write(
            f"resize_array = {{ array = global.super_culture_colour_array size = {max_super_culture_index + 1} value = 0 }}\n\t\t\t"
			f"resize_array = {{ array = global.super_culture_names_array  size = {max_super_culture_index + 1} value = 0 }}\n\t\t\t"
			f"resize_array = {{ array = global.super_culture_desc_array   size = {max_super_culture_index + 1} value = 0 }}\n\n\t\t\t"
        )

        for super_culture in super_cultures_list:
            f.write(
                f"set_variable = {{  global.super_culture_names_array^{super_culture.index} = token:TDA_culture_{super_culture.token}_token_idea }}\n\t\t\t"
                f"set_variable = {{   global.super_culture_desc_array^{super_culture.index} = token:TDA_culture_{super_culture.token}_desc_token_idea }}\n\t\t\t"
                f"#{super_culture.red}, {super_culture.green}, {super_culture.blue}\n\t\t\t"
                f"set_variable = {{ global.super_culture_colour_array^{super_culture.index} = {RgbToInteger(super_culture.red, super_culture.green, super_culture.blue)} }}\n\n\t\t\t"
            )

        f.write(
            "#Do this after setting up the colours\n\t\t\tZZZ = { update_every_state_culture = yes }"
            "\n\t\t}\n\t}\n}"
        )

def WriteIdeasFile(cultures_list: list[Culture], super_cultures_list: list[SuperCulture], ideas_file: Path) -> None:
    with open(str(ideas_file), "w", encoding="utf-8") as f:
        f.write(
            "ideas = {\n\tcountry = {"
        )

        for culture in cultures_list:
            f.write(
                f"\n\t\tTDA_culture_{culture.token}_token_idea = {{}}"
                f"\n\t\tTDA_culture_{culture.token}_desc_token_idea = {{}}\n"
            )

        for super_culture in super_cultures_list:
            f.write(
                f"\n\t\tTDA_super_culture_{super_culture.token}_token_idea = {{}}"
                f"\n\t\tTDA_super_culture_{super_culture.token}_desc_token_idea = {{}}\n"
            )

        f.write(
            "\t}\n}"
        )

def WriteLocalisationFile(cultures_list: list[Culture], super_cultures_list: list[SuperCulture], localisation_file: Path) -> None:
    with open(str(localisation_file), "w", encoding="utf-8") as f:
        f.write('\ufeff')
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

        for super_culture in super_cultures_list:
            f.write(
                f"\n\n TDA_super_culture_{super_culture.token}_token_idea:0 \"{super_culture.name}\""
                f"\n TDA_super_culture_{super_culture.token}_desc_token_idea:0 \"{super_culture.desc}\""
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

def LoadCultureIndexes(on_actions_file: Path, cultures_list: list[Culture]):
    with open(str(on_actions_file), "r", encoding="utf-8") as f:
        culture_indexes: list[tuple[Any]] = re.findall(r"set_variable = {  global.culture_names_array\^(\d+) = token:TDA_culture_(\w+)_token_idea }", f.read())
        
        culture_to_index_dict: dict[str, int] = {culture : int(index)for (index, culture) in culture_indexes}
        
        for culture in cultures_list:
            culture.old_index = culture_to_index_dict.get(culture.token)

def LoadSuperCulturesFromJson(cultures_list: list[Culture]) -> list[SuperCulture]:
    super_culture_list: list[SuperCulture] = []

    super_to_sub_dict: dict[str, list[str]] = {}
    for culture in cultures_list:
        if culture.super_culture and culture.super_culture != "":
            if culture.super_culture in super_to_sub_dict:
                super_to_sub_dict[culture.super_culture].append(culture.token)
            else:
                super_to_sub_dict[culture.super_culture] = [culture.token]

    with open("cultures.json", "r", encoding="utf-8") as json_file:
        json_data = json.load(json_file)

        for super_culture in json_data.get("super_cultures"):
            super_culture_list.append(SuperCulture(
                token = super_culture.get("token"),
                red = super_culture.get("red", 0),
                green = super_culture.get("green", 0),
                blue = super_culture.get("blue", 0),
                name = super_culture.get("name", ""),
                desc = super_culture.get("desc", ""),
                subcultures=super_to_sub_dict.get(super_culture.get("token"), None)
            ))

    return super_culture_list

def GetBucketSize(super_cultures_list: list[SuperCulture], non_super_cultures_count: int) -> list[Any]:
    bucket_size: int = 2
    return_cultures_index: list[list[str]] = []
    total_size = math.inf
    return_largest_bucket: int = 0
    
    for i in range(2, super_cultures_list[-1].subcultures_count + 1):
        cultures_indexed: list[list[str]] = [
            [] for i in range(math.ceil(super_cultures_list[-1].subcultures_count / i))
        ]
        for super_culture in super_cultures_list:
            cultures_indexed[math.ceil(super_culture.subcultures_count / i) - 1].append(super_culture.token)

        largest_bucket: int = max((len(sublist) for sublist in cultures_indexed), default=0)
        culture_array_size: int = sum([i * j * largest_bucket for j in range(1, len(cultures_indexed))]) + (len(cultures_indexed[-1]) * i * len(cultures_indexed))
        super_culture_array_size: int = int(culture_array_size / i)

        empty_space: int = 0
        for j in range(len(cultures_indexed) - 1):
            empty_space += (largest_bucket - len(cultures_indexed[j])) * i * (j + 1)

        this_array_size: int = culture_array_size + super_culture_array_size

        if empty_space < non_super_cultures_count:
            this_array_size += non_super_cultures_count - empty_space

        if this_array_size <= total_size:
            total_size = this_array_size
            bucket_size = i
            return_cultures_index = cultures_indexed
            return_largest_bucket = largest_bucket
            
    
    return [bucket_size, return_cultures_index, return_largest_bucket]

def ComputeNewIndexes(
        cultures_list: list[Culture], 
        super_cultures_list: list[SuperCulture],
        bucket_size: int, 
        largest_bucket: int,
        cultures_indexed: list[list[str]]
    ) -> list[int]:
    culture_token_to_list_index_dict: dict[str, int] = {culture.token : i for i, culture in enumerate(cultures_list)}
    super_culture_token_to_list_index_dict: dict[str, int] = {super_culture.token : i for i, super_culture in enumerate(super_cultures_list)}
    non_super_cultures_list: list[Culture] = [culture for culture in cultures_list if not culture.super_culture and culture.token != "other"]

    cultures_list[culture_token_to_list_index_dict.get("other")].new_index = 0
    culture_token_to_list_index_dict.pop("other")

    current_culture_index: int = 1
    current_super_culture_index: int = 0
    max_culture_index: int = 1
    max_super_culture_index: int = 0

    cultures_indexed.append([])
    for i in range(len(cultures_indexed)):
        for j in range(largest_bucket):
            if j < len(cultures_indexed[i]):
                super_culture_index: int = super_culture_token_to_list_index_dict.get(cultures_indexed[i][j])

                super_cultures_list[super_culture_index].index = current_super_culture_index
                max_super_culture_index = current_super_culture_index

                subculture_tokens: list[str] = super_cultures_list[super_culture_index].subcultures 
                for k, subculture in enumerate(subculture_tokens):
                    culture_index: int = culture_token_to_list_index_dict.get(subculture)
                    cultures_list[culture_index].new_index = current_culture_index + k
                    max_culture_index = current_culture_index + k

            elif len(non_super_cultures_list) > 0:
                for k in range(bucket_size * (i + 1)):
                    if len(non_super_cultures_list) <= 0:
                        break

                    culture_index: int = culture_token_to_list_index_dict.get(non_super_cultures_list[0].token)
                    cultures_list[culture_index].new_index = current_culture_index + k
                    max_culture_index = current_culture_index + k
                    non_super_cultures_list.pop(0)

            current_culture_index += bucket_size * (i + 1)
            current_super_culture_index += 1

    return [max_culture_index, max_super_culture_index]

def UpdateStateFiles(cultures_list: list[Culture], states_folder: Path) -> None:
    state_files: list[Path] = list(states_folder.glob("**/*.txt"))
    old_to_new_index_dict: dict[int, int] = {culture.old_index : culture.new_index for culture in cultures_list}

    for file in state_files:
        with open(str(file)) as f:
            text = f.read()

        text: str = re.sub(
            r"set_variable = { culture_index_array\^(\d+) = (\d+) }",
            lambda m: f"set_variable = {{ culture_index_array^{m.group(1)} = {old_to_new_index_dict.get(int(m.group(2)))} }}",
            text
        )

        with open(str(file), "w") as f:
            f.write(text)

def main():
    mod_directory: Path = Path.cwd()
    mod_directory = mod_directory.parents[1]

    on_actions_file: Path = mod_directory / "common/on_actions/TDA_culture_on_actions.txt"
    idea_tokens_file: Path = mod_directory / "common/ideas/TDA_culture_tokens.txt"
    localisation_file: Path = mod_directory / "localisation/english/TDA_culture_l_english.yml"
    states_folder: Path = mod_directory / "history/states"

    cultures_list: list[Culture] = LoadCulturesFromJson()
    super_cultures_list: list[SuperCulture] = LoadSuperCulturesFromJson(cultures_list)
    LoadCultureIndexes(on_actions_file, cultures_list)

    super_cultures_list.sort(key=lambda x : x.subcultures_count)
    non_super_cultures_count: int = len(cultures_list) - sum([culture.subcultures_count for culture in super_cultures_list])
    bucket_size, cultures_indexed, largest_bucket = GetBucketSize(super_cultures_list, non_super_cultures_count)

    max_culture_index, max_super_culture_index = ComputeNewIndexes(cultures_list, super_cultures_list, bucket_size, largest_bucket, cultures_indexed)

    cultures_list.sort(key=lambda x : x.new_index)

    WriteOnActionsFile(cultures_list, super_cultures_list, on_actions_file, bucket_size, largest_bucket, max_culture_index, max_super_culture_index)
    WriteIdeasFile(cultures_list, super_cultures_list, idea_tokens_file)
    WriteLocalisationFile(cultures_list, super_cultures_list, localisation_file)

    UpdateStateFiles(cultures_list, states_folder)


if __name__ == "__main__":
    main()