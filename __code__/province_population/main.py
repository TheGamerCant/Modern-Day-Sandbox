from collections import Counter
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
    # Second batch — weird-format / romanization mismatches against EU_data +
    # citypopulation. Diacritics & tone marks the normalizer can't bridge,
    # administrative suffixes/abbreviations, parentheticals, and mod typos.
    "Niagra Falls": "Niagara Falls",
    "Villa Real": "Vila Real",
    "Nykøbing Falster": "Nykøbing (Falster)",
    "Bismark": "Bismarck",
    "Zhucheng": "Zucheng",
    "Sumgayt": "Sumgayit",
    "Subshine Coast": "Sunshine Coast",
    "Khamis Mushait": "Khamis Mushayt",
    "Rijelka": "Rijeka",
    "Mingechevir": "Mingachevir",
    "Bangalore": "Bengaluru",
    "Shamakhy": "Shamakhi",
    "Saryagash": "Sarıağaş",
    "St. Petersburg": "Saint Petersburg",
    "St Pölten": "Sankt Pölten",
    "Jalagon": "Jālgaon",
    "Hromtau": "Hromtaý",
    "Tuymazy": "Tujmazy",
    "Cumilla": "Comilla",
    "Al-Thawrah": "Ath-Thawrah",
    "Gwailor": "Gwalior",
    "Taif": "At Ta'if",
    "Cheongju": "Cheongju-si",
    "Chittagong": "Chattogram",
    # Third batch — Slavic/Kazakh scientific-vs-English transliteration (iy/y,
    # y/j, kh/h, shch/sh) that the bracketed English form doesn't fully cover,
    # plus renamed cities and a typo.
    "Petropavlovsk-Kamchatskiy": "Petropavlovsk-Kamchatsky",
    "Bilhorod-Dnistrovsky": "Bilhorod-Dnistrovskyj",
    "Leninsk-Kuznetskiy": "Leninsk-Kuznetsky",
    "Shakhtinsk": "Shahtinsk",
    "Kostyantynivka": "Kostjantynivka",
    "Rudny": "Rudniy",
    "Zyryanovsk": "Zyrjanovsk",
    "Shchuchinsk": "Shuchinsk",
    "Pular": "Pula",
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

def _bracketed_contents(text: str) -> tuple[list[str], str]:
    """Pull the contents of every () / [] group (handling nesting) and return
    them plus the "main" text left once all bracket groups are removed.

    Repeatedly stripping the innermost group copes with nested labels like
    "Albury ( - Wodonga (Vic.) )", which a single-level regex would mangle into
    "Albury )"."""
    contents: list[str] = []
    stripped = text
    previous: str | None = None
    innermost = re.compile(r"[(\[]([^()\[\]]*)[)\]]")

    def _collect(match: "re.Match[str]") -> str:
        contents.append(match.group(1).strip())
        return " "

    while previous != stripped:
        previous = stripped
        stripped = innermost.sub(_collect, stripped)
    return contents, " ".join(stripped.split())


def _expand_city_variants(city: str) -> tuple[set[str], set[str]]:
    """Split a label like "Name 1 (Name 2) [Name 3]" into its component names.

    citypopulation.de often packs several names into one label — an alternate
    or local/indigenous name in brackets (e.g. "Apatula (Finke)",
    "Galiwinku (Elcho Island)"), a native-script name with the English form in
    brackets ("Tajšet [ Tayshet ]"), or a dual name ("Wagait Beach - Mandorah",
    "Albury ( - Wodonga (Vic.) )"). If only the whole string is a key, a VP
    called just "Finke" or "Wodonga" never matches.

    Returns (primary, alternate) sets of NORMALIZED names:
    * primary   = the full label + the "main" name (label minus bracketed parts)
    * alternate = each bracket group's content and each ' - '-split part

    Keeping them separate lets the caller give primary names precedence, so an
    alternate (sometimes just a region/disambiguator) can't clobber another
    city's real name."""
    full = city.strip()
    contents, main = _bracketed_contents(full)

    primary_raw: set[str] = {full}
    alternate_raw: set[str] = set()
    if main:
        primary_raw.add(main)

    # A ' - ' inside the main text or a bracket group joins twin cities.
    for segment in [main, *contents]:
        cleaned = segment.strip().lstrip("-").strip()
        if not cleaned:
            continue
        if segment is not main:
            alternate_raw.add(cleaned)
        for part in cleaned.split(" - "):
            if part.strip():
                alternate_raw.add(part.strip())

    primary = {NormalizeName(name) for name in primary_raw if name.strip()}
    alternate = {NormalizeName(name) for name in alternate_raw if name.strip()} - primary
    return primary, alternate


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

