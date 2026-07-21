from pathlib import Path
import re
from time import perf_counter
from typing import Any

import pandas as pd
from pandas import DataFrame
import json

MOD_DIRECTORY: Path = Path.cwd()
MOD_DIRECTORY = MOD_DIRECTORY.parents[1]

class Province:
    def __init__(self, prov_id: int):
        self.prov_id: int = prov_id
        self.state_id: int = 0
        self.names: set[str] = set()
        self.prev_victory_points: int = 0
        self.new_victory_points: int = 0

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

def FindDuplicates(provinces_list: list[Province], states_list: list[State], city_df: DataFrame):
    all_keys: dict[tuple[str, str], list[int]] = {}
    duplicate_tuples: set[tuple[str, str]] = set()

    for vp in provinces_list:
        state_owner = states_list[vp.state_id].owner
        if state_owner == "ZZZ":
            continue

        for name in vp.names:
            key = (state_owner, name)

            if key in all_keys:
                all_keys[key].append(vp.prov_id)
                duplicate_tuples.add(key)
            else:
                all_keys[key] = [vp.prov_id]

    print({key: value for key, value in all_keys.items() if key in duplicate_tuples})

def main():
    time_start: float = perf_counter()

    #Load the map
    provinces_list, states_list = LoadMap()

    #Load the city data
    #city_df: DataFrame = LoadCityData()

    #Load JSON file
    json_data = LoadJson()

    #Find duplicates
    #FindDuplicates(provinces_list, states_list, city_df)

    load_time: float = perf_counter()- time_start
    print(f"Load Time: {load_time:.3}s\n")


if __name__ == "__main__":
    main()