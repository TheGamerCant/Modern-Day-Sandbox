from pathlib import Path
import colorsys
from typing import Any
import re
from time import perf_counter


def UpdateStateFiles(country_files_path: Path) -> None:
    country_files: list[Path] = list(country_files_path.glob("**/*.txt"))

    for file in country_files:
        with open(str(file)) as f:
            text = f.read()

        text: str = re.sub(
            r"set_variable = { culture_index_array\^(\d+) = (\d+) }",
            lambda m: f"set_variable = {{ culture_index_array^{m.group(1)} = {old_to_new_index_dict.get(int(m.group(2)))} }}",
            text
        )

        with open(str(file), "w") as f:
            f.write(text)

def main():
    time_start: float = perf_counter()

    mod_directory: Path = Path.cwd()
    mod_directory = mod_directory.parents[1]

    country_files_path: Path = mod_directory / "common/countries"

    time_taken: float = perf_counter()- time_start
    print(f"Time taken: {time_taken:.3}s")