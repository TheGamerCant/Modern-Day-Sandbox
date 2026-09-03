from typing import Any
from pathlib import Path
import re
import os
from collections import defaultdict, Counter
import ast

import cv2
import numpy as np

MOD_DIRECTORY: Path = Path.cwd()
MOD_DIRECTORY = MOD_DIRECTORY.parents[1]

def load_file_to_string(filename: str) -> str:
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


def parse_clausewitz(text: str) -> dict[Any, Any]:
    tokens = re.findall(r'"[^"]*"|[{}=]|[^\s{}=]+', text)

    def add_value(obj, key, value):
        """Preserve duplicate keys as lists."""
        if key in obj:
            if not isinstance(obj[key], list):
                obj[key] = [obj[key]]
            obj[key].append(value)
        else:
            obj[key] = value

    def parse_value(i):
        token = tokens[i]

        # Parse block
        if token == "{":
            i += 1
            obj = {}
            arr = []

            has_keys = False

            while tokens[i] != "}":
                # key = value
                if i + 1 < len(tokens) and tokens[i + 1] == "=":
                    has_keys = True

                    key = tokens[i]
                    i += 2

                    value, i = parse_value(i)
                    add_value(obj, key, value)

                else:
                    value, i = parse_value(i)
                    arr.append(value)

            i += 1  # skip }

            return (obj if has_keys else arr), i

        # Quoted string
        elif token.startswith('"') and token.endswith('"'):
            return token[1:-1], i + 1

        # Integer
        elif re.fullmatch(r"-?\d+", token):
            return int(token), i + 1

        # Float
        elif re.fullmatch(r"-?\d+\.\d+", token):
            return float(token), i + 1

        # Bare identifier
        else:
            return token, i + 1

    result = {}
    i = 0

    # If the file doesn't end with a "}", account for it and pretend it does
    if len(tokens) > 1 and tokens.count("{") - tokens.count("}") == 1:
        tokens.append("}")

    while i < len(tokens):
        if i + 1 < len(tokens) and tokens[i + 1] == "=":
            key = tokens[i]
            i += 2

            value, i = parse_value(i)
            add_value(result, key, value)
        else:
            i += 1

    return result


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

def load_localisation() -> dict[str, str]:
    localisation_directory: str = str(MOD_DIRECTORY / "localisation/english")

    def format_line(line : str) -> str:
        return_line: str = line.strip()
        first_quote: int = return_line.find('"')
        last_quote: int = return_line.rfind('"')

        if first_quote == -1 or last_quote == -1:
            return return_line

        l_hash: int = return_line.find('#', 0, first_quote)
        if l_hash != -1:
            return return_line[:l_hash]

        r_hash: int = -1
        if last_quote + 1 != len(return_line):
            r_hash = return_line.find('#', last_quote + 1)

            if r_hash != -1:
                return return_line[:r_hash]

        return return_line

    pattern = re.compile(r'^\s*(\w+):\d*\s+"(.*)"')
    localisation: dict[str, str] = {}

    if not os.path.isdir(localisation_directory):
        return {}

    replace_folder_exists: bool = False

    loc_files: list[str] = []
    replace_loc_files: list[str] = []

    for subdir, dirs, files in os.walk(localisation_directory):
        current_dir: str = subdir.translate({92: 47})

        if current_dir == f"{localisation_directory}/replace":
            replace_folder_exists = True
            continue

        for file in files:
            full_path: str = f"{subdir}/{file}".translate({92: 47})     # '\\' -> '/'

            if not full_path.lower().endswith(".yml"):
                continue

            loc_files.append(full_path)

    for file in loc_files:
        with open(file, encoding="utf-8") as f:
            for line in f:
                line_formatted: str = format_line(line)

                match = pattern.match(line_formatted)
                if match:
                    localisation[match.group(1)] = match.group(2)

    if replace_folder_exists:
        replace_loc_files = [f"{localisation_directory}/replace/{file}" for file in os.listdir(f"{localisation_directory}/replace") if file.endswith(".yml")]

        for file in replace_loc_files:
            with open(file, encoding="utf-8") as f:
                for line in f:
                    line_formatted: str = format_line(line)

                    match = pattern.match(line_formatted)
                    if match:
                        localisation[match.group(1)] = match.group(2)


    return localisation

class Colour:
    def __init__(self, red: int, green: int, blue: int):
        self.red: int = red
        self.green: int = green
        self.blue: int = blue

    def to_tuple_rgb(self) -> tuple[int, int, int]:
        return self.red, self.green, self.blue

    def to_tuple_bgr(self) -> tuple[int, int, int]:
        return self.blue, self.green, self.red

class Region:
    def __init__(self, region_id: int, file_path: Path | None, provinces: list[int] | None):
        # Region ID
        self.region_id: int = region_id

        # File path
        self.file_path: Path | None = file_path

        # Provinces in the state as a set for fast checking
        self.provinces: set[int] | None = set(provinces) if isinstance(provinces, list) else None

        # States in the region as a set for fast checking
        self.states: set[int] = set()


class State:
    def __init__(self, state_id: int, file_path: Path | None, provinces: list[int] | None):
        # State ID
        self.state_id: int = state_id

        # File path
        self.file_path: Path | None = file_path

        # Provinces in the state as a set for fast checking
        self.provinces: set[int] | None = set(provinces) if isinstance(provinces, list) else None

        # The state's region
        self.region: int | None = None

class Province:
    def __init__(self, province_id: int, colour: Colour):
        # Province ID
        self.province_id: int = province_id

        # RGB colour
        self.colour: Colour = colour

        # State, loaded as a list so we can check all province/state
        # relationships after loading to give better error messages
        self.state: int | None = None
        self.states_list: list[int] = []

        # Region, loaded as a list so we can check all province/region
        # relationships after loading to give better error messages
        self.region: int | None = None
        self.regions_list: list[int] = []

