"""Merge VP population / victory-point data into names.json.

Reads province_population/vp_pops.csv and, for each victory_point_names entry in
names.json matched by id == prov_id:

* if the row has a population AND a year, sets
      entry["population"] = {"tag": <owner>, "population": <int>, "year": <int>}
* if the row has victory_points, sets
      entry["victory_points"] = <int>

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


def merge(names_path: Path, csv_path: Path) -> tuple[int, int]:
    data: dict[str, Any] = json.loads(names_path.read_text(encoding="utf-8"))
    vp_rows = load_vp_rows(csv_path)

    populations_added = 0
    victory_points_added = 0
    for entry in data.get("victory_point_names", []):
        row = vp_rows.get(entry.get("id"))
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
            populations_added += 1

        victory_points = _to_int(row.get("victory_points"))
        if victory_points is not None:
            entry["victory_points"] = victory_points
            victory_points_added += 1

    # ensure_ascii=False keeps accented names (Zürich, …) readable.
    names_path.write_text(
        json.dumps(data, indent=4, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    return populations_added, victory_points_added


def main() -> None:
    populations_added, victory_points_added = merge(NAMES_JSON, VP_POPS_CSV)
    print(
        f"Added population objects to {populations_added} entries, "
        f"victory_points to {victory_points_added} entries."
    )


if __name__ == "__main__":
    main()