# citypopulation.de derives its country column from URL slugs, so a handful
# don't match the tag_map's full country names. Bridge them here.
_COUNTRY_ALIASES: dict[str, str] = {
    "Bosnia": "Bosnia and Herzegovina",
    "Czechrep": "Czechia",
    "Domrep": "Dominican Republic",
    "Mexico": "México",
    "Northkorea": "North Korea",
    "Saudiarabia": "Saudi Arabia",
    "Southkorea": "South Korea",
    "USA": "United States",
    "Uae": "United Arab Emirates",
    "Uk": "United Kingdom",
}

# Mod-States for China are usually named after the Chinese province (which
# matches china_citypopulation.csv's `state` column), but some are prefectures.
# Map those prefectures to their province so the `state` disambiguation works.
_CHINA_PREFECTURE_TO_PROVINCE: dict[str, str] = {
    "chifeng": "neimenggu", "hohhot": "neimenggu", "huhehaote": "neimenggu",
    "kokeqota": "neimenggu", "hulunbeier": "neimenggu", "hulunbuir": "neimenggu",
    "kolon buyir": "neimenggu", "alxa": "neimenggu", "alashan": "neimenggu",
    "alasa ayimay": "neimenggu", "xilingol": "neimenggu", "xilinguole": "neimenggu",
    "sili-yin yool": "neimenggu", "ulaganqada": "neimenggu",
    "kashgar": "xinjiang", "kashi": "xinjiang", "qeshqer": "xinjiang",
    "urumqi": "xinjiang", "urumchi": "xinjiang", "wulumuqi": "xinjiang",
    "shijiazhuang": "hebei", "tangshan": "hebei", "zhangjiakou": "hebei",
    "harbin": "heilongjiang", "haerbin": "heilongjiang",
    "tibet": "xizang", "bejing": "beijing",
}


def LoadEuData() -> DataFrame:
    """Primary source of truth: the hand-curated EU_data regional CSVs
    (columns City, Country, Population (2025))."""
    data_dir: Path = Path.cwd() / "EU_data"
    data_files: list[Path] = list(data_dir.glob("**/*.csv"))

    eu_df: DataFrame = pd.concat((pd.read_csv(f) for f in data_files), ignore_index=True)
    eu_df["Population (2025)"] = eu_df["Population (2025)"].round().astype(int)
    return eu_df

def LoadCityPopulationData() -> DataFrame:
    """Fallback source: the scraped citypopulation.de data
    (columns city, country, population, ...)."""
    data_file: Path = Path.cwd() / "citypopulation_data" / "city_populations.csv"
    citypop_df: DataFrame = pd.read_csv(data_file)
    citypop_df = citypop_df.dropna(subset=["population"])
    citypop_df["population"] = citypop_df["population"].round().astype(int)
    return citypop_df

def _extract_year(value: Any) -> int | None:
    """Pull a 4-digit year out of a value (a date string, column name, etc.)."""
    match = re.search(r"\d{4}", str(value))
    return int(match.group(0)) if match else None


