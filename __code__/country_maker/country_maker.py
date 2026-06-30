import json
import shutil
import os
from PIL import Image
from typing import Any
from pathlib import Path
from time import perf_counter
import re
import stat

# Assumes you're running it from {mod}/__code__/country_maker/, change this if you're not
MOD_DIRECTORY: Path = Path.cwd()
MOD_DIRECTORY = MOD_DIRECTORY.parents[1]

# Print logs to the terminal (SET TO FALSE IF USING IDLE!!!!!!!)
PRINT_COUNTRY_LOGS: bool = False
PRINT_LOCALISATION_LOGS: bool = False
PRINT_HISTORY_LOGS: bool = False
PRINT_FLAG_LOGS: bool = True

# Create flags after creating the countries?
CREATE_FLAGS: bool = True

# If True, create flags for every ideology, otherwise only create flags for the defined ideologies and default to {tag}.tga
CREATE_FLAGS_FOR_EVERY_IDEOLOGY: bool = False

# If True, appending a flag or localisation value with _leftist, _democratic or _rightist will cause it to be the default 
# for all of the subideologies, will not overwrite any specific ideologies you define though
#
# Valid marco ideologies: 'leftist', 'leftism', 'democratic', 'rightist', 'rightism'
FFTF_MACRO_IDEOLOGIES: bool = True

LOCAL_FLAG_FILES: list[Path] = [f for ext in [".png", ".jpg", ".jpeg", ".tga", ".bmp"] for f in Path(Path.cwd() / "flags").glob(f"**/*{ext}")]
IDEOLOGY_FILES: list[Path] = list(Path(MOD_DIRECTORY / "common/ideologies").glob("**/*.txt"))
COUNTRIES_DIRECTORY: Path = MOD_DIRECTORY / "common/countries"
COUNTRY_TAGS_DIRECTORY: Path = MOD_DIRECTORY / "common/country_tags"
COUNTRY_LOCALISATION_FILE: Path = MOD_DIRECTORY / "localisation/english/countries_l_english.yml"
COSMETIC_TAG_LOCALISATION_FILE: Path = MOD_DIRECTORY / "localisation/english/countries_cosmetic_l_english.yml"
PDX_FLAGS_DIRECTORY: Path = MOD_DIRECTORY / "gfx/flags"
HISTORY_COUNTRIES_DIRECTORY: Path = MOD_DIRECTORY / "history/countries"


FFTF_IDEOLOGIES_MACRO_MICRO: dict[str, list[str]] = {}
FFTF_IDEOLOGIES_MACRO_MICRO_LOCALISATION: dict[str, list[str]] = {}


def format_localisation(localisation: dict[str, str], tag: str, ideologies: list[str]) -> dict[str, str]:
    valid_loc_entries: set[str] = set([
        f'{tag}{ideology}{suffix}' for ideology in [f'_{i}' for i in ideologies] + [''] for suffix in ['', '_ADJ', '_DEF']
    ])
    valid_macro_entries = set([
        f'{tag}{ideology}{suffix}' for ideology in [f'_{i}' for i in FFTF_IDEOLOGIES_MACRO_MICRO.keys()] for suffix in ['', '_ADJ', '_DEF']
    ])

    if FFTF_MACRO_IDEOLOGIES:
        macro_expanded_localisation: dict[str, str] = {}

        macro_localisation: dict[str, str] = { key: value for key, value in localisation.items() if key in valid_macro_entries }

        for key, value in macro_localisation.items():
            for loc_end, loc_replace_list in FFTF_IDEOLOGIES_MACRO_MICRO_LOCALISATION.items():
                if key.endswith(loc_end):

                    for sub_ideology_end in loc_replace_list:
                        new_key: str = key.rsplit(loc_end, 1)[0] + sub_ideology_end
                        macro_expanded_localisation[new_key] = value

                    break

        for key, value in macro_expanded_localisation.items():
            if key not in localisation:
                localisation[key] = value

        localisation = {key: value for key, value in localisation.items() if key not in macro_localisation}

    """
    invalid_localisation: set[str] = {key for key in localisation.keys() if key not in valid_loc_entries}

    if len(invalid_localisation) > 0:
        print(f'The following localisation keys may not be valid, they will be printed as-is regardless: {invalid_localisation}')
    """
        
    return localisation

