import os
import re
import shutil
from pathlib import Path
import colorsys
from PIL import Image
from time import perf_counter
from __code__.country_maker.functions import get_map_from_brackets, load_file_to_string, clamp
from typing import Literal

MOD_DIRECTORY: Path = Path.cwd()
MOD_DIRECTORY = MOD_DIRECTORY.parents[1]

LOCAL_FLAG_FILES: list[Path] = list(Path(Path.cwd() / "flags").glob("**/*.png"))

IDEOLOGY_FILES: list[Path] = list(Path(MOD_DIRECTORY / "common/ideologies").glob("**/*.txt"))
COUNTRY_FILES: list[Path] = list(Path(MOD_DIRECTORY / "common/country_tags").glob("**/*.txt"))
GRAPHICAL_CULTURES_FILE: Path = Path(MOD_DIRECTORY / "common/graphicalculturetype.txt")
COUNTRY_COLOURS_FILE: Path = MOD_DIRECTORY / "common/countries/colors.txt"
COSMETIC_TAGS_FILE: Path = MOD_DIRECTORY / "common/countries/cosmetic.txt"
COUNTRY_LOCALISATION: Path = MOD_DIRECTORY / "localisation/english/countries_l_english.yml"
COSMETIC_TAG_LOCALISATION: Path = MOD_DIRECTORY / "localisation/english/countries_cosmetic_l_english.yml"
PDX_FLAGS_FOLDER: Path = MOD_DIRECTORY / "gfx/flags"

class Colour:
    def __init__(self, colour_type: Literal["rgb", "hsv"], value_1: float, value_2: float, value_3: float):
        self.__r: float = 0.0
        self.__g: float = 0.0
        self.__b: float = 0.0

        self.__h: float = 0.0
        self.__s: float = 0.0
        self.__v: float = 0.0

        if colour_type == "rgb":
            self.__r = value_1
            self.__g = value_2
            self.__b = value_3

            self.__update_hsv()

        elif colour_type == "hsv":
            self.__h = value_1
            self.__s = value_2
            self.__v = value_3

            self.__update_rgb()


    def __update_rgb(self) -> None:
        self.__r, self.__g, self.__b = colorsys.hsv_to_rgb(self.__h, self.__s, self.__v)
    def __update_hsv(self) -> None:
        self.__h, self.__s, self.__v = colorsys.rgb_to_hsv(self.__r, self.__g, self.__b)

    def get_colours(self, return_as: Literal["float", "uint8"] = "float", *args) -> tuple[float, ...] | tuple[int, ...]:
        return_colours: list[float] = []

        for arg in args:
            match arg:
                case "r":
                    return_colours.append(self.__r)
                case "g":
                    return_colours.append(self.__g)
                case "b":
                    return_colours.append(self.__b)
                case "h":
                    return_colours.append(self.__h)
                case "s":
                    return_colours.append(self.__s)
                case "v":
                    return_colours.append(self.__v)
                case _:
                    pass

        if return_as == "uint8":
            return_colours_int: list[int] = [round(clr * 255.0) for clr in return_colours]
            return tuple(return_colours_int)

        return tuple(return_colours)

    def set_colours(
            self,
            red: float | None = None,
            green: float | None = None,
            blue: float | None = None,
            hue: float | None = None,
            saturation: float | None = None,
            value: float | None = None
    ) -> None:
        any_rgb: bool = any([red, green, blue])
        any_hsv: bool = any([hue, saturation, value])

        if any_rgb and any_hsv:
            raise ValueError("Cannot modify both RGB and HSV values, must be XOR")
        elif not any_rgb and not any_hsv:
            return None

        if any_rgb:
            if red:
                self.__r = clamp(red, 0.0, 1.0)

            if green:
                self.__g = clamp(green, 0.0, 1.0)

            if blue:
                self.__b = clamp(blue, 0.0, 1.0)

            self.__update_hsv()

        else:
            if hue:
                self.__h = clamp(hue, 0.0, 1.0)

            if saturation:
                self.__s = clamp(saturation, 0.0, 1.0)

            if value:
                self.__v = clamp(value, 0.0, 1.0)

            self.__update_rgb()

        return None

