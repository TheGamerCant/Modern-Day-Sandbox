from pathlib import Path
import colorsys
from typing import Any
import re
from time import perf_counter

brightness_mult: float = 0.8

def adjustBrightness(match) -> str:
    r, g, b = map(int, match.groups())

    # Normalize to 0–1
    r_f, g_f, b_f = r / 255.0, g / 255.0, b / 255.0

    # Convert to HLS
    h, l, s = colorsys.rgb_to_hls(r_f, g_f, b_f)

    # Reduce brightness (lightness)
    l *= brightness_mult
    l = max(0, min(1, l))  # clamp

    # Convert back to RGB
    r_new, g_new, b_new = colorsys.hls_to_rgb(h, l, s)

    # Back to 0–255
    r_new = int(r_new * 255)
    g_new = int(g_new * 255)
    b_new = int(b_new * 255)

    return f"rgb {{ {r_new} {g_new} {b_new} }}"

def UpdateStateFiles(country_files_path: Path) -> None:
    country_files: list[Path] = list(country_files_path.glob("**/*.txt"))
    pattern = re.compile(r"rgb\s*{\s*(\d+)\s+(\d+)\s+(\d+)\s*}")

    for file in country_files:
        with open(str(file)) as f:
            text: str = f.read()

        text = pattern.sub(adjustBrightness, text)

        with open(str(file), "w") as f:
            f.write(text)

def main():
    time_start: float = perf_counter()

    mod_directory: Path = Path.cwd()
    mod_directory = mod_directory.parents[1]

    country_files_path: Path = mod_directory / "common/countries"

    UpdateStateFiles(country_files_path)

    time_taken: float = perf_counter()- time_start
    print(f"Time taken: {time_taken:.3}s")

if __name__ == "__main__":
    main()