class Colour:
    def __init__(self, red: int, green: int, blue: int):
        self.red: int = red
        self.green: int = green
        self.blue: int = blue

class Country:
    def __init__(
            self, 
            tag: str, 
            colour: Colour, 
            graphical_culture: str, 
            graphical_culture_2d: str,
            capital: int,
            ideology: str,
            elections_allowed: str,
            localisation: dict[str, str]
        ):
        self.tag: str = tag
        self.colour: Colour = colour
        self.graphical_culture: str = graphical_culture
        self.graphical_culture_2d: str = graphical_culture_2d
        self.capital: int = capital
        self.ideology: str = ideology
        self.elections_allowed: str = elections_allowed
        self.localisation: dict[str, str] = localisation

class DynamicCountry:
    def __init__(
            self, 
            tag: str, 
            colour: Colour
        ):
        self.tag: str = tag
        self.colour: Colour = colour

class CosmeticCountry:
    def __init__(
            self, 
            tag: str, 
            colour: Colour, 
            localisation: dict[str, str]
        ):
        self.tag: str = tag
        self.colour: Colour = colour
        self.localisation: dict[str, str] = localisation

INVALID_TAGS: set[str] = {"NOT", "AND", "OOB", "LOG", "NUM", "RED"}

def is_valid_tag(tag: Any) -> bool:
    if not isinstance(tag, str):
        return False
    
    if tag in INVALID_TAGS:
        return False

    return bool(re.match(r'^[A-Z][A-Z0-9]{2}$', tag))

def is_valid_colour(red: Any, blue: Any, green: Any) -> bool:
    if not isinstance(red, int) or not isinstance(blue, int) or not isinstance(green, int):
        return False

    if 0 <= red <= 255 and 0 <= blue <= 255 and 0 <= green <= 255:
        return True
    
    return False

def load_ideologies() -> list[str]:
    ideologies: list[str] = []

    for file in IDEOLOGY_FILES:
        ideologies.extend(list(get_map_from_brackets(get_map_from_brackets(load_file_to_string(str(file))).get("ideologies")).keys()))

    if len(ideologies) == 0:
        raise Exception("No Ideologies have been defined")

    return ideologies

def is_country_tag_valid(country: dict[str, Any]) -> bool:
    tag: Any = country.get("tag")

    if not is_valid_tag(tag):
        print(f"Invalid tag {tag}, skipping country")
        return False
    
    return True

def is_country_colour_valid(country: dict[str, Any]) -> bool:
    red: Any = country.get("red")
    green: Any = country.get("green")
    blue: Any = country.get("blue")
    
    if not is_valid_colour(red, green, blue):
        print(f"Invalid colour for tag {country.get("tag")}, skipping country.")
        return False
    
    return True

def return_localisation_if_valid(localisation: Any) -> dict[str, str]:
    if not isinstance(localisation, dict):
        return {}
    
    if len(localisation) > 0:
        return {key: value for key, value in localisation.items() if isinstance(key, str) and isinstance(value, str)}

    return {}

def get_yes_no_from_any_default_no(str_in: Any) -> str:
    if not isinstance(str_in, str):
        return "no"
    elif str_in.lower() not in ["yes", "no"]:
        return "no"
    
    return str_in.lower()


def is_valid_country(country: dict[str, Any], base_ideology: str) -> bool:
    if not is_country_tag_valid(country) or not is_country_colour_valid(country):
        return False
    
    tag: Any = country.get("tag")

    graphical_culture: Any = country.get("graphical_culture")
    graphical_culture_2d: Any = country.get("graphical_culture_2d")
    capital: Any = country.get("capital")
    ideology: Any = country.get("ideology")
    elections_allowed: Any = country.get("elections_allowed")
    
    if not isinstance(graphical_culture, str):
        print(f"Invalid graphical culture for tag {tag}, skipping country.")
        return False
    
    if not isinstance(graphical_culture_2d, str):
        print(f"Invalid graphical culture 2d for tag {tag}, skipping country.")
        return False
    
    if not isinstance(capital, int):
        print(f"Invalid capital for tag {tag}, skipping country.")
        return False
    
    if not isinstance(ideology, str):
        print(f"Invalid ideology for tag {tag}, defaulting to {base_ideology}.")

    if not isinstance(elections_allowed, str):
        print(f'Invalid / no entry for "elections_allowed", defaulting to "no".')
    elif elections_allowed.lower() not in ["yes", "no"]:
        print(f'Invalid / no entry for "elections_allowed", defaulting to "no".')

    return True