def LoadChinaData() -> DataFrame:
    """China cities scraped per-province (columns city, country, population,
    reference_date, state) — carries a `state` column for disambiguation."""
    data_file: Path = Path.cwd() / "china_citypopulation.csv"
    china_df: DataFrame = pd.read_csv(data_file)
    china_df = china_df.dropna(subset=["population"])
    china_df["population"] = china_df["population"].round().astype(int)
    return china_df


def BuildPopulationLookup(
    city_df: DataFrame,
    city_column: str,
    country_column: str,
    population_column: str,
    country_name_to_tag_dict: dict[str, str],
    year_column: str | None = None,
    default_year: int | None = None,
) -> dict[tuple[str, str], tuple[int, int | None]]:
    """Build a (normalized city, owner tag) -> (population, year) dict.

    Works for either source by naming its columns. The year comes from
    ``year_column`` per row (e.g. citypopulation's reference_date) or from
    ``default_year`` (e.g. EU_data's single 2025 vintage). Rows whose country
    doesn't resolve to a tag are skipped — their key could never match a
    province's owner tag anyway."""
    tags = city_df[country_column].map(country_name_to_tag_dict)
    if year_column is not None:
        years: Any = city_df[year_column].map(_extract_year)
    else:
        years = [default_year] * len(city_df)

    lookup: dict[tuple[str, str], tuple[int, int | None]] = {}
    # Alternate names are collected separately and only added where no primary
    # name already claims the key (see _expand_city_variants).
    alternate_lookup: dict[tuple[str, str], tuple[int, int | None]] = {}
    for city, tag, population, year in zip(
        city_df[city_column], tags, city_df[population_column], years
    ):
        if not isinstance(tag, str):
            continue
        # pandas may widen an int column to float when NaNs are present;
        # coerce back to a clean int (or None) so years aren't "2020.0".
        clean_year = int(year) if pd.notna(year) else None
        value = (int(population), clean_year)

        primary_names, alternate_names = _expand_city_variants(str(city))
        for name in primary_names:
            lookup[(name, tag)] = value
        for name in alternate_names:
            alternate_lookup.setdefault((name, tag), value)

    for key, value in alternate_lookup.items():
        lookup.setdefault(key, value)
    return lookup

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
    primary_lookup: dict[tuple[str, str], tuple[int, int | None]],
    fallback_lookup: dict[tuple[str, str], tuple[int, int | None]],
    duplicate_provinces: set[int],
) -> dict[int, tuple[int, int | None]]:
    """Assign a population to each province.

    ``primary_lookup`` (EU_data) is the source of truth; ``fallback_lookup``
    (citypopulation.de) is consulted ONLY for provinces the primary can't
    resolve. Returns prov_id -> (population, year) for linked provinces; also
    sets `Province.population` in place."""
    # Normalize the manual alias table once: normalized VP name -> normalized city name.
    aliases_by_name: dict[str, str] = {
        NormalizeName(vp_name): NormalizeName(city_name)
        for vp_name, city_name in MANUAL_NAME_ALIASES.items()
    }

    def matched_entries(
        lookup: dict[tuple[str, str], tuple[int, int | None]],
        province: Province,
        owner_tag: str,
    ) -> list[tuple[int, int | None]]:
        # For each name try both its normalized form and any manual alias.
        entries: list[tuple[int, int | None]] = []
        for name in province.names:
            normalized_name = NormalizeName(name)
            for city_key in {normalized_name, aliases_by_name.get(normalized_name, normalized_name)}:
                if (city_key, owner_tag) in lookup:
                    entries.append(lookup[(city_key, owner_tag)])
        return entries

    prov_id_to_result: dict[int, tuple[int, int | None]] = {}
    for province in provinces_list:
        # Skip provinces whose (owner, name) key is shared — can't tell which
        # province the city population belongs to.
        if province.prov_id in duplicate_provinces:
            continue

        owner_tag: str = states_list[province.state_id].owner
        if owner_tag == "ZZZ":
            continue

        # EU_data first; only consult citypopulation if EU_data found nothing.
        entries = matched_entries(primary_lookup, province, owner_tag)
        if not entries:
            entries = matched_entries(fallback_lookup, province, owner_tag)
        if not entries:
            continue

        # A province may carry several names; take the largest matching city
        # and keep the year that figure refers to.
        population, year = max(entries, key=lambda pop_year: pop_year[0])
        province.population = population
        prov_id_to_result[province.prov_id] = (population, year)

    return prov_id_to_result

