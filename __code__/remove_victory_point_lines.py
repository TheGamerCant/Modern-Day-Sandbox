"""Remove single-line `victory_points = { <int> <int> }` entries from every
file in history/states.

Only lines whose entire content (ignoring leading/trailing whitespace) matches
that exact shape are removed; everything else in each file is left byte-for-byte
intact, including original line endings. Multi-line victory_points blocks are
NOT touched — the pattern requires the whole thing on one line.

Set DRY_RUN = True to preview what would be removed without writing changes.
"""

from __future__ import annotations

import re
from pathlib import Path

# history/states sits at the mod root, two levels up from this script (__code__/).
STATES_DIR = Path(__file__).resolve().parent.parent / "history" / "states"

DRY_RUN = False  # True = only report, don't modify files

# ^ optional indent, victory_points = { int int }, optional trailing spaces, EOL.
VICTORY_POINTS_LINE = re.compile(
    r"^[ \t]*victory_points[ \t]*=[ \t]*\{[ \t]*\d+[ \t]+\d+[ \t]*\}[ \t]*$"
)


def strip_victory_point_lines(path: Path) -> int:
    """Remove matching lines from one file. Returns how many were removed."""
    text = path.read_text(encoding="utf-8")
    lines = text.splitlines(keepends=True)  # keepends preserves \n / \r\n exactly

    kept = [line for line in lines if not VICTORY_POINTS_LINE.match(line.rstrip("\r\n"))]
    removed = len(lines) - len(kept)

    if removed and not DRY_RUN:
        path.write_text("".join(kept), encoding="utf-8")
    return removed


def main() -> None:
    if not STATES_DIR.is_dir():
        raise SystemExit(f"history/states not found at {STATES_DIR}")

    total_removed = 0
    files_changed = 0
    for path in sorted(STATES_DIR.rglob("*.txt")):
        removed = strip_victory_point_lines(path)
        if removed:
            files_changed += 1
            total_removed += removed
            print(f"{path.name}: {removed} line(s)")

    verb = "would remove" if DRY_RUN else "removed"
    print(f"\nDone: {verb} {total_removed} victory_points line(s) across {files_changed} file(s).")


if __name__ == "__main__":
    main()