class Country:
    def __init__(self, tag: str, file_path: Path):
        self.__tag: str = tag
        self.__file_path: Path = file_path

        self.__graphical_culture: str | None = None
        self.__graphical_culture_2d: str | None = None
        self.__colour: Colour | None = None
        self.__colour_ui: Colour | None = None

    def load_country_file_data(self, graphical_culture: str, graphical_culture_2d: str, colour: Colour):
        self.__graphical_culture = graphical_culture
        self.__graphical_culture_2d = graphical_culture_2d
        self.__colour = colour

    def load_country_colours_file_data(self, colour: Colour, colour_ui: Colour):
        self.__colour = colour
        self.__colour_ui = colour_ui

    def get_file_path(self) -> Path:
        return self.__file_path


class CosmeticCountry:
    def __init__(self, tag: str, colour: Colour, colour_ui: Colour):
        self.__tag: str = tag
        self.__colour: Colour = colour
        self.__colour_ui: Colour = colour_ui


def load_ideologies() -> list[str]:
    ideologies: list[str] = []

    for file in IDEOLOGY_FILES:
        ideologies.extend(get_map_from_brackets(get_map_from_brackets(load_file_to_string(str(file))).get("ideologies")).keys())

    if len(ideologies) == 0:
        raise Exception("No Ideologies have been defined")

    return ideologies


def load_graphical_cultures() -> list[str]:
    graphical_cultures: list[str] =  load_file_to_string(str(GRAPHICAL_CULTURES_FILE)).split()

    if len(graphical_cultures) == 0:
        raise Exception("No Graphical Cultures have been defined")

    return graphical_cultures


def get_colour_from_regex(regex_pattern: str, string: str, tag: str, file_path: Path) -> Colour:
    colour_match: list[str] = re.findall(regex_pattern, string, re.IGNORECASE)
    colour = Colour("rgb", 0.5, 0.5, 0.5)

    if len(colour_match) == 0:
        r, g, b = colour.get_colours("uint8", "r", "g", "b")
        print(f"Country {tag} has no color defined in {file_path}, defaulting to rgb ({r}, {g}, {b})")
    else:
        colour_type: str = colour_match[-1][0].strip().lower()
        numbers: list[float] = [float(n) for n in colour_match[-1][1].strip().split()]

        if colour_type not in ["rgb", "hsv", ""]:
            raise ValueError(f"Country colour for {tag} must be of type 'rgb' or 'hsv'")

        if len(numbers) != 3:
            raise ValueError(f"{tag} must have 3 colour values defined, {len(numbers)} were defined instead")

        if colour_type == "hsv":
            if any([(n > 1.0 or n < 0.0) for n in numbers]):
                raise ValueError("Bad HSV values, must be between 0.0 and 1.0")

            colour = Colour("hsv", numbers[0], numbers[1], numbers[2])

        elif colour_type == "rgb" or colour_type == "":
            if any([n > 1.0 for n in numbers]):
                numbers = [n / 255 for n in numbers]

            if any([(n > 1.0 or n < 0.0) for n in numbers]):
                raise ValueError("Bad RGB values, must be between 0.0 and 1.0 or 0 and 255")

            colour = Colour("rgb", numbers[0], numbers[1], numbers[2])

        if len(colour_match) > 1:
            print(f"Country {tag} has multiple colours defined in {file_path}, will choose the last one defined ({colour_match[-1][1]})")

    return colour