def load_map() -> tuple[list[Province], list[State], list[Region]]:
    definition_file: Path = MOD_DIRECTORY / "map/definition.csv"
    state_files: list[Path] = list(Path(MOD_DIRECTORY / "history/states").glob("**/*.txt"))
    region_files: list[Path] = list(Path(MOD_DIRECTORY / "map/strategicregions").glob("**/*.txt"))

    total_map_files_count: int = 2 + len(state_files) + len(region_files)
    print_progress_bar(1, total_map_files_count, prefix = 'Loading Map Files:', suffix = 'Complete', length = 50)

    provinces_list: list[Province] = []
    states_list: list[State] = [State(0, None, None)]
    regions_list: list[Region] = [Region(0, None, None)]

    province_definitions: list[str] = load_file_to_string(str(definition_file)).split("\n")

    for i, definition in enumerate(province_definitions):
        if definition == "":
            continue

        entries: list[str] = definition.split(";")

        if len(entries) != 8:
            raise Exception(f"ERROR: Bad province definition in map/definition.csv line {i + 1}: {len(entries)} defined instead of expected 8.")
        if not all(str.isdigit(entries[i]) for i in range(4)):
            raise Exception(f"ERROR: Bad province definition in map/definition.csv line {i + 1}: First entry must be a digit.")

        provinces_list.append(
            Province(
                int(entries[0]),
                Colour(int(entries[1]), int(entries[2]), int(entries[3]))
            )
        )

    for i, state_file in enumerate(state_files):
        print_progress_bar(2 + i, total_map_files_count, prefix = 'Loading Map Files:', suffix = 'Complete', length = 50)
        state_dict: dict[Any, Any] = parse_clausewitz(load_file_to_string(str(state_file)))

        if not (state_data := state_dict.get("state")) or not isinstance(state_data, (list, dict)):
            raise Exception(f"ERROR: State file {state_file.relative_to(MOD_DIRECTORY)} does not contain a valid 'state' entry.")

        if isinstance(state_data, dict):
            states = [state_data]

        else:
            states = state_data

        for state in states:
            provinces = state.get("provinces")
            state_id = state.get("id")

            if provinces is None or not isinstance(provinces, list) or (len(provinces) > 0 and not isinstance(provinces[0], int)):
                raise Exception(f"ERROR: State file {state_file.relative_to(MOD_DIRECTORY)} does not contain a valid 'provinces' entry.")

            if state_id is None or not isinstance(state_id, int):
                raise Exception(f"ERROR: State file {state_file.relative_to(MOD_DIRECTORY)} does not contain a valid 'id' entry.")

            states_list.append(State(state_id, state_file, provinces))

    for i, region_file in enumerate(region_files):
        print_progress_bar(2 + len(state_files) + i, total_map_files_count, prefix = 'Loading Map Files:', suffix = 'Complete', length = 50)
        region_dict: dict[Any, Any] = parse_clausewitz(load_file_to_string(str(region_file)))

        if not (region_data := region_dict.get("strategic_region")) or not isinstance(region_data, (list, dict)):
            raise Exception(f"ERROR: Strategic Region file {region_file.relative_to(MOD_DIRECTORY)} does not contain a valid 'strategic_region' entry.")

        if isinstance(region_data, dict):
            regions = [region_data]

        else:
            regions = region_data

        for region in regions:
            provinces = region.get("provinces")
            region_id = region.get("id")

            if provinces is None or not isinstance(provinces, list) or (len(provinces) > 0 and not isinstance(provinces[0], int)):
                raise Exception(f"ERROR: Strategic Region file {region_file.relative_to(MOD_DIRECTORY)} does not contain a valid 'provinces' entry.")

            if region_id is None or not isinstance(region_id, int):
                raise Exception(f"ERROR: Strategic Region file {region_file.relative_to(MOD_DIRECTORY)} does not contain a valid 'id' entry.")

            regions_list.append(Region(region_id, region_file, provinces))

    # Sort so that states_list[N]/regions_list[N] is directly the state/
    # region whose id is N (verify_map checks this holds - ids must be
    # contiguous from 0 with no gaps for this to be safe)
    states_list.sort(key=lambda state: state.state_id)
    regions_list.sort(key=lambda region: region.region_id)

    return provinces_list, states_list, regions_list


def verify_map(provinces_list: list[Province], states_list: list[State], regions_list: list[Region]) -> list[str]:
    errors: list[str] = []

    # Verify all non-zero provinces have pixels in provinces.bmp
    provinces_file_path = MOD_DIRECTORY / "map/provinces.bmp"
    provinces_bgr: np.ndarray = cv2.imread(str(provinces_file_path))

    if provinces_bgr is None:
        raise FileNotFoundError("Could not open map/provinces.bmp")

    bgr_set: set[tuple] = set(map(tuple, provinces_bgr.reshape(-1, provinces_bgr.shape[-1])))

    print_progress_bar(1, 1, prefix = 'Loading Map Files:', suffix = 'Complete', length = 50)

    provinces_without_pixels = [province for province in provinces_list[1:] if province.colour.to_tuple_bgr() not in bgr_set]
    if provinces_without_pixels:
        for prov in provinces_without_pixels:
            errors.append(f"Province {prov.province_id} does not have any pixels in map/provinces.bmp")

    # Verify all provinces are in order
    previous_province: int = -1
    provinces_equal_enumeration = True

    for i, province in enumerate(provinces_list):
        if province.province_id != previous_province + 1:
            errors.append(f"Province {province.province_id} is defined directly after {previous_province} in map/definition.csv, every province ID should be one more than the previous one.")

        if province.province_id != i:
            provinces_equal_enumeration = False

        previous_province = province.province_id

    if not provinces_equal_enumeration:
        errors.append("Provinces in map/definition.csv must start from zero and go up by +1 for each line.")

    if len(errors) > 0:
        return errors

    # Verify states_list/regions_list are continuous
    for i, state in enumerate(states_list):
        if state.state_id != i:
            errors.append(f"State list index {i} holds state {state.state_id} instead - state ids must be contiguous from 0 with no gaps.")

    for i, region in enumerate(regions_list):
        if region.region_id != i:
            errors.append(f"Region list index {i} holds region {region.region_id} instead - region ids must be contiguous from 0 with no gaps.")

    if len(errors) > 0:
        return errors

    # Verify all state/region provinces are valid
    for state in states_list:
        if state.state_id == 0:
            continue

        for province_id in state.provinces:
            if 0 <= province_id < len(provinces_list):
                provinces_list[province_id].states_list.append(state.state_id)
            else:
                errors.append(f"Invalid province ({province_id}) defined in {state.file_path.relative_to(MOD_DIRECTORY)}")

    for region in regions_list:
        if region.region_id == 0:
            continue

        for province_id in region.provinces:
            if 0 <= province_id < len(provinces_list):
                provinces_list[province_id].regions_list.append(region.region_id)
            else:
                errors.append(f"Invalid province ({province_id}) defined in {region.file_path.relative_to(MOD_DIRECTORY)}")


    if len(errors) > 0:
        return errors

    # Verify the relationship between provinces/states/regions is valid
    for province in provinces_list:
        if len(province.states_list) > 1:
            errors.append(
                f"Province {province.province_id} belongs to {len(province.states_list)} states ({', '.join([str(s) for s in province.states_list])})"
            )
        elif len(province.states_list) == 1:
            province.state = province.states_list[0]

        if len(province.regions_list) > 1:
            errors.append(
                f"Province {province.province_id} belongs to {len(province.regions_list)} strategic regions ({', '.join([str(r) for r in province.regions_list])})"
            )
        elif len(province.regions_list) == 1:
            province.region = province.regions_list[0]

    if len(errors) > 0:
        return errors

    # Verify every state has one strategic region
    for state in states_list:
        if state.state_id == 0:
            continue

        regions: set[int] = set([provinces_list[prov].region for prov in state.provinces])


        if len(regions) > 1:
            state_provs_to_regions: dict[int, int] = {prov: provinces_list[prov].region for prov in state.provinces}
            most_common_region, count = Counter(state_provs_to_regions.values()).most_common(1)[0]

            erroneous_provs = {prov for prov, region in state_provs_to_regions.items() if region != most_common_region}

            errors.append(
                f"State {state.state_id} has provinces in multiple strategic regions ({list(regions)})\nnt - Erroneous provinces: {erroneous_provs}"
            )
        else:
            state.region = list(regions)[0]
            regions_list[state.region].states.add(state.state_id)

    return errors

