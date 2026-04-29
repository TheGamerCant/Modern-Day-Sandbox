from pathlib import Path
import re
from time import perf_counter
from PIL import Image
import numpy as np

class Province:
    def __init__(self, province_id: int, red: int, green: int, blue: int, definition_entry: str, state_id: int | None = None, strategic_region_id: int | None = None):
        self.province_id: int = province_id
        self.red: int = red
        self.green: int = green
        self.blue: int = blue
        self.definition_entry: str = definition_entry
        self.state_id: int | None = state_id
        self.strategic_region_id: int | None = strategic_region_id

class State:
    def __init__(self, state_id: int, provinces: set[int], file_path: Path | None):
        self.state_id: int = state_id
        self.provinces: set[int] = provinces
        self.file_path: Path | None = file_path

class StrategicRegion:
    def __init__(self, strategic_region_id: int, provinces: set[int], file_path: Path | None):
        self.strategic_region_id: int = strategic_region_id
        self.provinces: set[int] = provinces
        self.file_path: Path | None = file_path

def LoadMap(mod_dir: Path) -> tuple[list[Province], list[State], list[StrategicRegion]]:
    definition_file: Path = mod_dir / "map/definition.csv"
    states_dir: Path = mod_dir / "history/states"
    strategic_regions_dir: Path = mod_dir / "map/strategicregions"

    provinces_list: list[Province] = []

    with open(str(definition_file)) as f:
        lines: list[str] = f.readlines()

        for i, line in enumerate(lines):
            prov_data: list[str] = line.split(";", 4)
            provinces_list.append(
                Province(
                    province_id=i,
                    red=int(prov_data[1]),
                    green=int(prov_data[2]),
                    blue=int(prov_data[3]),
                    definition_entry=prov_data[4]
                )
            )

    state_files: list[Path] = list(states_dir.glob("**/*.txt"))
    states_list: list[State] = [State(0, set(), None)]

    for s_file in state_files:
        with open(str(s_file)) as f:
            state_text: str = f.read().replace("\n", " ")

            state_id: int = int(re.search(r"id\s*=\s*(\d+)", state_text, re.IGNORECASE)[1])
            provinces_list_string: str = re.search(r"provinces\s*=\s*{(.*?)}", state_text, re.IGNORECASE | re.DOTALL)[1]
            province_ids: set[int] = {int(prov.strip()) for prov in provinces_list_string.split(" ") if prov.strip().isdigit()}

            for prov_id in province_ids:
                provinces_list[prov_id].state_id = state_id

            states_list.append(State(state_id, province_ids, s_file))

    strategic_regions_files: list[Path] = list(strategic_regions_dir.glob("**/*.txt"))
    strategic_regions_list: list[StrategicRegion] = [StrategicRegion(0, set(), None)]

    for sr_file in strategic_regions_files:
        with open(str(sr_file)) as f:
            strategic_region_text: str = f.read().replace("\n", " ")

            strategic_region_id: int = int(re.search(r"id\s*=\s*(\d+)", strategic_region_text, re.IGNORECASE)[1])
            provinces_list_string: str = re.search(r"provinces\s*=\s*{(.*?)}", strategic_region_text, re.IGNORECASE)[1]
            province_ids: set[int] = {int(prov.strip()) for prov in provinces_list_string.split(" ") if prov.strip().isdigit()}

            for prov_id in province_ids:
                provinces_list[prov_id].strategic_region_id = strategic_region_id

            strategic_regions_list.append(StrategicRegion(strategic_region_id, province_ids, sr_file))

    states_list.sort(key=lambda x: x.state_id)
    strategic_regions_list.sort(key=lambda x: x.strategic_region_id)

    return provinces_list, states_list, strategic_regions_list

def LoadProvincesBitmap(mod_dir: Path) -> np.ndarray:
    bmp_file: Path = mod_dir / "map/provinces.bmp"
    img = Image.open(bmp_file)
    rgb_vals: np.ndarray = np.array(img)
    img.close()

    return rgb_vals

def MergeProvinces(
        provinces_list: list[Province],
        states_list: list[State],
        strategic_regions_list: list[StrategicRegion],
        provinces_bitmap: np.ndarray,
        prov_to_remove: int,
        prov_to_merge_into: int,
        changed_states: set[int],
        changed_strategic_regions: set[int],
        old_to_new_ids: dict[int, int]
    ):
    #Step 1 - Remove prov_to_remove from it's state and strategic region
    prov_to_remove_state: int = provinces_list[prov_to_remove].state_id
    if prov_to_remove_state:
        states_list[prov_to_remove_state].provinces.remove(prov_to_remove)
        changed_states.add(prov_to_remove_state)

    prov_to_remove_strategic_region: int = provinces_list[prov_to_remove].strategic_region_id
    strategic_regions_list[prov_to_remove_strategic_region].provinces.remove(prov_to_remove)
    changed_strategic_regions.add(prov_to_remove_strategic_region)

    #Step 2 - Change the ID of the last prov to the one to remove in it's state and strategic region
    last_prov_id: int = len(provinces_list) - 1
    last_prov_state: int = provinces_list[last_prov_id].state_id
    if last_prov_state:
        states_list[last_prov_state].provinces.remove(last_prov_id)
        states_list[last_prov_state].provinces.add(prov_to_remove)
        changed_states.add(last_prov_state)

    last_prov_strategic_region: int = provinces_list[last_prov_id].strategic_region_id
    strategic_regions_list[last_prov_strategic_region].provinces.remove(last_prov_id)
    strategic_regions_list[last_prov_strategic_region].provinces.add(prov_to_remove)
    changed_strategic_regions.add(last_prov_strategic_region)

    old_to_new_ids[last_prov_id] = prov_to_remove

    #Step 3 - Merge the bitmap colours
    old_color = np.array([provinces_list[prov_to_remove].red, provinces_list[prov_to_remove].green, provinces_list[prov_to_remove].blue])
    new_color = np.array([provinces_list[prov_to_merge_into].red, provinces_list[prov_to_merge_into].green, provinces_list[prov_to_merge_into].blue])

    mask = np.all(provinces_bitmap == old_color, axis=-1)
    provinces_bitmap[mask] = new_color

    prov_to_merge_into_state: int = provinces_list[prov_to_merge_into].state_id
    if prov_to_merge_into_state:
        changed_states.add(prov_to_merge_into_state)

    prove_to_merge_into_strategic_region: int = provinces_list[prov_to_merge_into].strategic_region_id
    changed_strategic_regions.add(prove_to_merge_into_strategic_region)

    #Step 4 - Update the definitions file
    provinces_list[prov_to_remove] = Province(
        province_id=prov_to_remove,
        red=provinces_list[last_prov_id].red,
        green=provinces_list[last_prov_id].green,
        blue=provinces_list[last_prov_id].blue,
        definition_entry=provinces_list[last_prov_id].definition_entry,
        state_id=provinces_list[last_prov_id].state_id,
        strategic_region_id=provinces_list[last_prov_id].strategic_region_id,
    )
    provinces_list.pop()