def load_countries(graphical_cultures: list[str]) -> tuple[dict[str, Country], dict[str, CosmeticCountry]]:
    countries: dict[str, Country] = {}
    cosmetic_tags: dict[str, CosmeticCountry] = {}

    for file in COUNTRY_FILES:
        lines: list[str] = load_file_to_string(str(file)).split("\n")

        for line in lines:
            if re.search(r"dynamic_tags\s*=\s*yes", line, re.IGNORECASE):
                break

            match = re.search(r'(\w+)\s*=\s*"(.*)"', line, re.IGNORECASE)
            if match:
                countries[match[1]] = Country(
                    match[1],
                    MOD_DIRECTORY / f"common/{match[2]}"
                )

    if len(countries) == 0:
        raise Exception("No Countries have been defined")

    for tag, country in countries.items():
        country_info: str = load_file_to_string(str(country.get_file_path()))

        graphical_culture_match: list[str] = re.findall(r"graphical_culture\s*=\s*(\w+)", country_info, re.IGNORECASE)
        graphical_culture_2d_match: list[str] = re.findall(r"graphical_culture_2d\s*=\s*(\w+)", country_info, re.IGNORECASE)
        colour_match: list[str] = re.findall(r"color\s*=\s*(.*){(.*)}", country_info, re.IGNORECASE)

        graphical_culture: str = next((s for s in graphical_cultures if s.endswith("_gfx")), graphical_cultures[0])
        graphical_culture_2d: str = next((s for s in graphical_cultures if s.endswith("_2d")), graphical_cultures[0])

        colour: Colour = get_colour_from_regex(r"color\s*=\s*(.*){(.*)}", country_info, tag, country.get_file_path())


        if len(graphical_culture_match) == 0:
            print(f"Country {tag} has no graphical_culture defined in {country.get_file_path()}, defaulting to {graphical_culture}")
        elif graphical_culture_match[-1] not in graphical_cultures:
            print(f"Country {tag} has an invalid graphical_culture defined in {country.get_file_path()} ({graphical_culture_match[-1]}), defaulting to {graphical_culture}")
        else:
            graphical_culture = graphical_culture_match[-1]
            if len(graphical_culture_match) > 1:
                print(f"Country {tag} has multiple graphical_cultures defined in {country.get_file_path()}, will choose the last one defined ({graphical_culture})")

        if len(graphical_culture_2d_match) == 0:
            print(f"Country {tag} has no graphical_culture_2d defined in {country.get_file_path()}, defaulting to {graphical_culture_2d}")
        elif graphical_culture_2d_match[-1] not in graphical_cultures:
            print(f"Country {tag} has an invalid graphical_culture_2d defined in {country.get_file_path()} ({graphical_culture_2d_match[-1]}), defaulting to {graphical_culture}")
        else:
            graphical_culture_2d = graphical_culture_2d_match[-1]
            if len(graphical_culture_2d_match) > 1:
                print(f"Country {tag} has multiple graphical_culture_2ds defined in {country.get_file_path()}, will choose the last one defined ({graphical_culture_2d})")

        country.load_country_file_data(
            graphical_culture=graphical_culture,
            graphical_culture_2d=graphical_culture_2d,
            colour=colour
        )

    country_tags_and_colours: dict[str, str] = get_map_from_brackets(load_file_to_string(str(COUNTRY_COLOURS_FILE)))
    for tag, colour_data in country_tags_and_colours.items():
        if tag not in countries:
            continue

        colour: Colour = get_colour_from_regex(r"color\s*=\s*(.*){(.*)}", colour_data[0], tag, countries.get(tag).get_file_path() if tag in countries else "")
        colour_ui: Colour = get_colour_from_regex(r"color_ui\s*=\s*(.*){(.*)}", colour_data[0], tag, countries.get(tag).get_file_path() if tag in countries else "")

        countries[tag].load_country_colours_file_data(colour, colour_ui)

    cosmetic_country_tags_and_colours: dict[str, str] = get_map_from_brackets(load_file_to_string(str(COSMETIC_TAGS_FILE)))
    for tag, colour_data in cosmetic_country_tags_and_colours.items():
        colour: Colour = get_colour_from_regex(r"color\s*=\s*(.*){(.*)}", colour_data[0], tag, countries.get(tag).get_file_path() if tag in countries else "")
        colour_ui: Colour = get_colour_from_regex(r"color_ui\s*=\s*(.*){(.*)}", colour_data[0], tag, countries.get(tag).get_file_path() if tag in countries else "")

        cosmetic_tags[tag] = CosmeticCountry(tag, colour, colour_ui)

    return countries, cosmetic_tags