COMMANDS_STRING: str = (
    "Commands (case insensitive):\n"
    "\n"
    "s_move provinces=[] into=*int* - Moves the listed provinces into the state\n"
    "r_move states=[] into=[] region=*int* - Moves the listed states (all their provinces) and the listed provinces into an existing strategic region. Either list may be empty\n"
    "\n"
    "s_create provinces=[] - Creates a state with the listed provinces. If containing provinces in multiple regions, the region of the first province will be chosen\n"
    "r_create states=[] provinces=[] - Creates a new strategic region containing the listed states' provinces plus the listed provinces. Either list may be empty\n"
    "\n"
    "p_merge provinces=[] into=*int* - Merges the listed provinces into a chosen province, deleting all provinces in the list\n"
    "s_merge states=[] into=*int* - Merges the listed states into a chosen state, deleting all states in the list\n"
    "r_merge regions=[] into=*int* - Merges the listed strategic regions into a chosen region, deleting all regions in the list\n"
    "\n"
    "edit - Allows you to edit move operations. Use arrow keys to move up/down and enter to delete an operation or leave the queue\n"
    "run - Runs all operations in the queue and closes the program\n"
    "quit - Exits to program without running the queue\n"
)

COMMAND_SCHEMAS: dict[str, set[str]] = {
    "s_move": {"provinces", "into"},
    "p_merge": {"provinces", "into"},
    "s_merge": {"states", "into"},
    "s_create": {"provinces"},
    "r_move": {"states", "provinces", "into"},
    "r_create": {"states", "provinces"},
    "r_merge": {"regions", "into"},
    "edit": {},
    "run": {},
    "quit": {}
}

class MoveOperation:
    def __init__(self, province_id: int, state_id: int):
        self.province_id: int = province_id
        self.state_id: int = state_id

    def __str__(self) -> str:
        return f"Move province {self.province_id} to state {self.state_id}"

class MergeProvincesOperation:
    def __init__(self, province_to_delete_id: int, province_to_merge_into: int):
        self.province_to_delete_id: int = province_to_delete_id
        self.province_to_merge_into: int = province_to_merge_into

    def __str__(self) -> str:
        return f"Merge province {self.province_to_delete_id} into province {self.province_to_merge_into}"

class MergeStatesOperation:
    def __init__(self, state_to_delete_id: int, state_to_merge_into: int):
        self.state_to_delete_id: int = state_to_delete_id
        self.state_to_merge_into: int = state_to_merge_into

    def __str__(self) -> str:
        return f"Merge state {self.state_to_delete_id} into state {self.state_to_merge_into}"

class CreateStateOperation:
    def __init__(self, province_id: int, state_id: int):
        self.province_id: int = province_id
        self.state_id: int = state_id

    def __str__(self) -> str:
        return f"Create a new state (ID={self.state_id}) containing the province {self.province_id}"

class MoveProvinceRegionOperation:
    def __init__(self, province_id: int, region_id: int):
        self.province_id: int = province_id
        self.region_id: int = region_id

    def __str__(self) -> str:
        return f"Move province {self.province_id} to strategic region {self.region_id}"

class MoveStateRegionOperation:
    def __init__(self, state_id: int, region_id: int):
        self.state_id: int = state_id
        self.region_id: int = region_id

    def __str__(self) -> str:
        return f"Move state {self.state_id} (all its provinces) to strategic region {self.region_id}"

class CreateRegionOperation:
    def __init__(self, region_id: int):
        # Informational only - the planner assigns the real id from the live
        # region count at the point this operation is reached
        self.region_id: int = region_id

    def __str__(self) -> str:
        return f"Create a new strategic region (ID={self.region_id})"

class MergeRegionsOperation:
    def __init__(self, region_to_delete_id: int, region_to_merge_into: int):
        self.region_to_delete_id: int = region_to_delete_id
        self.region_to_merge_into: int = region_to_merge_into

    def __str__(self) -> str:
        return f"Merge strategic region {self.region_to_delete_id} into strategic region {self.region_to_merge_into}"


def get_key() -> str:
    try:
        import msvcrt

        ch = msvcrt.getch()

        if ch in (b"\x00", b"\xe0"):
            ch2 = msvcrt.getch()

            if ch2 == b"H":
                return "UP"
            if ch2 == b"P":
                return "DOWN"

            return ""

        if ch in (b"\r", b"\n"):
            return "ENTER"
        if ch == b"\x03":
            raise KeyboardInterrupt

        return ch.decode(errors="ignore")

    except ImportError:
        import sys
        import tty
        import termios

        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)

        try:
            tty.setraw(fd)
            ch = sys.stdin.read(1)

            if ch == "\x1b":
                ch2 = sys.stdin.read(2)

                if ch2 == "[A":
                    return "UP"
                if ch2 == "[B":
                    return "DOWN"

                return ""

            if ch in ("\r", "\n"):
                return "ENTER"
            if ch == "\x03":
                raise KeyboardInterrupt

            return ch

        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)


def edit_move_queue(move_queue: list[MoveOperation]) -> None:
    selected_index = 0

    while True:
        options: list[str] = [str(operation) for operation in move_queue] + ["[Leave]"]

        os.system('cls' if os.name == 'nt' else 'clear')
        print("Editing move queue. Use Up/Down to navigate, Enter to delete an operation or leave.\n")

        for i, option in enumerate(options):
            prefix = "> " if i == selected_index else "  "
            print(f"{prefix}{option}")

        key = get_key()

        if key == "UP":
            selected_index = (selected_index - 1) % len(options)

        elif key == "DOWN":
            selected_index = (selected_index + 1) % len(options)

        elif key == "ENTER":
            if selected_index == len(move_queue):
                break

            del move_queue[selected_index]

            if selected_index >= len(move_queue) and selected_index > 0:
                selected_index -= 1

    os.system('cls' if os.name == 'nt' else 'clear')

def find_top_level_blocks(text: str, keyword: str) -> list[tuple[int, int]]:
    """
    Find every `keyword = { ... }` block in text (e.g. every "state = {"
    or "provinces = {" block), returning the (start, end) character
    offsets of each block - start is the index of the opening "{", end
    is one past the index of its matching closing "}".
    """
    spans: list[tuple[int, int]] = []

    for match in re.finditer(rf"\b{re.escape(keyword)}\s*=\s*{{", text):
        start = match.end() - 1  # index of the opening "{"
        depth = 0
        in_single_quotes = False
        in_double_quotes = False
        in_comment = False
        i = start

        while i < len(text):
            c = text[i]

            if c == "\n":
                in_comment = False

            elif in_comment:
                pass

            elif c == "#" and not in_single_quotes and not in_double_quotes:
                # Skip the rest of the line - a "{" or "}" in a comment
                # must not be counted towards brace depth
                in_comment = True

            elif c == "'" and not in_double_quotes and (i == 0 or text[i - 1] != "\\"):
                in_single_quotes = not in_single_quotes

            elif c == '"' and not in_single_quotes and (i == 0 or text[i - 1] != "\\"):
                in_double_quotes = not in_double_quotes

            elif not in_single_quotes and not in_double_quotes:
                if c == "{":
                    depth += 1
                elif c == "}":
                    depth -= 1
                    if depth == 0:
                        spans.append((start, i + 1))
                        break

            i += 1

    return spans