def remove_readonly(func, path, excinfo):
        os.chmod(path, stat.S_IWRITE)
        func(path) 

def write_country_files(countries: list[Country], cosmetic_countries: list[CosmeticCountry], dynamic_countries: list[DynamicCountry]) -> None:
    shutil.rmtree(COUNTRY_TAGS_DIRECTORY, ignore_errors=True, onexc=remove_readonly)
    COUNTRY_TAGS_DIRECTORY.mkdir(parents=True, exist_ok=True)

    countries_count: int = len(countries)
    cosmentic_countries_count: int = len(cosmetic_countries)
    dynamic_countries_count: int = len(dynamic_countries)
    set_files_count: int = 3 if dynamic_countries_count == 0 else 4
    files_count: int = countries_count + dynamic_countries_count + set_files_count

    if PRINT_COUNTRY_LOGS:
        print_progress_bar(1, files_count, prefix = 'Writing Country Files:', suffix = 'Complete', length = 50)
    with open(str(COUNTRY_TAGS_DIRECTORY / "00_countries.txt"), "w", encoding="utf-8") as f:
        for country in countries:
            f.write(f'{country.tag} = "countries/{country.tag}.txt"\n')

    if len(dynamic_countries) > 0:
        if PRINT_COUNTRY_LOGS:
            print_progress_bar(2, files_count, prefix = 'Writing Country Files:', suffix = 'Complete', length = 50)
        with open(str(COUNTRY_TAGS_DIRECTORY / "zz_dynamic_countries.txt"), "w", encoding="utf-8") as f:
            f.write("dynamic_tags = yes\n\n")
            for dynamic_country in dynamic_countries:
                f.write(f'{dynamic_country.tag} = "countries/{dynamic_country.tag}.txt"\n')

    shutil.rmtree(COUNTRIES_DIRECTORY, ignore_errors=True, onexc=remove_readonly)
    COUNTRIES_DIRECTORY.mkdir(parents=True, exist_ok=True)

    for index, country in enumerate(countries):
        if PRINT_COUNTRY_LOGS:
            print_progress_bar(set_files_count - 1 + index, files_count, prefix = 'Writing Country Files:', suffix = 'Complete', length = 50)
        with open(str(COUNTRIES_DIRECTORY / f"{country.tag}.txt"), "w", encoding="utf-8") as f:
            f.write(
                f'graphical_culture = {country.graphical_culture}\n'
                f'graphical_culture_2d = {country.graphical_culture_2d}\n'
                f'color = rgb {{ {country.colour.red} {country.colour.green} {country.colour.blue} }}'
            )

    for index, dynamic_country in enumerate(dynamic_countries):
        if PRINT_COUNTRY_LOGS:
            print_progress_bar(set_files_count - 1 + index + countries_count, files_count, prefix = 'Writing Country Files:', suffix = 'Complete', length = 50)
        with open(str(COUNTRIES_DIRECTORY / f"{dynamic_country.tag}.txt"), "w", encoding="utf-8") as f:
            f.write(
                f'color = rgb {{ {dynamic_country.colour.red} {dynamic_country.colour.green} {dynamic_country.colour.blue} }}'
            )

    if PRINT_COUNTRY_LOGS:
        print_progress_bar(files_count - 1, files_count, prefix = 'Writing Country Files:', suffix = 'Complete', length = 50)
    with open(str(COUNTRIES_DIRECTORY / f"colors.txt"), "w", encoding="utf-8") as f:
        f.write("#reload countrycolors\n")
        for country in countries:
            f.write(
                f'\n{country.tag} = {{\n'
                f'\tcolor = rgb {{ {country.colour.red} {country.colour.green} {country.colour.blue} }}\n'
                f'\tcolor_ui = rgb {{ {country.colour.red} {country.colour.green} {country.colour.blue} }}\n'
                '}'
            )
    if PRINT_COUNTRY_LOGS:
        print_progress_bar(files_count, files_count, prefix = 'Writing Country Files:', suffix = 'Complete', length = 50)
    with open(str(COUNTRIES_DIRECTORY / f"cosmetic.txt"), "w", encoding="utf-8") as f: 
        for cosmetic_country in cosmetic_countries:
            f.write(
                f'\n{cosmetic_country.tag} = {{\n'
                f'\tcolor = rgb {{ {cosmetic_country.colour.red} {cosmetic_country.colour.green} {cosmetic_country.colour.blue} }}\n'
                f'\tcolor_ui = rgb {{ {cosmetic_country.colour.red} {cosmetic_country.colour.green} {cosmetic_country.colour.blue} }}\n'
                '}'
            )