def write_flags(countries: dict[str, Country], cosmetic_countries: dict[str, CosmeticCountry], ideologies: list[str]) -> None:
    shutil.rmtree(PDX_FLAGS_FOLDER, ignore_errors=True)
    os.makedirs(PDX_FLAGS_FOLDER / "medium")
    os.makedirs(PDX_FLAGS_FOLDER / "small")

    tags: list[str] = [tag for tag in countries]
    tags.extend([tag for tag in cosmetic_countries])

    tag_to_flag_file: dict[str, Path] = {p.stem: p for p in LOCAL_FLAG_FILES}

    #Loop over every cosmetic and country tag
    for tag in tags:
        tag_ideologies: list[str] = [f"{tag}_{ideology}" for ideology in ideologies]
        ideology_flags_made: set[str] = set()

        for tag_w_ideology in tag_ideologies:
            #If TAG_IDEOLOGY.png exists, copy that one
            if tag_w_ideology in tag_to_flag_file:
                tag_ideology_flag_root_path: str = str(tag_to_flag_file.get(tag_w_ideology))

                flag_large: Image.Image = Image.open(tag_ideology_flag_root_path).convert('RGBA')

                flag_medium: Image.Image = flag_large.resize((41, 26))
                flag_medium.save(str(PDX_FLAGS_FOLDER / f"medium/{tag_w_ideology}.tga"), format='TGA', rle=False)
                flag_small: Image.Image = flag_large.resize((10, 7))
                flag_small.save(str(PDX_FLAGS_FOLDER / f"small/{tag_w_ideology}.tga"), format='TGA', rle=False)

                if flag_large.width != 82 or flag_large.height != 52:
                    flag_large = flag_large.resize((82, 52))
                    flag_large.save(str(PDX_FLAGS_FOLDER / f"{tag_w_ideology}.tga"), format='TGA', rle=False)
                else:
                    shutil.copyfile(tag_ideology_flag_root_path, str(PDX_FLAGS_FOLDER / f"{tag_w_ideology}.tga"))

                ideology_flags_made.add(tag_w_ideology)

        #If every ideology has a unique flag, there's no need to copy the base one
        if tag_ideologies == ideology_flags_made:
            continue

        #We can ignore cosmetic country flags that don't exist but not regular tags
        if tag not in tag_to_flag_file and tag in cosmetic_countries:
            continue

        if tag not in tag_to_flag_file:
            raise ValueError(f"{tag}.png does not exist")

        #Create 1 ideology flag as our base and copy it for the rest
        tag_ideologies_remaining: list[str] = [i for i in tag_ideologies if i not in ideology_flags_made]
        default_ideology: str = tag_ideologies_remaining[-1]
        tag_ideologies_remaining.pop()

        tag_ideology_flag_root_path: str = str(tag_to_flag_file.get(tag))

        flag_large: Image.Image = Image.open(tag_ideology_flag_root_path).convert('RGBA')

        flag_medium: Image.Image = flag_large.resize((41, 26))
        flag_medium.save(str(PDX_FLAGS_FOLDER / f"medium/{default_ideology}.tga"), format='TGA', rle=False)
        flag_small: Image.Image = flag_large.resize((10, 7))
        flag_small.save(str(PDX_FLAGS_FOLDER / f"small/{default_ideology}.tga"), format='TGA', rle=False)
        if flag_large.width != 82 or flag_large.height != 52:
            flag_large = flag_large.resize((82, 52))
            flag_large.save(str(PDX_FLAGS_FOLDER / f"{default_ideology}.tga"), format='TGA', rle=False)
        else:
            shutil.copyfile(tag_ideology_flag_root_path, str(PDX_FLAGS_FOLDER / f"{default_ideology}.tga"))

        for tag_ideology in tag_ideologies_remaining:
            shutil.copyfile(str(PDX_FLAGS_FOLDER / f"{default_ideology}.tga"), str(PDX_FLAGS_FOLDER / f"{tag_ideology}.tga"))
            shutil.copyfile(str(PDX_FLAGS_FOLDER / f"medium/{default_ideology}.tga"), str(PDX_FLAGS_FOLDER / f"medium/{tag_ideology}.tga"))
            shutil.copyfile(str(PDX_FLAGS_FOLDER / f"small/{default_ideology}.tga"), str(PDX_FLAGS_FOLDER / f"small/{tag_ideology}.tga"))


def main():
    time_start: float = perf_counter()

    ideologies: list[str] = load_ideologies()
    graphical_cultures: list[str] = load_graphical_cultures()
    countries, cosmetic_countries = load_countries(graphical_cultures)

    write_flags(countries, cosmetic_countries, ideologies)

    load_time: float = perf_counter() - time_start
    print(f"Load Time: {load_time:.3}s")

if __name__ == "__main__":
    main()