class BlockEdit:
    def __init__(self, delete: bool = False, new_id: int | None = None, new_provinces: set[int] | None = None):
        self.delete: bool = delete
        self.new_id: int | None = new_id
        self.new_provinces: set[int] | None = new_provinces


# Maps a block keyword to the prefix HOI4 auto-generates its localisation
# key with (e.g. a "state" block's default `name` is `STATE_<id>`).
LOC_KEY_PREFIXES: dict[str, str] = {
    "state": "STATE",
    "strategic_region": "STRATEGICREGION",
}


def apply_block_edits(text: str, keyword: str, edits_by_original_id: dict[int, BlockEdit]) -> str:
    """
    Apply queued edits to every `keyword = { ... }` block in text, matched
    by each block's own (as currently on disk) "id" field. An edit can
    delete the whole block, rewrite its id, rewrite its provinces list, or
    both - the rest of the file (comments, other fields, other blocks) is
    left as-is.
    """
    replacements: list[tuple[int, int, str | None]] = []

    for start, end in find_top_level_blocks(text, keyword):
        block_text = text[start:end]

        id_match = re.search(r"\bid\s*=\s*(-?\d+)", block_text)
        if id_match is None:
            continue

        original_id = int(id_match.group(1))
        edit = edits_by_original_id.get(original_id)
        if edit is None:
            continue

        if edit.delete:
            replacements.append((start, end, None))
            continue

        new_block_text = block_text

        if edit.new_provinces is not None:
            provinces_spans = find_top_level_blocks(new_block_text, "provinces")
            if provinces_spans:
                p_start, p_end = provinces_spans[0]
                new_provinces_text = "{ " + " ".join(str(p) for p in sorted(edit.new_provinces)) + " }"
                new_block_text = new_block_text[:p_start] + new_provinces_text + new_block_text[p_end:]

        if edit.new_id is not None:
            new_id_match = re.search(r"\bid\s*=\s*(-?\d+)", new_block_text)
            new_block_text = new_block_text[:new_id_match.start(1)] + str(edit.new_id) + new_block_text[new_id_match.end(1):]

            # Keep an auto-generated localisation key (e.g. name = "STATE_1234")
            # in sync with the id it's derived from. A custom name that
            # doesn't match "<PREFIX>_<original id>" exactly is left alone.
            name_prefix = LOC_KEY_PREFIXES.get(keyword)
            if name_prefix is not None:
                name_pattern = re.compile(rf'(\bname\s*=\s*"{name_prefix}_){original_id}(")')
                new_block_text = name_pattern.sub(
                    lambda m: f"{m.group(1)}{edit.new_id}{m.group(2)}",
                    new_block_text,
                    count=1,
                )

        replacements.append((start, end, new_block_text))

    # Apply from the end of the file backwards so earlier offsets stay valid
    for start, end, new_block_text in sorted(replacements, key=lambda r: r[0], reverse=True):
        if new_block_text is None:
            # Delete the block, and swallow one trailing newline so we don't
            # leave a double-blank-line where it used to be
            deletion_end = end + 1 if end < len(text) and text[end] == "\n" else end
            text = text[:start] + text[deletion_end:]
        else:
            text = text[:start] + new_block_text + text[end:]

    return text


def rename_file_for_id_change(file_path: Path, edits: dict[int, BlockEdit]) -> Path:
    """
    A merge can leave a surviving state/region file's own id changed (e.g.
    the highest-numbered state/region gets renumbered into the slot freed up
    by the one that got merged away) without the file on disk being touched
    otherwise. If exactly one surviving block in this file had its id
    changed, and the file's name still follows the usual "<id>-<name>.txt"
    convention with that block's *old* id, rename the file on disk so its
    name reflects the new id too. Returns the (possibly renamed) path.
    """
    id_changes = {
        original_id: edit.new_id
        for original_id, edit in edits.items()
        if not edit.delete and edit.new_id is not None
    }

    if len(id_changes) != 1:
        return file_path

    ((old_id, new_id),) = id_changes.items()

    name_match = re.match(rf"^{old_id}-(.+)$", file_path.name)
    if not name_match:
        return file_path

    new_file_path = file_path.with_name(f"{new_id}-{name_match.group(1)}")

    if new_file_path.exists():
        return file_path  # avoid clobbering an existing file with the same name

    file_path.rename(new_file_path)
    return new_file_path


class PlanningError(Exception):
    pass


class ProvincePlan:
    def __init__(self, row: str, state_id: int | None, region_id: int | None):
        # The full raw map/definition.csv line for this province - kept as
        # text (rather than parsed) so unrelated columns (terrain, coastal,
        # continent, ...) survive untouched even when a province's id changes
        self.row: str = row
        self.state_id: int | None = state_id
        self.region_id: int | None = region_id


class StatePlan:
    def __init__(self, file_path: Path | None, region_id: int | None, provinces: set[int], original_id: int | None, is_new: bool = False):
        self.file_path: Path | None = file_path
        self.region_id: int | None = region_id
        self.provinces: set[int] = provinces

        # Snapshot of the province set as originally loaded, so we can
        # detect "nothing actually changed for this state" and skip writing it
        self.original_provinces: frozenset[int] = frozenset(provinces)

        # The id this state is currently written under in file_path (None
        # for a brand-new state that doesn't exist in any file yet)
        self.original_id: int | None = original_id

        self.is_new: bool = is_new


class RegionPlan:
    def __init__(self, file_path: Path | None, provinces: set[int], original_id: int | None, is_new: bool = False):
        self.file_path: Path | None = file_path
        self.provinces: set[int] = provinces
        self.original_provinces: frozenset[int] = frozenset(provinces)

        # The id this region is currently written under in file_path (None
        # for a brand-new region that doesn't exist in any file yet)
        self.original_id: int | None = original_id

        self.is_new: bool = is_new


def parse_definition_row_colour(row: str) -> tuple[int, int, int]:
    parts = row.split(";")
    return int(parts[1]), int(parts[2]), int(parts[3])


def set_definition_row_id(row: str, new_id: int) -> str:
    _, _, rest = row.partition(";")
    return f"{new_id};{rest}"


def resolve_colour(
    colour_remap: dict[tuple[int, int, int], tuple[int, int, int]],
    colour: tuple[int, int, int],
) -> tuple[int, int, int]:
    """Follow a chain of merges (e.g. A merged into B, B later merged into C) to the final colour."""
    seen: set[tuple[int, int, int]] = set()

    while colour in colour_remap and colour not in seen:
        seen.add(colour)
        colour = colour_remap[colour]

    return colour


