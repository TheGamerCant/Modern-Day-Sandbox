"""Merge VP population / victory-point data into names.json.

Reads province_population/vp_pops.csv and, for each victory_point_names entry in
names.json matched by id == prov_id:

* if the row has a population AND a year, sets
      entry["population"] = {"tag": <owner>, "population": <int>, "year": <int>}
* if the row has victory_points, sets
      entry["victory_points"] = <int>

It also links every victory_point_names entry to map/definition.csv (matched by
id == province id) and sets
      entry["terrain"] = <terrain>   # e.g. plains / forest / urban / ocean

Rows are matched on the numeric id; state_names is left untouched. Re-running is
idempotent (it overwrites the same keys rather than duplicating them).
"""

from __future__ import annotations

import csv
import json
from pathlib import Path
from typing import Any

SCRIPT_DIR = Path(__file__).resolve().parent
NAMES_JSON = SCRIPT_DIR / "names.json"
VP_POPS_CSV = SCRIPT_DIR.parent / "province_population" / "vp_pops.csv"
# map/definition.csv lives at the mod root (two levels up from __code__/…).
DEFINITION_CSV = SCRIPT_DIR.parent.parent / "map" / "definition.csv"


def _to_int(value: str | None) -> int | None:
    """Parse a CSV cell to int, tolerating floats ("1.0") and blanks."""
    text = (value or "").strip()
    if not text:
        return None
    try:
        return int(float(text))
    except ValueError:
        return None


def load_vp_rows(csv_path: Path) -> dict[int, dict[str, str]]:
    """prov_id -> CSV row. utf-8-sig strips the BOM the file was written with."""
    rows_by_prov_id: dict[int, dict[str, str]] = {}
    with open(csv_path, encoding="utf-8-sig", newline="") as csv_file:
        for row in csv.DictReader(csv_file):
            prov_id = _to_int(row.get("prov_id"))
            if prov_id is not None:
                rows_by_prov_id[prov_id] = row
    return rows_by_prov_id


def load_terrain(definition_path: Path) -> dict[int, str]:
    """province id -> terrain, from map/definition.csv.

    Format is `id;r;g;b;type;coastal;terrain;continent` (semicolon-separated),
    so terrain is field index 6."""
    terrain_by_id: dict[int, str] = {}
    with open(definition_path, encoding="utf-8", newline="") as definition_file:
        for line in definition_file:
            fields = line.rstrip("\n").split(";")
            if len(fields) > 6 and fields[0].isdigit():
                terrain_by_id[int(fields[0])] = fields[6]
    return terrain_by_id


def merge(names_path: Path, csv_path: Path, definition_path: Path) -> dict[str, int]:
    data: dict[str, Any] = json.loads(names_path.read_text(encoding="utf-8"))
    vp_rows = load_vp_rows(csv_path)
    terrain_by_id = load_terrain(definition_path)

    counts = {"population": 0, "victory_points": 0, "terrain": 0}
    for entry in data.get("victory_point_names", []):
        province_id = entry.get("id")

        # Terrain applies to every province, independent of the population CSV.
        terrain = terrain_by_id.get(province_id)
        if terrain is not None:
            entry["terrain"] = terrain
            counts["terrain"] += 1

        row = vp_rows.get(province_id)
        if row is None:
            continue

        population = _to_int(row.get("population"))
        year = _to_int(row.get("year"))
        if population is not None and year is not None:
            entry["population"] = {
                "tag": (row.get("owner") or "").strip(),
                "population": population,
                "year": year,
            }
            counts["population"] += 1

        victory_points = _to_int(row.get("victory_points"))
        if victory_points is not None:
            entry["victory_points"] = victory_points
            counts["victory_points"] += 1

    # ensure_ascii=False keeps accented names (Zürich, …) readable.
    names_path.write_text(
        json.dumps(data, indent=4, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    return counts


def main() -> None:
    counts = merge(NAMES_JSON, VP_POPS_CSV, DEFINITION_CSV)
    print(
        f"Added population to {counts['population']} entries, "
        f"victory_points to {counts['victory_points']} entries, "
        f"terrain to {counts['terrain']} entries."
    )


if __name__ == "__main__":
    main()