def write_country_localisation_files(countries: list[Country], cosmetic_countries: list[CosmeticCountry]) -> None:
    if PRINT_LOCALISATION_LOGS:
        print_progress_bar(1, 2, prefix = 'Writing Localisation Files:', suffix = 'Complete', length = 50)
    with open(str(COUNTRY_LOCALISATION_FILE), "w", encoding="utf-8") as f:
        f.write('\ufeffl_english:')
        for country in countries:
            for loc_entry, loc_value in country.localisation.items():
                f.write(f'\n {loc_entry}: "{loc_value}"')

    if PRINT_LOCALISATION_LOGS:
        print_progress_bar(2, 2, prefix = 'Writing Localisation Files:', suffix = 'Complete', length = 50)
    if len(cosmetic_countries) > 0:
        with open(str(COSMETIC_TAG_LOCALISATION_FILE), "w", encoding="utf-8") as f:
            f.write('\ufeffl_english:')
            for cosmetic_country in cosmetic_countries:
                for loc_entry, loc_value in cosmetic_country.localisation.items():
                    f.write(f'\n {loc_entry}: "{loc_value}"')

def write_country_history_files(countries: list[Country]) -> None:
    existing_history_files: set[str] = set([file.stem[:3] for file in list(HISTORY_COUNTRIES_DIRECTORY.glob("**/*.txt")) if len(file.stem) >= 3])

    countries_to_write: set[Country] = [country for country in countries if country.tag not in existing_history_files]

    for index, country in enumerate(countries_to_write):
        if PRINT_HISTORY_LOGS:
            print_progress_bar(index + 1, len(countries_to_write),prefix = 'Writing History Files:', suffix = 'Complete', length = 50)

        history_file_path: Path = (
            HISTORY_COUNTRIES_DIRECTORY / f"{country.tag} - {country.localisation.get(country.tag)}.txt"
            if isinstance(country.localisation, dict) and country.tag in country.localisation
            else HISTORY_COUNTRIES_DIRECTORY / f"{country.tag}.txt"
        )

        with open(str(history_file_path), "w", encoding="utf-8") as f:
            f.write(
                f'capital = {country.capital}\n\n'
                'set_popularities = {\n'
                f'\t{country.ideology} = 100\n'
                '}\n\n'
                'set_politics = {\n'
                f'\truling_party = {country.ideology}\n'
                '\tlast_election = "1936.1.1"\n'
                '\telection_frequency = 48\n'
                f'\telections_allowed = {"yes" if country.elections_allowed else "no" }\n'
                '}'
            )