def LinkChineseProvinces(
    provinces_list: list[Province],
    states_list: list[State],
    china_df: DataFrame,
    prov_id_to_result: dict[int, tuple[int, int | None]],
) -> int:
    """Fill still-empty PRC provinces from china_citypopulation.csv.

    Chinese city names collide heavily across provinces, so this uses the mod's
    State name (a Chinese province, or a prefecture mapped to one) to pick the
    right city when a name is ambiguous. A name that resolves to a single
    province is taken directly. Only provinces with no existing result are
    touched. Returns how many were filled; also sets Province.population."""
    # Expanded name -> list of (chinese state, population, year).
    china_index: dict[str, set[tuple[str, int, int | None]]] = {}
    china_provinces: set[str] = set()
    for city, state, population, reference_date in zip(
        china_df["city"], china_df["state"], china_df["population"], china_df["reference_date"]
    ):
        normalized_state = NormalizeName(str(state))
        china_provinces.add(normalized_state)
        year = _extract_year(reference_date)
        primary_names, alternate_names = _expand_city_variants(str(city))
        for name in primary_names | alternate_names:
            china_index.setdefault(name, set()).add((normalized_state, int(population), year))

    filled = 0
    for province in provinces_list:
        if states_list[province.state_id].owner != "PRC":
            continue
        if province.prov_id in prov_id_to_result:
            continue  # only fill empties

        # The province's Chinese state(s), prefectures mapped to their province.
        province_states = {
            _CHINA_PREFECTURE_TO_PROVINCE.get(NormalizeName(name), NormalizeName(name))
            for name in states_list[province.state_id].names
        }

        candidates: set[tuple[str, int, int | None]] = set()
        for name in province.names:
            candidates |= china_index.get(NormalizeName(name), set())
        if not candidates:
            continue

        in_state = [entry for entry in candidates if entry[0] in province_states]
        if in_state:
            # State-verified — the safest match.
            chosen = max(in_state, key=lambda entry: entry[1])
        elif province_states & china_provinces:
            # The province's real Chinese state is known but no candidate sits
            # in it — so a same-named city elsewhere would be the WRONG match
            # (e.g. Binhai/Tianjin vs Binhai/Jiangsu). Leave it empty.
            continue
        elif len({state for state, _, _ in candidates}) == 1:
            # State unknown (e.g. an unmapped prefecture) but the name is
            # unambiguous across the data — safe to take.
            chosen = max(candidates, key=lambda entry: entry[1])
        else:
            continue  # ambiguous and unresolvable

        _, population, year = chosen
        province.population = population
        prov_id_to_result[province.prov_id] = (population, year)
        filled += 1

    return filled


