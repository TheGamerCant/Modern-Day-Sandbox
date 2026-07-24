from pathlib import Path
import re
import unicodedata
from time import perf_counter
from typing import Any

import pandas as pd
from pandas import DataFrame
import json

MOD_DIRECTORY: Path = Path.cwd()
MOD_DIRECTORY = MOD_DIRECTORY.parents[1]

# Modifier letters / quotes used in Arabic & Indic transliteration that are NOT
# combining marks, so NFKD won't strip them (e.g. ʿayn, ʾhamza, and the apostrophes
# often substituted for them). Removed explicitly so "Sanʿaʾ" == "Sana'a" == "Sanaa".
_TRANSLITERATION_MODIFIERS: frozenset[str] = frozenset(
    "ʿʾʼ’‘'`´"
)

# Hand-curated links for provinces the automatic matcher misses: VP name (left)
# -> the city as spelled in the population CSV (right). These are all confirmed
# to be the SAME place — mod typos ("Niagra Falls"), differing romanizations
# ("Homieĺ" / "Homel", "Tblisi" / "Tbilisi"), or transliteration schemes the
# normalizer can't reconcile ("Bāgalkōṭ" / "Bagalkote"). Fuzzy near-misses that
# are actually DIFFERENT cities (e.g. "Línzhī" vs "Linzi") are deliberately
# excluded. Both sides are normalized before use, so only the spelling matters.
# The CSV city is still looked up under the province's own owner tag, so an
# alias can never pull in a same-named city from the wrong country.
MANUAL_NAME_ALIASES: dict[str, str] = {
    "Castellón de la Plana": "Castelló de la Plana",
    "Bordj Bou Arréridj": "Bordj Bou Arreridjj",
    "Chapai Nawabganj": "Chapainawabganj",
    "Kaiserslauten": "Kaiserslautern",
    "Niagra Falls": "Niagara Falls",
    "Saarbrücken": "Saarbruecken",
    "Bhubaneswar": "Bhubaneshwar",
    "Chapayevsk": "Chapaevsk",
    "Davangere": "Davanagere",
    "Pampalona": "Pamplona",
    "Bāgalkōṭ": "Bagalkote",
    "Prishtina": "Pristina",
    "Debreccen": "Debrecen",
    "Hannover": "Hanover",
    "Chatres": "Chartres",
    "Nandyala": "Nandyal",
    "Vitebsk": "Vitsebsk",
    "Poiters": "Poitiers",
    "Uzhorod": "Uzhhorod",
    "Ras Al-Khaimah": "Ras Al Khaimah",
    "Severodonetsk": "Sievierodonetsk",
    "Gantok": "Gangtok",
    "Yadgiri": "Yadgir",
    "Tblisi": "Tbilisi",
    "Nikshic": "Nikšić",
    "Dar es Salaam": "Dar es-Salaam",
    "Tiruchengode": "Tiruchengodu",
    "Aïn Oussera": "Ain Oussara",
    "Homieĺ": "Homel",
    "Archangelsk": "Arkhangelsk",
    "Gothenberg": "Gothenburg",
    "Al-Mayādīn": "Al Mayadin",
    "Diyarbakır": "Diyarbakir",
    "As-Suwaydā'": "As Suwayda",
    "Ussuriysk": "Ussurijsk",
    "Berdyansk": "Berdiansk",
    "Nuremburg": "Nuremberg",
    "Slov'yans'k": "Sloviansk",
    "Ar-Rastan": "Al-Rastan",
}

def NormalizeName(name: str) -> str:
    """Fold a place name to a case- and accent-insensitive matching key.

    NFKD decomposition + dropping combining marks handles Latin diacritics and
    most Indic/Arabic transliteration marks (macrons, dots-below, etc. — e.g.
    'Ḥ' -> 'h', 'ā' -> 'a'). Transliteration modifier letters are stripped
    separately since they are not combining marks. `casefold` handles case
    (more aggressive than `lower` for non-ASCII), and whitespace is collapsed."""
    decomposed: str = unicodedata.normalize("NFKD", name)
    stripped: str = "".join(
        ch
        for ch in decomposed
        if not unicodedata.combining(ch) and ch not in _TRANSLITERATION_MODIFIERS
    )
    return " ".join(stripped.casefold().split())

class Province:
    def __init__(self, prov_id: int):
        self.prov_id: int = prov_id
        self.state_id: int = 0
        self.names: set[str] = set()
        self.prev_victory_points: int = 0
        self.population: int = 0