def write_flags(countries: list[Country], cosmetic_countries: list[CosmeticCountry], ideologies: list[str]) -> None:
    shutil.rmtree(PDX_FLAGS_DIRECTORY, ignore_errors=True, onexc=remove_readonly)
    os.makedirs(PDX_FLAGS_DIRECTORY / "medium")
    os.makedirs(PDX_FLAGS_DIRECTORY / "small")

    tags: list[str] = [country.tag for country in countries]
    cosmetic_tags: list[str] = [country.tag for country in cosmetic_countries]
    cosmetic_tags_set: set[str] = set(cosmetic_tags)
    tags.extend(cosmetic_tags)
    all_possible_flags: set[str] = set([
        f'{tag}{ideology}' for tag in tags for ideology in [f'_{idlgy}' for idlgy in ideologies] + ['']
    ])

    tag_to_flag_file: dict[str, Path] = {p.stem: p for p in LOCAL_FLAG_FILES}

    #Handle macro ideologies
    if FFTF_MACRO_IDEOLOGIES:
        all_possible_macro_flags: set[str] = set([
            f'{tag}_{ideology}' for tag in tags for ideology in FFTF_IDEOLOGIES_MACRO_MICRO.keys()
        ])

        existing_macro_flags: dict[str, Path] = {key: value for key, value in tag_to_flag_file.items() if key in all_possible_macro_flags}

        new_flags: dict[str, Path] = {}

        for key, value in existing_macro_flags.items():
            for flag_end, flag_replace_list in FFTF_IDEOLOGIES_MACRO_MICRO.items():
                if key.endswith(flag_end):

                    for sub_ideology_end in flag_replace_list:
                        new_key: str = key.rsplit(flag_end, 1)[0] + sub_ideology_end
                        new_flags[new_key] = value

                    break

        for key, value in new_flags.items():
            if key not in tag_to_flag_file:
                tag_to_flag_file[key] = value

        tag_to_flag_file = {key: value for key, value in tag_to_flag_file.items() if key not in existing_macro_flags}

    #Make sure we have the correct flags
    for tag in tags:
        if tag in cosmetic_tags_set:
            continue

        if all(f'{tag}_{ideology}' in tag_to_flag_file for ideology in ideologies):
            continue

        if tag not in tag_to_flag_file:
            raise ValueError(f'{tag} does not have a default flag.')
        
        if CREATE_FLAGS_FOR_EVERY_IDEOLOGY:
            for ideology in ideologies:
                if f'{tag}_{ideology}' not in tag_to_flag_file:
                    tag_to_flag_file[f'{tag}_{ideology}'] = tag_to_flag_file.get(tag)

            if tag in tag_to_flag_file:
                tag_to_flag_file.pop(tag)

    tag_to_flag_file = {key: value for key, value in tag_to_flag_file.items() if key in all_possible_flags}

    flag_to_tag_list: dict[str, list[str]] = {}
    for key, value in tag_to_flag_file.items():
        flag_to_tag_list.setdefault(value, []).append(key)

    #Loop over the flag -> tag dictionary
    for index, (flag_path, tags_list) in enumerate(flag_to_tag_list.items()):
        if PRINT_FLAG_LOGS:
            print_progress_bar(index + 1, len(flag_to_tag_list),prefix = 'Writing Flags:', suffix = 'Complete', length = 50)


        base_flag_large: Image.Image = Image.open(str(flag_path)).convert('RGBA')
        base_flag_medium: Image.Image = base_flag_large.resize((41, 26))
        base_flag_small: Image.Image = base_flag_large.resize((10, 7))
        if base_flag_large.width != 82 or base_flag_large.height != 52:
            base_flag_large = base_flag_large.resize((82, 52))

        default_tag: str = tags_list[-1]
        tags_list.pop()
        default_large_path: str = str(PDX_FLAGS_DIRECTORY / f"{default_tag}.tga")
        default_medium_path: str = str(PDX_FLAGS_DIRECTORY / f"medium/{default_tag}.tga")
        default_small_path: str = str(PDX_FLAGS_DIRECTORY / f"small/{default_tag}.tga")

        base_flag_large.save(default_large_path, format='TGA', rle=False)
        base_flag_large.close()
        base_flag_medium.save(default_medium_path, format='TGA', rle=False)
        base_flag_medium.close()
        base_flag_small.save(default_small_path, format='TGA', rle=False)
        base_flag_small.close()

        for tag in tags_list:
            shutil.copyfile(default_large_path, str(PDX_FLAGS_DIRECTORY / f"{tag}.tga"))
            shutil.copyfile(default_medium_path, str(PDX_FLAGS_DIRECTORY / f"medium/{tag}.tga"))
            shutil.copyfile(default_small_path, str(PDX_FLAGS_DIRECTORY / f"small/{tag}.tga"))