def build_execution_plan(
    provinces_list: list[Province],
    states_list: list[State],
    regions_list: list[Region],
    commands_queue: list,
) -> tuple[list[ProvincePlan], list[StatePlan], list[RegionPlan], dict[tuple[int, int, int], tuple[int, int, int]], list[tuple[Path, int]], list[tuple[Path, int]]]:
    """
    Replay commands_queue in order against an in-memory copy of the loaded
    map, resolving chains/conflicts as it goes (e.g. a province moved twice,
    or merged and then moved), and validating every operation against the
    *live* numbering at the point it's reached - not the numbering that was
    true when the operation was originally typed. Deleting a province/state
    swaps it with the current last one and shrinks the list by one (rather
    than shifting every higher id down), so ids stay contiguous; be aware
    that later operations in the same run must refer to the post-swap ids.

    Raises PlanningError (aborting before anything is written) if any
    operation doesn't make sense given the live numbering, or if the queue
    would leave a state with zero provinces.
    """
    definition_file: Path = MOD_DIRECTORY / "map/definition.csv"
    definition_rows: list[str] = [row for row in load_file_to_string(str(definition_file)).split("\n") if row != ""]

    province_plans: list[ProvincePlan] = [
        ProvincePlan(definition_rows[p.province_id], p.state, p.region)
        for p in provinces_list
    ]

    state_plans: list[StatePlan] = [
        StatePlan(s.file_path, s.region, set(s.provinces) if s.provinces else set(), original_id=s.state_id)
        for s in states_list
    ]

    region_plans: list[RegionPlan] = [
        RegionPlan(r.file_path, set(r.provinces) if r.provinces else set(), original_id=r.region_id)
        for r in regions_list
    ]

    colour_remap: dict[tuple[int, int, int], tuple[int, int, int]] = {}
    blocks_to_delete: list[tuple[Path, int]] = []
    region_blocks_to_delete: list[tuple[Path, int]] = []
    errors: list[str] = []

    def valid_province(pid: int) -> bool:
        return 0 < pid < len(province_plans)

    def valid_state(sid: int) -> bool:
        return 0 < sid < len(state_plans)

    def valid_region(rid: int) -> bool:
        return 0 < rid < len(region_plans)

    def move_province_region(pid: int, new_region_id: int) -> None:
        province = province_plans[pid]

        if province.region_id != new_region_id:
            if province.region_id is not None:
                region_plans[province.region_id].provinces.discard(pid)
            region_plans[new_region_id].provinces.add(pid)
            province.region_id = new_region_id

    def move_state_region(sid: int, new_region_id: int) -> None:
        for pid in state_plans[sid].provinces:
            move_province_region(pid, new_region_id)

        state_plans[sid].region_id = new_region_id

    def move_province_state(pid: int, new_state_id: int) -> None:
        province = province_plans[pid]

        if province.state_id != new_state_id:
            if province.state_id is not None:
                state_plans[province.state_id].provinces.discard(pid)
            state_plans[new_state_id].provinces.add(pid)
            province.state_id = new_state_id

        new_region_id = state_plans[new_state_id].region_id
        if province.region_id != new_region_id:
            if province.region_id is not None:
                region_plans[province.region_id].provinces.discard(pid)
            if new_region_id is not None:
                region_plans[new_region_id].provinces.add(pid)
            province.region_id = new_region_id

    def delete_province(pid: int) -> None:
        province = province_plans[pid]

        if province.state_id is not None:
            state_plans[province.state_id].provinces.discard(pid)
        if province.region_id is not None:
            region_plans[province.region_id].provinces.discard(pid)

        last_index = len(province_plans) - 1

        if pid != last_index:
            last = province_plans.pop()

            if last.state_id is not None:
                state_plans[last.state_id].provinces.discard(last_index)
                state_plans[last.state_id].provinces.add(pid)
            if last.region_id is not None:
                region_plans[last.region_id].provinces.discard(last_index)
                region_plans[last.region_id].provinces.add(pid)

            last.row = set_definition_row_id(last.row, pid)
            province_plans[pid] = last
        else:
            province_plans.pop()

    def delete_region(rid: int) -> None:
        deleted = region_plans[rid]

        if deleted.file_path is not None and deleted.original_id is not None:
            region_blocks_to_delete.append((deleted.file_path, deleted.original_id))

        last_index = len(region_plans) - 1

        if rid != last_index:
            last = region_plans.pop()

            for pid in last.provinces:
                province_plans[pid].region_id = rid

            for state in state_plans:
                if state.region_id == last_index:
                    state.region_id = rid

            region_plans[rid] = last
        else:
            region_plans.pop()

    def delete_state(sid: int) -> None:
        deleted = state_plans[sid]

        if deleted.file_path is not None and deleted.original_id is not None:
            blocks_to_delete.append((deleted.file_path, deleted.original_id))

        last_index = len(state_plans) - 1

        if sid != last_index:
            last = state_plans.pop()

            for pid in last.provinces:
                province_plans[pid].state_id = sid

            state_plans[sid] = last
        else:
            state_plans.pop()

    for command in commands_queue:
        if isinstance(command, MoveOperation):
            if not valid_province(command.province_id):
                errors.append(f"move: province {command.province_id} does not exist")
                continue
            if not valid_state(command.state_id):
                errors.append(f"move: state {command.state_id} does not exist")
                continue

            move_province_state(command.province_id, command.state_id)

        elif isinstance(command, MergeProvincesOperation):
            delete_id, merge_into_id = command.province_to_delete_id, command.province_to_merge_into

            if not valid_province(delete_id):
                errors.append(f"p_merge: province {delete_id} does not exist")
                continue
            if not valid_province(merge_into_id):
                errors.append(f"p_merge: province {merge_into_id} does not exist")
                continue
            if delete_id == merge_into_id:
                errors.append(f"p_merge: cannot merge province {delete_id} into itself")
                continue

            old_colour = parse_definition_row_colour(province_plans[delete_id].row)
            new_colour = parse_definition_row_colour(province_plans[merge_into_id].row)
            colour_remap[old_colour] = new_colour

            delete_province(delete_id)

        elif isinstance(command, MergeStatesOperation):
            delete_id, merge_into_id = command.state_to_delete_id, command.state_to_merge_into

            if not valid_state(delete_id):
                errors.append(f"s_merge: state {delete_id} does not exist")
                continue
            if not valid_state(merge_into_id):
                errors.append(f"s_merge: state {merge_into_id} does not exist")
                continue
            if delete_id == merge_into_id:
                errors.append(f"s_merge: cannot merge state {delete_id} into itself")
                continue

            for pid in list(state_plans[delete_id].provinces):
                move_province_state(pid, merge_into_id)

            delete_state(delete_id)

        elif isinstance(command, CreateStateOperation):
            province_id = command.province_id

            if not valid_province(province_id):
                errors.append(f"create: province {province_id} does not exist")
                continue

            new_state_id = len(state_plans)
            state_plans.append(StatePlan(None, province_plans[province_id].region_id, set(), original_id=None, is_new=True))

            move_province_state(province_id, new_state_id)

        elif isinstance(command, MoveProvinceRegionOperation):
            if not valid_province(command.province_id):
                errors.append(f"r_move: province {command.province_id} does not exist")
                continue
            if not valid_region(command.region_id):
                errors.append(f"r_move: strategic region {command.region_id} does not exist")
                continue

            move_province_region(command.province_id, command.region_id)

        elif isinstance(command, MoveStateRegionOperation):
            if not valid_state(command.state_id):
                errors.append(f"r_move: state {command.state_id} does not exist")
                continue
            if not valid_region(command.region_id):
                errors.append(f"r_move: strategic region {command.region_id} does not exist")
                continue

            move_state_region(command.state_id, command.region_id)

        elif isinstance(command, CreateRegionOperation):
            region_plans.append(RegionPlan(None, set(), original_id=None, is_new=True))

        elif isinstance(command, MergeRegionsOperation):
            delete_id, merge_into_id = command.region_to_delete_id, command.region_to_merge_into

            if not valid_region(delete_id):
                errors.append(f"r_merge: strategic region {delete_id} does not exist")
                continue
            if not valid_region(merge_into_id):
                errors.append(f"r_merge: strategic region {merge_into_id} does not exist")
                continue
            if delete_id == merge_into_id:
                errors.append(f"r_merge: cannot merge strategic region {delete_id} into itself")
                continue

            for pid in list(region_plans[delete_id].provinces):
                move_province_region(pid, merge_into_id)

            for state in state_plans:
                if state.region_id == delete_id:
                    state.region_id = merge_into_id

            delete_region(delete_id)

        else:
            errors.append(f"Unknown operation: {command!r}")

    empty_state_ids = [sid for sid in range(1, len(state_plans)) if len(state_plans[sid].provinces) == 0]
    if empty_state_ids:
        errors.append(
            f"The following state id(s) would be left with no provinces after these operations: {empty_state_ids}. "
            "Queue an s_merge for them (or avoid emptying them) before running."
        )

    # A state whose provinces end up split across strategic regions would be
    # rejected by verify_map on the next load - refuse to write it in the
    # first place (this typically means an r_move listed individual land
    # provinces instead of their whole state)
    for sid in range(1, len(state_plans)):
        state_regions = {province_plans[pid].region_id for pid in state_plans[sid].provinces}

        if len(state_regions) > 1:
            errors.append(
                f"These operations would leave state {sid} with provinces in multiple strategic regions "
                f"({', '.join(str(r) for r in sorted(state_regions, key=lambda r: (r is None, r)))}). "
                f"Move the whole state (r_move states=[{sid}] ...) instead of individual land provinces."
            )

    if errors:
        raise PlanningError("\n".join(errors))

    return province_plans, state_plans, region_plans, colour_remap, blocks_to_delete, region_blocks_to_delete