def WriteProvinceDefinitions(mod_dir: Path, provinces_list: list[Province]):
    definition_file: Path = mod_dir / "map/definition.csv"

    with open(str(definition_file), "w") as f:
        for prov in provinces_list:
            f.write(f"{prov.province_id};{prov.red};{prov.green};{prov.blue};{prov.definition_entry}{'' if prov.definition_entry.endswith('\n') else '\n'}")

def UpdateStateAndStrategicRegionFiles(
        states_list: list[State],
        strategic_regions_list: list[StrategicRegion],
        changed_states: set[int],
        changed_strategic_regions: set[int],
        old_to_new_ids: dict[int, int]
    ):

    changed_states = {state for state in changed_states if state < len(states_list)}
    changed_strategic_regions = {sr for sr in changed_strategic_regions if sr < len(strategic_regions_list)}

    for state_id in changed_states:
        with open(str(states_list[state_id].file_path)) as f:
            text = f.read()

        provinces_str = ' '.join(map(str, sorted(states_list[state_id].provinces)))

        text = re.sub(
            r"provinces\s*=\s*{(.*?)}",
            f"provinces = {{\n\t\t{provinces_str}\n\t}}",
            text,
            flags=re.IGNORECASE | re.DOTALL
        )

        text: str = re.sub(
            r"victory_points\s*=\s*{\s*(\d+)\s+(\d+)\s*}",
            lambda m: f"victory_points = {{ {old_to_new_ids.get(int(m.group(1)), m.group(1))} {m.group(2)} }}",
            text
        )

        with open(str(states_list[state_id].file_path), "w") as f:
            f.write(text)

    for strategic_region_id in changed_strategic_regions:
        with open(str(strategic_regions_list[strategic_region_id].file_path)) as f:
            text = f.read()

        provinces_str = ' '.join(map(str, sorted(strategic_regions_list[strategic_region_id].provinces)))

        text = re.sub(
            r"provinces\s*=\s*{(.*?)}",
            f"provinces = {{ {provinces_str} }}",
            text,
            flags=re.IGNORECASE | re.DOTALL
        )

        with open(str(strategic_regions_list[strategic_region_id].file_path), "w") as f:
            f.write(text)



def main():
    time_start: float = perf_counter()

    mod_directory: Path = Path.cwd()
    mod_directory = mod_directory.parents[1]

    provinces_list, states_list, strategic_regions_list = LoadMap(
        mod_dir=mod_directory
    )

    #[height][width][r/g/b]
    provinces_bitmap: np.ndarray = LoadProvincesBitmap(mod_dir=mod_directory)

    changed_states: set[int] = set()
    changed_strategic_regions: set[int] = set()
    old_to_new_ids: dict[int, int] = {}

    load_time: float = perf_counter()- time_start

    print(f"Load Time: {load_time:.3}s\n\nCommands:\n%p [PROV_TO_REMOVE_ID] [PROV_TO_MERGE_INTO_ID]\n%q to quit\n")
    while True:
        user_input: str = input("").lower()

        if user_input == "%q":
            break

        user_input_list: list[str] = user_input.split(" ")
        if len(user_input_list) == 3 and user_input_list[0] == "%p" and str.isnumeric(user_input_list[1]) and str.isnumeric(user_input_list[2]):
            MergeProvinces(
                provinces_list,
                states_list,
                strategic_regions_list,
                provinces_bitmap,
                int(user_input_list[1]),
                int(user_input_list[2]),
                changed_states,
                changed_strategic_regions,
                old_to_new_ids
            )

    WriteProvinceDefinitions(mod_directory, provinces_list)
    UpdateStateAndStrategicRegionFiles(states_list, strategic_regions_list, changed_states, changed_strategic_regions, old_to_new_ids)
    Image.fromarray(provinces_bitmap).save(str(mod_directory / "map/provinces.bmp"))

    print(f"Changed States:\n{changed_states}\n\nChanged Strategic Regions:\n{changed_strategic_regions}\nOld ID to new ID map:")
    for key, value in old_to_new_ids.items():
        print(f" - {key} - {value}")

if __name__ == "__main__":
    main()