def handle_flags(flags: Any):
    if isinstance(flags, dict) and len(flags) > 0:
        if (local_print_country_logs := flags.get("print_country_logs")) is not None and isinstance(local_print_country_logs, bool):
            global PRINT_COUNTRY_LOGS 
            PRINT_COUNTRY_LOGS = local_print_country_logs
        if (local_print_localisation_logs := flags.get("print_localisation_logs")) is not None and isinstance(local_print_localisation_logs, bool):
            global PRINT_LOCALISATION_LOGS
            PRINT_LOCALISATION_LOGS = local_print_localisation_logs
        if (local_print_history_logs := flags.get("print_history_logs")) is not None and isinstance(local_print_history_logs, bool):
            global PRINT_HISTORY_LOGS
            PRINT_HISTORY_LOGS = local_print_history_logs
        if (local_print_flag_logs := flags.get("print_flag_logs")) is not None and isinstance(local_print_flag_logs, bool):
            global PRINT_FLAG_LOGS 
            PRINT_FLAG_LOGS = local_print_flag_logs

        if (local_write_flags := flags.get("write_flags")) is not None and isinstance(local_write_flags, bool):
            global CREATE_FLAGS 
            CREATE_FLAGS = local_write_flags

        if (local_ideology_flags := flags.get("create_flags_for_every_ideology")) is not None and isinstance(local_ideology_flags, bool):
            global CREATE_FLAGS_FOR_EVERY_IDEOLOGY 
            CREATE_FLAGS_FOR_EVERY_IDEOLOGY = local_ideology_flags

        if (local_use_macro_ideologies := flags.get("use_macro_ideologies")) is not None and isinstance(local_use_macro_ideologies, bool):
            global FFTF_MACRO_IDEOLOGIES 
            FFTF_MACRO_IDEOLOGIES = local_use_macro_ideologies

            if local_use_macro_ideologies and (local_macro_ideologies := flags.get("macro_ideologies")) and isinstance(local_macro_ideologies, dict) and len(local_macro_ideologies) > 0:
                if all(isinstance(k, str) and isinstance(v, list) and all(isinstance(i, str) for i in v)for k, v in local_macro_ideologies.items()):

                    global FFTF_IDEOLOGIES_MACRO_MICRO
                    FFTF_IDEOLOGIES_MACRO_MICRO = local_macro_ideologies

                    global FFTF_IDEOLOGIES_MACRO_MICRO_LOCALISATION
                    FFTF_IDEOLOGIES_MACRO_MICRO_LOCALISATION = {
                        f'{key}{suffix}': [f'{v}{suffix}' for v in values]
                        for key, values in FFTF_IDEOLOGIES_MACRO_MICRO.items()
                        for suffix in ('', '_ADJ', '_DEF')
                    }
                else:
                    print('"macro_ideologies" must be of type dict[str, list[str]], defaulting to an empty dict.')

def load_file_to_string(filename: str):
    try:
        with open(filename, "r", encoding="utf-8") as file:
            variables: dict[str, str] = {}
            result: list[str] = []

            for line in file:
                line = line.rstrip("\n")

                in_single_quotes = False
                in_double_quotes = False

                processed_line = []

                # Remove comments outside quotes
                i = 0
                while i < len(line):
                    c = line[i]

                    if (
                        c == "'"
                        and not in_double_quotes
                        and (i == 0 or line[i - 1] != "\\")
                    ):
                        in_single_quotes = not in_single_quotes

                    elif (
                        c == '"'
                        and not in_single_quotes
                        and (i == 0 or line[i - 1] != "\\")
                    ):
                        in_double_quotes = not in_double_quotes

                    if c == "#" and not in_single_quotes and not in_double_quotes:
                        break

                    processed_line.append(c)
                    i += 1

                processed_line = "".join(processed_line).strip()

                # Handle variable definition
                if processed_line.startswith("@"):
                    eq = processed_line.find("=")

                    if eq != -1:
                        var_name = processed_line[1:eq].strip()
                        var_value = processed_line[eq + 1:].strip()

                        variables[var_name] = var_value

                    continue

                # Replace variable references
                for name, value in variables.items():
                    token = "@" + name
                    processed_line = processed_line.replace(token, value)

                result.append(processed_line)

            return "\n".join(result)

    except OSError:
        raise RuntimeError(f"Failed to open file: {filename}")