class State:
    def __init__(self, state_id: int, provinces: set[int], file: Path | None = None, owner: str = "ZZZ"):
        self.state_id: int = state_id
        self.provinces: set[int] = provinces
        self.owner: str = owner
        self.names: set[str] = set()
        self.file: Path | None = file

def LoadMap() -> tuple[list[Province], list[State]]:
    definition_file: Path = MOD_DIRECTORY / "map/definition.csv"
    states_dir: Path = MOD_DIRECTORY / "history/states"
    victory_point_loc: Path = MOD_DIRECTORY / "localisation/english/victory_points_l_english.yml"
    state_name_loc: Path = MOD_DIRECTORY / "localisation/english/state_names_l_english.yml"

    provinces_list: list[Province] = []

    with open(str(definition_file)) as f:
        lines: list[str] = f.readlines()
        incr: int = 0

        for i, line in enumerate(lines):
            prov_data: list[str] = line.split(";")
            if len(prov_data) > 6:
                provinces_list.append(Province(incr))
                incr += 1

    state_files: list[Path] = list(states_dir.glob("**/*.txt"))
    states_list: list[State] = [State(
        state_id=0,
        provinces=set()
    )]

    for state_file in state_files:
        with open(str(state_file)) as f:
            state_text: str = f.read().replace("\n", " ")

            state_id: int = int(re.search(r"id\s*=\s*(\d+)", state_text, re.IGNORECASE)[1])
            owner: str = re.search(r"owner\s*=\s*(\w+)", state_text, re.IGNORECASE)[1]
            provinces_list_string: str = re.search(r"provinces\s*=\s*{(.*?)}", state_text, re.IGNORECASE | re.DOTALL)[1]
            province_ids: set[int] = {int(prov.strip()) for prov in provinces_list_string.split(" ") if prov.strip().isdigit()}

            vps: list[tuple[str, str]] = re.findall(r"victory_points\s*=\s*{\s*(\d+)\s+(\d+)\s*}", state_text, re.IGNORECASE)

            for prov_id in province_ids:
                provinces_list[prov_id].state_id = state_id

            for prov_id, value in vps:
                provinces_list[int(prov_id)].prev_victory_points = int(value)

            states_list.append(State(
                state_id=state_id, 
                provinces=province_ids, 
                owner=owner,
                file=state_file
            ))

    states_list.sort(key=lambda x: x.state_id)

    with open(str(victory_point_loc), encoding="utf-8") as f:
        lines: list[str] = f.readlines()
        for line in lines:
            match = re.match(r'^VICTORY_POINTS_(\d+)[_:].*?"(.*)"', line.strip(), re.IGNORECASE)
            if match:
                vp_id: int = int(match[1])
                name: str = match[2]

                provinces_list[vp_id].names.add(name)

    with open(str(state_name_loc), encoding="utf-8") as f:
        lines: list[str] = f.readlines()
        for line in lines:
            match = re.match(r'^STATE_(\d+)[_:].*?"(.*)"', line.strip(), re.IGNORECASE)
            if match:
                state_id: int = int(match[1])
                name: str = match[2]

                states_list[state_id].names.add(name)

    return provinces_list, states_list

def LoadCityData() -> DataFrame:
    data_dir: Path = Path.cwd() / "data"
    data_files: list[Path] = list(data_dir.glob("**/*.csv"))

    df: DataFrame = pd.concat((pd.read_csv(f) for f in data_files), ignore_index=True)

    df["Population (2025)"] = df["Population (2025)"].round().astype(int)
    return df

def LoadJson() -> Any:
    json_file: Path = Path.cwd() / "tag_data.json"
    json_data: Any = None

    with open(str(json_file), "r", encoding="utf-8") as f:
        json_data = json.load(f)

    return json_data

def FindDuplicates(provinces_list: list[Province], states_list: list[State]) -> set[int]:
    # (owner tag, normalized name) -> the distinct provinces carrying that name.
    # A set of prov_ids per key is essential: a single province often lists
    # several spellings of its own name (e.g. "Qingyang" / "Qìngyáng"), which
    # collapse to one key after normalization but must NOT count as ambiguous.
    prov_ids_by_key: dict[tuple[str, str], set[int]] = {}

    for province in provinces_list:
        owner_tag = states_list[province.state_id].owner
        if owner_tag == "ZZZ":
            continue

        for name in province.names:
            key = (owner_tag, NormalizeName(name))
            prov_ids_by_key.setdefault(key, set()).add(province.prov_id)

    # Ambiguous only when two or more DIFFERENT provinces share a name+owner.
    duplicate_provinces: set[int] = set()
    for prov_ids in prov_ids_by_key.values():
        if len(prov_ids) > 1:
            duplicate_provinces.update(prov_ids)

    return duplicate_provinces

