from pathlib import Path
from time import perf_counter
from __code__.country_maker.functions import get_map_from_brackets, load_file_to_string

def LoadIdeologies(mod_directory: Path) -> list[str]:
    ideology_files: list[Path] = list(Path(mod_directory / "common/ideologies").glob("**/*.txt"))

    ideologies: list[str] = []

    for file in ideology_files:
        ideologies.extend(get_map_from_brackets(get_map_from_brackets(load_file_to_string(str(file))).get("ideologies")).keys())

    return ideologies

def main():
    time_start: float = perf_counter()

    mod_directory: Path = Path.cwd()
    #mod_directory = mod_directory.parents[1]

    print(LoadIdeologies(mod_directory))

    load_time: float = perf_counter()- time_start

    print(f"Load Time: {load_time:.3}s")

if __name__ == "__main__":
    main()