def get_map_from_brackets(obj_in: str | list[str] | None):
    def parse_blocks(text: str) -> dict[str, list[str]]:
        result: dict[str, list[str]] = {}

        i: int = 0
        n: int = len(text)

        while i < n:
            # Skip whitespace
            while i < n and text[i].isspace():
                i += 1

            # Read key
            key_start = i
            while i < n and text[i] not in "= \t\n":
                i += 1

            key = text[key_start:i].strip()

            # Skip until opening brace
            while i < n and text[i] != '{':
                i += 1

            if i >= n:
                break

            # Parse matching braces
            depth = 0
            value_start = i + 1

            while i < n:
                if text[i] == '{':
                    depth += 1
                elif text[i] == '}':
                    depth -= 1

                    if depth == 0:
                        value = text[value_start:i].strip()

                        if key in result:
                            result[key].append(value)
                        else:
                            result[key] = [value]

                        i += 1
                        break

                i += 1

        return result

    results: dict[str, list[str]] = {}

    if isinstance(obj_in, str):
        results = parse_blocks(obj_in)
    elif isinstance(obj_in, list):
        for entry in obj_in:
            tmp_results = parse_blocks(entry)

            for key, value in tmp_results.items():
                if key in results:
                    results[key].extend(value)
                else:
                    results[key] = value

    return results
    
# Print iterations progress
def print_progress_bar (iteration, total, prefix = '', suffix = '', decimals = 1, length = 100, fill = '█', printEnd = "\r"):
    """
    Call in a loop to create terminal progress bar
    @params:
        iteration   - Required  : current iteration (Int)
        total       - Required  : total iterations (Int)
        prefix      - Optional  : prefix string (Str)
        suffix      - Optional  : suffix string (Str)
        decimals    - Optional  : positive number of decimals in percent complete (Int)
        length      - Optional  : character length of bar (Int)
        fill        - Optional  : bar fill character (Str)
        printEnd    - Optional  : end character (e.g. "\r", "\r\n") (Str)
    """
    percent = ("{0:." + str(decimals) + "f}").format(100 * (iteration / float(total)))
    filledLength = int(length * iteration // total)
    bar = fill * filledLength + '-' * (length - filledLength)
    print(f'\r{prefix} |{bar}| {percent}% {suffix}', end = printEnd)
    # Print New Line on Complete
    if iteration == total: 
        print()

def main():
    time_start: float = perf_counter()

    countries_json: Any = None
    with open("countries.json", "r", encoding="utf-8") as json_file:
        countries_json = json.load(json_file)

    handle_flags(countries_json.get("flags", {}))

    ideologies: list[str] = load_ideologies()

    countries: list[Country] = []
    dynamic_countries: list[DynamicCountry] = []
    cosmetic_countries: list[CosmeticCountry] = []

    for country in countries_json.get("countries"):
        if not is_valid_country(country, ideologies[0]):
            continue

        countries.append(Country(
            tag=country.get("tag"),
            colour=Colour(
                red=country.get("red"),
                green=country.get("green"),
                blue=country.get("blue"),
            ),
            graphical_culture=country.get("graphical_culture"),
            graphical_culture_2d=country.get("graphical_culture_2d"),
            capital=country.get("capital"),
            ideology=country.get("ideology", ideologies[0]),
            elections_allowed=get_yes_no_from_any_default_no(country.get("ideology")),
            localisation=format_localisation(
                return_localisation_if_valid(country.get("localisation", {})),
                country.get("tag"),
                ideologies
            )
        ))

    for dynamic_country in countries_json.get("dynamic_countries"):
        if not is_country_tag_valid(dynamic_country) or not is_country_colour_valid(dynamic_country):
            continue

        dynamic_countries.append(DynamicCountry(
            tag=dynamic_country.get("tag"),
            colour=Colour(
                red=dynamic_country.get("red"),
                green=dynamic_country.get("green"),
                blue=dynamic_country.get("blue"),
            )
        ))
        
    for cosmetic_country in countries_json.get("cosmetic_countries"):
        if not is_country_colour_valid(cosmetic_country):
            continue

        cosmetic_countries.append(CosmeticCountry(
            tag=cosmetic_country.get("tag"),
            colour=Colour(
                red=cosmetic_country.get("red"),
                green=cosmetic_country.get("green"),
                blue=cosmetic_country.get("blue"),
            ),
            localisation=format_localisation(
                return_localisation_if_valid(cosmetic_country.get("localisation", {})),
                cosmetic_country.get("tag"),
                ideologies
            )
        ))
        
    write_country_files(countries, cosmetic_countries, dynamic_countries)
    write_country_localisation_files(countries, cosmetic_countries)
    write_country_history_files(countries)
    
    if CREATE_FLAGS:
        write_flags(countries, cosmetic_countries, ideologies)

    time_taken: float = perf_counter()- time_start
    print(f"Time taken: {time_taken:.3}s")

    

if __name__ == "__main__":
    main()