def write_state_files(
    state_plans: list[StatePlan],
    blocks_to_delete: list[tuple[Path, int]],
) -> tuple[list[Path], list[Path], list[int], list[int]]:
    """
    Write every queued state file edit, deletion, and creation.
    Returns (edited files, fully-deleted files, created state ids, all touched state ids).
    """
    edits_by_file: dict[Path, dict[int, BlockEdit]] = defaultdict(dict)

    for file_path, original_id in blocks_to_delete:
        edits_by_file[file_path][original_id] = BlockEdit(delete=True)

    new_states: list[tuple[int, StatePlan]] = []
    touched_ids: list[int] = []

    for final_id, state in enumerate(state_plans):
        if final_id == 0:
            continue

        if state.is_new:
            new_states.append((final_id, state))
            touched_ids.append(final_id)
            continue

        if state.original_id == final_id and state.provinces == state.original_provinces:
            continue  # nothing changed for this state - don't touch its file

        edits_by_file[state.file_path][state.original_id] = BlockEdit(
            new_id=final_id if final_id != state.original_id else None,
            new_provinces=set(state.provinces),
        )
        touched_ids.append(final_id)

    edited_files: list[Path] = []
    deleted_files: list[Path] = []

    for file_path, edits in edits_by_file.items():
        with open(file_path, "r", encoding="utf-8") as file:
            text = file.read()

        new_text = apply_block_edits(text, "state", edits)

        if not find_top_level_blocks(new_text, "state"):
            file_path.unlink()
            deleted_files.append(file_path)
        else:
            with open(file_path, "w", encoding="utf-8") as file:
                file.write(new_text)

            final_path = rename_file_for_id_change(file_path, edits)
            edited_files.append(final_path)

            for state in state_plans:
                if state.file_path == file_path and state.original_id in edits:
                    state.file_path = final_path

    created_state_ids: list[int] = []

    for final_id, state in new_states:
        new_file_path = MOD_DIRECTORY / "history/states" / f"{final_id}-New_State.txt"
        provinces_text = " ".join(str(p) for p in sorted(state.provinces))

        content = (
            "state = {\n"
            f"    id = {final_id}\n"
            f'    name = "STATE_{final_id}"\n'
            "    manpower = 10000\n"
            "    state_category = rural\n\n"
            "    provinces = {\n"
            f"        {provinces_text}\n"
            "    }\n"
            "}\n"
        )

        with open(new_file_path, "w", encoding="utf-8") as file:
            file.write(content)

        state.file_path = new_file_path
        created_state_ids.append(final_id)

    return edited_files, deleted_files, created_state_ids, touched_ids


def write_region_files(
    region_plans: list[RegionPlan],
    region_blocks_to_delete: list[tuple[Path, int]],
) -> tuple[list[Path], list[Path], list[int]]:
    """
    Write every queued strategic region file edit, deletion, and creation.
    Returns (edited files, fully-deleted files, created region ids).
    """
    edits_by_file: dict[Path, dict[int, BlockEdit]] = defaultdict(dict)

    for file_path, original_id in region_blocks_to_delete:
        edits_by_file[file_path][original_id] = BlockEdit(delete=True)

    new_regions: list[tuple[int, RegionPlan]] = []

    for final_id, region in enumerate(region_plans):
        if final_id == 0:
            continue

        if region.is_new:
            new_regions.append((final_id, region))
            continue

        if region.original_id == final_id and region.provinces == region.original_provinces:
            continue  # nothing changed for this region - don't touch its file

        edits_by_file[region.file_path][region.original_id] = BlockEdit(
            new_id=final_id if final_id != region.original_id else None,
            new_provinces=set(region.provinces),
        )

    edited_files: list[Path] = []
    deleted_files: list[Path] = []

    for file_path, edits in edits_by_file.items():
        with open(file_path, "r", encoding="utf-8") as file:
            text = file.read()

        new_text = apply_block_edits(text, "strategic_region", edits)

        if not find_top_level_blocks(new_text, "strategic_region"):
            file_path.unlink()
            deleted_files.append(file_path)
        else:
            with open(file_path, "w", encoding="utf-8") as file:
                file.write(new_text)

            final_path = rename_file_for_id_change(file_path, edits)
            edited_files.append(final_path)

            for region in region_plans:
                if region.file_path == file_path and region.original_id in edits:
                    region.file_path = final_path

    created_region_ids: list[int] = []

    for final_id, region in new_regions:
        new_file_path = MOD_DIRECTORY / "map/strategicregions" / f"{final_id}-New_Region.txt"
        provinces_text = " ".join(str(p) for p in sorted(region.provinces))

        # A single all-year weather period so the file is valid in-game -
        # tune it (or split it into monthly periods) by hand afterwards
        content = (
            "strategic_region = {\n"
            f"\tid = {final_id}\n"
            f"\tname = \"STRATEGICREGION_{final_id}\"\n"
            "\tprovinces = {\n"
            f"\t\t{provinces_text}\n"
            "\t}\n"
            "\tweather = {\n"
            "\t\tperiod = {\n"
            "\t\t\tbetween = { 0.0 30.11 }\n"
            "\t\t\ttemperature = { -6.0 12.0 }\n"
            "\t\t\tno_phenomenon = 0.500\n"
            "\t\t\train_light = 0.250\n"
            "\t\t\train_heavy = 0.100\n"
            "\t\t\tsnow = 0.100\n"
            "\t\t\tblizzard = 0.000\n"
            "\t\t\tarctic_water = 0.000\n"
            "\t\t\tmud = 0.050\n"
            "\t\t\tsandstorm = 0.000\n"
            "\t\t\tmin_snow_level = 0.000\n"
            "\t\t}\n"
            "\t}\n"
            "}\n"
        )

        with open(new_file_path, "w", encoding="utf-8") as file:
            file.write(content)

        region.file_path = new_file_path
        created_region_ids.append(final_id)

    return edited_files, deleted_files, created_region_ids


