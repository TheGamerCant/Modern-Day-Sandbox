import re
from pathlib import Path
from time import perf_counter
from __code__.country_maker.functions import get_map_from_brackets, load_file_to_string

class ColourRgb:
    def __init__(self, r: int, g: int, b: int):
        self.r: int = r
        self.g: int = g
        self.b: int = b

    def __hash__(self) -> int:
        return hash((self.r, self.g, self.b))

class Country:
    def __init__(self, tag: str, file_path: Path):
        self.tag: str = tag
        self.file_path: Path = file_path

class CosmeticCountry:
    def __init__(self, tag: str):
        self.tag: str = tag

def load_ideologies(mod_directory: Path) -> list[str]:
    ideology_files: list[Path] = list(Path(mod_directory / "common/ideologies").glob("**/*.txt"))

    ideologies: list[str] = []

    for file in ideology_files:
        ideologies.extend(get_map_from_brackets(get_map_from_brackets(load_file_to_string(str(file))).get("ideologies")).keys())

    return ideologies

def load_graphical_cultures(mod_directory: Path) -> list[str]:
    return load_file_to_string(str(Path(mod_directory / "common/graphicalculturetype.txt"))).split()

def load_countries(mod_directory: Path, graphical_cultures: list[str]) -> tuple[list[Country], list[CosmeticCountry]]:
    country_files: list[Path] = list(Path(mod_directory / "common/country_tags").glob("**/*.txt"))

    countries: list[Country] = []
    cosmetic: list[CosmeticCountry] = []

    for file in country_files:
        lines: list[str] = load_file_to_string(str(file)).split("\n")

        for line in lines:
            if re.search(r"dynamic_tags\s*=\s*yes", line, re.IGNORECASE):
                break

            match = re.search(r'(\w+)\s*=\s*"(.*)"', line, re.IGNORECASE)
            if match:
                countries.append(Country(
                    match[1],
                    mod_directory / f"common/{match[2]}"
                ))

    cosmetic_tags: Path = mod_directory / "common/countries/cosmetic.txt"
    country_colours: Path = mod_directory / "common/countries/colors.txt"

    for country in countries:
        country_info: str = load_file_to_string(str(country.file_path))

        graphical_culture_match = re.search(r"graphical_culture\s*=\s*(\w+)", country_info, re.IGNORECASE)
        graphical_culture_2d_match = re.search(r"graphical_culture_2d\s*=\s*(\w+)", country_info, re.IGNORECASE)
        colour_match = re.search(r"color\s*=\s*(.*){(.*)}", country_info, re.IGNORECASE)


        colour_type: str = colour_match[1].strip()
        numbers: list[float | int] = [float(n) if '.' in n else int(n) for n in colour_match[2].strip().split()]

        print(f"\n{graphical_culture_match[1]}\n{graphical_culture_2d_match[1]}\n{colour_match[1]}\n{numbers}\n")

    return countries, cosmetic

def main():
    time_start: float = perf_counter()

    mod_directory: Path = Path.cwd()
    mod_directory = mod_directory.parents[1]

    ideologies: list[str] = load_ideologies(mod_directory)
    graphical_cultures: list[str] = load_graphical_cultures(mod_directory)
    countries, cosmetic_tags = load_countries(mod_directory, graphical_cultures)


    load_time: float = perf_counter()- time_start
    print(f"Load Time: {load_time:.3}s")

if __name__ == "__main__":
    main()
