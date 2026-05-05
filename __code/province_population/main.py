from pathlib import Path
import re
from time import perf_counter
import pandas as pd
from pandas import DataFrame


class Province:
    def __init__(self, prov_id: int):
        self.prov_id: int = prov_id
        self.state_id: int = 0
        self.names: set[str] = set()

class State:
    def __init__(self, state_id: int, provinces: set[int], owner: str = "ZZZ"):
        self.state_id: int = state_id
        self.provinces: set[int] = provinces
        self.owner: str = owner
        self.names: set[str] = set()

def LoadMap(mod_dir: Path) -> tuple[list[Province], list[State]]:
    definition_file: Path = mod_dir / "map/definition.csv"
    states_dir: Path = mod_dir / "history/states"
    victory_point_loc: Path = mod_dir / "localisation/english/victory_points_l_english.yml"
    state_name_loc: Path = mod_dir / "localisation/english/state_names_l_english.yml"

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
    states_list: list[State] = [State(0, set())]

    for state_file in state_files:
        with open(str(state_file)) as f:
            state_text: str = f.read().replace("\n", " ")

            state_id: int = int(re.search(r"id\s*=\s*(\d+)", state_text, re.IGNORECASE)[1])
            owner: str = re.search(r"owner\s*=\s*(\w+)", state_text, re.IGNORECASE)[1]
            provinces_list_string: str = re.search(r"provinces\s*=\s*{(.*?)}", state_text, re.IGNORECASE | re.DOTALL)[1]
            province_ids: set[int] = {int(prov.strip()) for prov in provinces_list_string.split(" ") if prov.strip().isdigit()}

            for prov_id in province_ids:
                provinces_list[prov_id].state_id = state_id

            states_list.append(State(state_id, province_ids, owner))

    states_list.sort(key=lambda x: x.state_id)

    with open(str(victory_point_loc)) as f:
        lines: list[str] = f.readlines()
        for line in lines:
            match = re.match(r'^VICTORY_POINTS_(\d+)[_:].*?"(.*)"', line.strip(), re.IGNORECASE)
            if match:
                vp_id: int = int(match[1])
                name: str = match[2]

                provinces_list[vp_id].names.add(name)

    with open(str(state_name_loc)) as f:
        lines: list[str] = f.readlines()
        for line in lines:
            match = re.match(r'^STATE_(\d+)[_:].*?"(.*)"', line.strip(), re.IGNORECASE)
            if match:
                state_id: int = int(match[1])
                name: str = match[2]

                states_list[state_id].names.add(name)

    return provinces_list, states_list

def LoadCityData(mod_dir: Path) -> DataFrame:
    data_dir: Path = mod_dir / "__code/province_population/data"
    data_files: list[Path] = list(data_dir.glob("**/*.csv"))

    df: DataFrame = pd.concat((pd.read_csv(f) for f in data_files), ignore_index=True)
    df["Population (2025)"] = df["Population (2025)"].round().astype(int)
    return df

def main():
    time_start: float = perf_counter()

    mod_directory: Path = Path.cwd()

    #Load the map
    provinces_list, states_list = LoadMap(mod_directory)

    #Load the city data
    df: DataFrame = LoadCityData(mod_directory)

    load_time: float = perf_counter()- time_start
    print(f"Load Time: {load_time:.3}s\n")



if __name__ == "__main__":
    main()