def write_definition_csv(province_plans: list[ProvincePlan]) -> None:
    definition_file: Path = MOD_DIRECTORY / "map/definition.csv"
    content = "\n".join(province.row for province in province_plans) + "\n"

    with open(definition_file, "w", encoding="utf-8") as file:
        file.write(content)


def write_provinces_bmp(colour_remap: dict[tuple[int, int, int], tuple[int, int, int]]) -> None:
    final_remap = {old: resolve_colour(colour_remap, old) for old in colour_remap}
    final_remap = {old: new for old, new in final_remap.items() if old != new}

    if not final_remap:
        return

    provinces_file_path: Path = MOD_DIRECTORY / "map/provinces.bmp"
    image: np.ndarray = cv2.imread(str(provinces_file_path))

    if image is None:
        raise FileNotFoundError("Could not open map/provinces.bmp")

    for (old_r, old_g, old_b), (new_r, new_g, new_b) in final_remap.items():
        mask = (
            (image[:, :, 0] == old_b) &
            (image[:, :, 1] == old_g) &
            (image[:, :, 2] == old_r)
        )
        image[mask] = (new_b, new_g, new_r)

    cv2.imwrite(str(provinces_file_path), image)


def execute_move_commands(
        provinces_list: list[Province],
        states_list: list[State],
        regions_list: list[Region],
        commands_queue: list[MoveOperation | MergeProvincesOperation | MergeStatesOperation | CreateStateOperation | MoveProvinceRegionOperation | MoveStateRegionOperation | CreateRegionOperation | MergeRegionsOperation],
        localisation: dict[str, str]
    ):
    if len(commands_queue) == 0:
        return

    try:
        province_plans, state_plans, region_plans, colour_remap, blocks_to_delete, region_blocks_to_delete = build_execution_plan(
            provinces_list, states_list, regions_list, commands_queue
        )
    except PlanningError as error:
        print("\n\nCannot run the queued operations:\n")
        print(str(error))
        return

    any_province_merge = any(isinstance(command, MergeProvincesOperation) for command in commands_queue)

    edited_state_files, deleted_state_files, created_state_ids, touched_state_ids = write_state_files(state_plans, blocks_to_delete)
    edited_region_files, deleted_region_files, created_region_ids = write_region_files(region_plans, region_blocks_to_delete)

    if any_province_merge:
        write_definition_csv(province_plans)
        write_provinces_bmp(colour_remap)

    # Print results to terminal
    print("\n\nFinished executing queued operations.\n")

    if created_state_ids:
        print(f"Created new state(s): {created_state_ids}")

    if created_region_ids:
        print(f"Created new strategic region(s): {created_region_ids}")

    if touched_state_ids:
        print("\nChanged state files:\n")
        for state_id in sorted(touched_state_ids):
            print(f"{state_id} ({localisation.get(f'STATE_{state_id}', f'STATE_{state_id}')})")

    if deleted_state_files:
        print("\nRemoved (now-empty) state file(s):")
        for file_path in deleted_state_files:
            print(f" - {file_path.relative_to(MOD_DIRECTORY)}")

    if edited_region_files:
        print("\nChanged strategic region files:\n")
        for file_path in edited_region_files:
            print(f"{file_path.relative_to(MOD_DIRECTORY)}")

    if deleted_region_files:
        print("\nRemoved (now-empty) strategic region file(s):")
        for file_path in deleted_region_files:
            print(f" - {file_path.relative_to(MOD_DIRECTORY)}")

    if any_province_merge:
        print("\nUpdated map/definition.csv and map/provinces.bmp for merged provinces.")

def parse_kv_args(raw: str) -> dict[str, object]:
    """
    Parse "key1=value1 key2=[1,2,3]" into {"key1": value1, "key2": [1, 2, 3]}.
    Each value is evaluated as a Python literal (int, list, string, ...).
    """
    pattern = re.compile(r"(\w+)=(\[[^\]]*\]|\S+)")
    parsed: dict[str, object] = {}

    for key, raw_value in pattern.findall(raw):
        try:
            parsed[key] = ast.literal_eval(raw_value)
        except (ValueError, SyntaxError):
            parsed[key] = raw_value  # leave as a plain string if it's not a literal

    return parsed

def parse_command(line: str) -> tuple[str | None, dict[str, object] | None]:
    command, _, rest = line.lower().strip().partition(" ")

    schema = COMMAND_SCHEMAS.get(command)
    if schema is None:
        print(f"Unknown command: {command}")
        return None, None

    kwargs = parse_kv_args(rest)

    missing = schema - kwargs.keys()
    if missing:
        print(f"{command}: missing argument(s): {', '.join(sorted(missing))}")
        return None, None

    unknown = kwargs.keys() - schema
    if unknown:
        print(f"{command}: unknown argument(s): {', '.join(sorted(unknown))}")
        return None, None

    return command, kwargs

def read_id_list_arg(kwargs: dict[str, object], command: str, arg_name: str, upper_bound: int, entity: str, allow_empty: bool = False) -> list[int] | None:
    """
    Validate a `name=[1, 2, 3]` command argument: it must be a list of
    integer ids, each in (0, upper_bound). Prints an error and returns None
    if the argument is invalid; otherwise returns the list.
    """
    value = kwargs.get(arg_name)

    if not isinstance(value, list) or not all([isinstance(e, int) for e in value]) or (len(value) == 0 and not allow_empty):
        empty_hint = "(possibly empty) " if allow_empty else ""
        print(f"\033[1;31mERROR\033[0m: {command} command requires '{arg_name}' arg to be a {empty_hint}list of integers")
        return None

    invalid = [e for e in value if not 0 < e < upper_bound]
    if invalid:
        print(f"\033[1;31mERROR\033[0m: invalid {entity} in {command} command {invalid}")
        return None

    return value