def WritePopulationCsv(
    provinces_list: list[Province],
    states_list: list[State],
    prov_id_to_result: dict[int, tuple[int, int | None]],
    output_path: Path,
) -> DataFrame:
    """Write one row per VP province: prov_id, names, owner tag, population and
    the year that figure refers to. Population and year are blank for provinces
    with no matched city."""
    population_records: list[dict[str, Any]] = [
        {
            "prov_id": province.prov_id,
            "names": "; ".join(sorted(province.names)),
            "owner": states_list[province.state_id].owner,
            # .get -> (None, None) when unmatched, rendered as empty cells.
            "population": prov_id_to_result.get(province.prov_id, (None, None))[0],
            "year": prov_id_to_result.get(province.prov_id, (None, None))[1],
        }
        for province in provinces_list
    ]

    populations_df: DataFrame = pd.DataFrame(
        population_records, columns=["prov_id", "names", "owner", "population", "year"]
    )
    # Nullable integer so unmatched rows stay blank rather than becoming floats.
    populations_df["population"] = populations_df["population"].astype("Int64")
    populations_df["year"] = populations_df["year"].astype("Int64")
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

    # Load both city-data sources
    eu_df: DataFrame = LoadEuData()
    citypop_df: DataFrame = LoadCityPopulationData()

    # Load JSON file
    json_data = LoadJson()
    country_name_to_tag_dict: dict[str, str] = {
        key: value.get("tag") for key, value in json_data.get("tag_map").items()
    }
    # citypopulation uses slug-style country names; bridge them to the tag_map.
    citypop_country_to_tag_dict: dict[str, str] = dict(country_name_to_tag_dict)
    for slug, full_name in _COUNTRY_ALIASES.items():
        citypop_country_to_tag_dict[slug] = country_name_to_tag_dict.get(full_name)

    load_time: float = perf_counter()- time_start
    print(f"Load Time: {load_time:.3}s\n")

    # Find duplicates
    duplicate_provinces: set[int] = FindDuplicates(provinces_list, states_list)

    # (normalized city, tag) -> (population, year), one lookup per source.
    # EU_data carries a single 2025 vintage (read from its column name);
    # citypopulation carries a per-row reference_date.
    eu_population_column: str = "Population (2025)"
    primary_lookup: dict[tuple[str, str], tuple[int, int | None]] = BuildPopulationLookup(
        eu_df, "City", "Country", eu_population_column, country_name_to_tag_dict,
        default_year=_extract_year(eu_population_column),
    )
    fallback_lookup: dict[tuple[str, str], tuple[int, int | None]] = BuildPopulationLookup(
        citypop_df, "city", "country", "population", citypop_country_to_tag_dict,
        year_column="reference_date",
    )

    # Province ID -> (population, year). EU_data is authoritative; citypopulation
    # only fills provinces EU_data can't resolve.
    prov_id_to_result: dict[int, tuple[int, int | None]] = LinkPopulations(
        provinces_list, states_list, primary_lookup, fallback_lookup, duplicate_provinces
    )

    # Fill still-empty Chinese provinces from the per-province China scrape,
    # using the mod-State name to disambiguate same-named cities.
    china_df: DataFrame = LoadChinaData()
    china_filled = LinkChineseProvinces(provinces_list, states_list, china_df, prov_id_to_result)
    print(f"China pass filled {china_filled} more PRC provinces.")

    year_counts = Counter(year for _, year in prov_id_to_result.values())
    year_summary = ", ".join(
        f"{year}: {count}" for year, count in sorted(year_counts.items(), key=lambda yc: (yc[0] is None, yc[0]))
    )
    print(
        f"Linked {len(prov_id_to_result)} of {len(provinces_list)} VP provinces "
        f"(by year — {year_summary})."
    )

    # Remaining unmatched, ignoring provinces that are airports (never cities).
    unmatched = [p for p in provinces_list if p.prov_id not in prov_id_to_result]
    airport_unmatched = [
        p for p in unmatched if any("airport" in name.lower() for name in p.names)
    ]
    print(
        f"Remaining unmatched (excluding {len(airport_unmatched)} airports): "
        f"{len(unmatched) - len(airport_unmatched)}."
    )

    # Write every VP province out to CSV, blank where no population was found.
    output_path: Path = Path.cwd() / "vp_populations_2.csv"
    WritePopulationCsv(provinces_list, states_list, prov_id_to_result, output_path)
    print(f"Wrote {output_path.name}")

    return prov_id_to_result

if __name__ == "__main__":
    main()