def LinkPopulations(
    provinces_list: list[Province],
    states_list: list[State],
    cities_to_population_dict: dict[tuple[str, str], int],
    duplicate_provinces: set[int],
) -> dict[int, int]:
    """Assign a population to each province where a city under the province's
    owner tag matches one of its names. Returns prov_id -> population for the
    provinces that were linked; also sets `Province.population` in place."""
    prov_id_to_population: dict[int, int] = {}

    # Normalize the manual alias table once: normalized VP name -> normalized city name.
    aliases_by_name: dict[str, str] = {
        NormalizeName(vp_name): NormalizeName(city_name)
        for vp_name, city_name in MANUAL_NAME_ALIASES.items()
    }

    for province in provinces_list:
        # Skip provinces whose (owner, name) key is shared — can't tell which
        # province the city population belongs to.
        if province.prov_id in duplicate_provinces:
            continue

        owner_tag: str = states_list[province.state_id].owner
        if owner_tag == "ZZZ":
            continue

        # For each name try a direct match, then fall back to a manual alias.
        matched_populations: list[int] = []
        for name in province.names:
            normalized_name: str = NormalizeName(name)
            city_key: str = aliases_by_name.get(normalized_name, normalized_name)
            if (city_key, owner_tag) in cities_to_population_dict:
                matched_populations.append(cities_to_population_dict[(city_key, owner_tag)])
        if not matched_populations:
            continue

        # A province may carry several names; take the largest matching city.
        population: int = max(matched_populations)
        province.population = population
        prov_id_to_population[province.prov_id] = population

    return prov_id_to_population

def WritePopulationCsv(
    provinces_list: list[Province],
    states_list: list[State],
    prov_id_to_population: dict[int, int],
    output_path: Path,
) -> DataFrame:
    """Write one row per VP province: prov_id, names, owner tag and population.
    Population is blank (None) for provinces with no matched city."""
    population_records: list[dict[str, Any]] = [
        {
            "prov_id": province.prov_id,
            "names": "; ".join(sorted(province.names)),
            "owner": states_list[province.state_id].owner,
            # .get -> None when unmatched, which na_rep renders as an empty cell.
            "population": prov_id_to_population.get(province.prov_id),
        }
        for province in provinces_list
    ]

    populations_df: DataFrame = pd.DataFrame(
        population_records, columns=["prov_id", "names", "owner", "population"]
    )
    # Nullable integer so unmatched rows stay blank rather than becoming floats.
    populations_df["population"] = populations_df["population"].astype("Int64")
    # utf-8-sig writes a BOM so Excel detects UTF-8 and renders accented /
    # transliterated names correctly instead of mojibake.
    populations_df.to_csv(output_path, index=False, na_rep="", encoding="utf-8-sig")

    return populations_df

def main():
    time_start: float = perf_counter()

    # Load the map
    provinces_list, states_list = LoadMap()

    # Only look at provinces with VPs
    provinces_list = [prov for prov in provinces_list if prov.prev_victory_points > 0]

    # Load the city data
    city_df: DataFrame = LoadCityData()

    # Load JSON file
    json_data = LoadJson()
    country_name_to_tag_dict: dict[str, str] = {
        key: value.get("tag") for key, value in json_data.get("tag_map").items()
    }

    load_time: float = perf_counter()- time_start
    print(f"Load Time: {load_time:.3}s\n")

    # Find duplicates
    duplicate_provinces: set[int] = FindDuplicates(provinces_list, states_list)

    # Tuple (city name, country tag) -> population
    cities_to_population_dict: dict[tuple[str, str], int] = dict(zip(
        zip(city_df["City"].map(NormalizeName), city_df["Country"].map(country_name_to_tag_dict)),
        city_df["Population (2025)"],
    ))

    # Province ID -> population, for provinces whose name matches a city
    # owned by the province's state owner.
    prov_id_to_population: dict[int, int] = LinkPopulations(
        provinces_list, states_list, cities_to_population_dict, duplicate_provinces
    )

    print(
        f"Linked {len(prov_id_to_population)} of {len(provinces_list)} "
        f"VP provinces to a population."
    )

    # Write every VP province out to CSV, blank where no population was found.
    output_path: Path = Path.cwd() / "vp_populations_2.csv"
    WritePopulationCsv(provinces_list, states_list, prov_id_to_population, output_path)
    print(f"Wrote {output_path.name}")

    return prov_id_to_population

if __name__ == "__main__":
    main()