def main():
    localisation = load_localisation()

    provinces_list, states_list, regions_list = load_map()

    errors = verify_map(provinces_list, states_list, regions_list)
    if len(errors) > 0:
        for error in errors:
            print(f"\033[1;31mERROR\033[0m: {error}")
        return

    # Clear the terminal
    os.system('cls' if os.name == 'nt' else 'clear')

    print(COMMANDS_STRING)

    commands_queue: list[MoveOperation | MergeProvincesOperation | MergeStatesOperation | CreateStateOperation | MoveProvinceRegionOperation | MoveStateRegionOperation | CreateRegionOperation | MergeRegionsOperation] = []

    run_queue: bool = True

    # Ints used for tracking live state/region IDs when creating new ones
    net_state_count: int = len(states_list)
    net_region_count: int = len(regions_list)

    while True:
        user_input = input("")

        command, kwargs = parse_command(user_input)

        if not command:
            continue

        match command:
            case "s_move":
                provinces: Any = read_id_list_arg(kwargs, "s_move", "provinces", len(provinces_list), "provinces")
                state: Any = kwargs.get("into")

                if provinces is None:
                    continue

                if not isinstance(state, int):
                    print("\033[1;31mERROR\033[0m: s_move command requires 'state' arg to be an integer")
                    continue

                if not 0 < state < len(states_list):
                    print(f"\033[1;31mERROR\033[0m: invalid state in s_move command")
                    continue

                for prov in provinces:
                    commands_queue.append(MoveOperation(prov, state))

                print(f"\033[1;32mQueued\033[0m: move provinces {provinces} to state {state} ({len(commands_queue)} operation(s) queued)")

            case "p_merge":
                provinces: Any = read_id_list_arg(kwargs, "p_merge", "provinces", len(provinces_list), "provinces")
                into: Any = kwargs.get("into")

                if provinces is None:
                    continue

                if not isinstance(into, int):
                    print("\033[1;31mERROR\033[0m: p_merge command requires 'into' arg to be an integer")
                    continue

                if not 0 < into < len(provinces_list):
                    print(f"\033[1;31mERROR\033[0m: invalid into province in p_merge command")
                    continue

                for prov in provinces:
                    commands_queue.append(MergeProvincesOperation(prov, into))

                print(f"\033[1;32mQueued\033[0m: merge provinces {provinces} into {into} ({len(commands_queue)} operation(s) queued)")

            case "s_merge":
                states: Any = read_id_list_arg(kwargs, "s_merge", "states", len(states_list), "states")
                into: Any = kwargs.get("into")

                if states is None:
                    continue

                if not isinstance(into, int):
                    print("\033[1;31mERROR\033[0m: s_merge command requires 'into' arg to be an integer")
                    continue

                if not 0 < into < len(states_list):
                    print(f"\033[1;31mERROR\033[0m: invalid into state in s_merge command")
                    continue

                for state in states:
                    commands_queue.append(MergeStatesOperation(state, into))
                    net_state_count -= 1

                print(f"\033[1;32mQueued\033[0m: merge states {states} into {into} ({len(commands_queue)} operation(s) queued)")

            case "s_create":
                provinces: Any = read_id_list_arg(kwargs, "s_create", "provinces", len(provinces_list), "provinces")

                if provinces is None:
                    continue

                commands_queue.append(CreateStateOperation(provinces[0], net_state_count))

                for prov in provinces[1:]:
                    commands_queue.append(MoveOperation(prov, net_state_count))

                net_state_count +=1

                print(f"\033[1;32mQueued\033[0m: create state with provinces {provinces} ({len(commands_queue)} operation(s) queued)")

            case "r_move":
                states: Any = read_id_list_arg(kwargs, "r_move", "states", len(states_list), "states", allow_empty=True)
                provinces: Any = read_id_list_arg(kwargs, "r_move", "provinces", len(provinces_list), "provinces", allow_empty=True)
                region: Any = kwargs.get("into")

                if states is None or provinces is None:
                    continue

                if len(states) == 0 and len(provinces) == 0:
                    print("\033[1;31mERROR\033[0m: r_move command requires at least one state or province")
                    continue

                if not isinstance(region, int):
                    print("\033[1;31mERROR\033[0m: r_move command requires 'region' arg to be an integer")
                    continue

                if not 0 < region < net_region_count:
                    print(f"\033[1;31mERROR\033[0m: invalid region in r_move command")
                    continue

                for state in states:
                    commands_queue.append(MoveStateRegionOperation(state, region))

                for prov in provinces:
                    commands_queue.append(MoveProvinceRegionOperation(prov, region))

                print(f"\033[1;32mQueued\033[0m: move states {states} and provinces {provinces} to strategic region {region} ({len(commands_queue)} operation(s) queued)")

            case "r_create":
                states: Any = read_id_list_arg(kwargs, "r_create", "states", len(states_list), "states", allow_empty=True)
                provinces: Any = read_id_list_arg(kwargs, "r_create", "provinces", len(provinces_list), "provinces", allow_empty=True)

                if states is None or provinces is None:
                    continue

                if len(states) == 0 and len(provinces) == 0:
                    print("\033[1;31mERROR\033[0m: r_create command requires at least one state or province")
                    continue

                commands_queue.append(CreateRegionOperation(net_region_count))

                for state in states:
                    commands_queue.append(MoveStateRegionOperation(state, net_region_count))

                for prov in provinces:
                    commands_queue.append(MoveProvinceRegionOperation(prov, net_region_count))

                net_region_count += 1

                print(f"\033[1;32mQueued\033[0m: create strategic region with states {states} and provinces {provinces} ({len(commands_queue)} operation(s) queued)")

            case "r_merge":
                regions: Any = read_id_list_arg(kwargs, "r_merge", "regions", net_region_count, "regions")
                into: Any = kwargs.get("into")

                if regions is None:
                    continue

                if not isinstance(into, int):
                    print("\033[1;31mERROR\033[0m: r_merge command requires 'into' arg to be an integer")
                    continue

                if not 0 < into < net_region_count:
                    print(f"\033[1;31mERROR\033[0m: invalid into region in r_merge command")
                    continue

                if into in regions:
                    print(f"\033[1;31mERROR\033[0m: r_merge cannot merge region {into} into itself")
                    continue

                for region in regions:
                    commands_queue.append(MergeRegionsOperation(region, into))
                    net_region_count -= 1

                print(f"\033[1;32mQueued\033[0m: merge strategic regions {regions} into {into} ({len(commands_queue)} operation(s) queued)")

            case "edit":
                if len(commands_queue) == 0:
                    print("The move queue is empty, nothing to edit.")
                    continue

                edit_move_queue(commands_queue)
                print(COMMANDS_STRING)

            case "run":
                break

            case "quit":
                run_queue = False
                break

            case _:
                print(f"\033[1;31mERROR\033[0m: Unknown command: {command}")


    if run_queue:
        execute_move_commands(provinces_list, states_list, regions_list, commands_queue, localisation)

if __name__ == "__main__":
    main()