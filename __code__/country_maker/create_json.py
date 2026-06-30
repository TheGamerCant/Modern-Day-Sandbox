import re
import json
from typing import Any
from pathlib import Path


MOD_DIRECTORY: Path = Path.cwd()
MOD_DIRECTORY = MOD_DIRECTORY.parents[1]

COUNTRY_FILES: list[Path] = list(Path(MOD_DIRECTORY / "common/countries").glob("**/*.txt"))
COUNTRY_FILES = [path for path in COUNTRY_FILES if path.stem not in ["cosmetic", "colors"]]
HISTORY_FILES: list[Path] = list(Path(MOD_DIRECTORY / "history/countries").glob("**/*.txt"))
LOCALISATION_FILE: Path = MOD_DIRECTORY / "localisation/english/countries_l_english.yml"

INVALID_TAGS: set[str] = {"NOT", "AND", "OOB", "LOG", "NUM", "RED"}

def is_valid_tag(tag: Any) -> bool:
    if not isinstance(tag, str):
        return False

    if tag in INVALID_TAGS:
        return False

    return bool(re.match(r'^[A-Z][A-Z0-9]{2}$', tag))

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

def load_countries() -> dict[str, dict[str, Any]]:
    countries_dict: dict[str, dict[str, Any]] = {}

    for file in COUNTRY_FILES:
        tag = file.stem[:3]
        country_data = load_file_to_string(str(file))

        graphical_culture = re.search(r"graphical_culture\s*=\s*(\w+)", country_data)
        graphical_culture_2d = re.search(r"graphical_culture_2d\s*=\s*(\w+)", country_data)
        colour = re.search(r"color\s*=\s*rgb\s*{\s*(\d+)\s+(\d+)\s+(\d+)\s*}", country_data)

        countries_dict[tag] = {
            "tag": tag,
            "red": int(colour.group(1)),
            "green": int(colour.group(1)),
            "blue": int(colour.group(1)),
            "graphical_culture": graphical_culture.group(1),
            "graphical_culture_2d": graphical_culture_2d.group(1)
        }

    return countries_dict

def load_country_history(countries_dict: dict[str, dict[str, Any]]):
    for file in HISTORY_FILES:
        tag = file.stem[:3]
        history_data = load_file_to_string(str(file))

        capital = re.search(r"capital\s*=\s*(\d+)", history_data)
        ideology = re.search(r"ruling_party\s*=\s*(\w+)", history_data)

        countries_dict[tag]["capital"] = int(capital.group(1))
        countries_dict[tag]["ideology"] = ideology.group(1)

        countries_dict[tag]["localisation"] = {}

def load_localisation(countries_dict: dict[str, dict[str, Any]]):
    with open(LOCALISATION_FILE, "r") as file:
        for line in file.readlines():
            loc_key = re.findall(r'\s*(\w+):\d* \"([^"]+)\"', line)

            if len(loc_key) != 1:
                continue

            tag = loc_key[0][0][:3]

            countries_dict[tag]["localisation"][loc_key[0][0]] = loc_key[0][1]

def main():
    countries_dict = load_countries()
    load_country_history(countries_dict)
    load_localisation(countries_dict)

    json_dict = {"countries": []}
    for tag, country_data in countries_dict.items():
        json_dict["countries"].append(country_data)

    with open("countries.json", "w", encoding="utf-8") as json_file:
        json_file.write(json.dumps(json_dict, indent="\t", ensure_ascii=False))



if __name__ == "__main__":
    main()