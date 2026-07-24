"""Scrape city populations from citypopulation.de.

Given a list of ``citypopulation.de/en/<country>/cities/`` pages, extract each
locality and its MOST RECENT population figure into a tidy table
(City, Country, Population, Reference Date, Source URL) and write it to CSV.

Usage
-----
    # explicit URLs
    python scrape_citypopulation.py \
        https://www.citypopulation.de/en/germany/cities/ \
        https://www.citypopulation.de/en/france/cities/ \
        -o city_populations.csv

    # bare country slugs also work (expanded to the full URL)
    python scrape_citypopulation.py germany france spain -o city_populations.csv

    # or read one URL/slug per line from a file
    python scrape_citypopulation.py --url-file countries.txt -o city_populations.csv

Notes
-----
* citypopulation.de serves the data table in static HTML but rejects the
  default ``requests`` user-agent, so a browser-like UA is sent.
* "Most recent population" = the right-most non-empty population column for
  each row (the newest column is sometimes blank for smaller places, so we walk
  leftwards to the first populated figure and record which date it came from).
* Be polite: there is a delay between requests. Don't hammer the site.

Dependencies: ``pip install requests beautifulsoup4 pandas`` (``lxml`` optional,
used automatically if installed; otherwise the stdlib HTML parser is used).
"""

from __future__ import annotations

import argparse
import re
import sys
import time
from dataclasses import dataclass
from pathlib import Path

import pandas as pd
import requests
from bs4 import BeautifulSoup, Tag

BASE_URL = "https://www.citypopulation.de"


def _pick_parser() -> str:
    """Use lxml if it's installed (fast), else the stdlib parser (always there)."""
    try:
        import lxml  # noqa: F401
        return "lxml"
    except ImportError:
        return "html.parser"


_PARSER = _pick_parser()

# citypopulation.de returns 403 for the stock requests UA; mimic a browser.
_HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/124.0 Safari/537.36"
    ),
    "Accept-Language": "en-US,en;q=0.9",
}

# A full ISO date (1987-05-25) or a bare year (2023) in a column header.
_DATE_RE = re.compile(r"\d{4}(?:-\d{2}-\d{2})?")


@dataclass
class CityPopulation:
    city: str
    country: str
    population: int
    reference_date: str  # date the figure refers to; "" if not determinable
    source_url: str


def normalize_url(url_or_slug: str) -> str:
    """Accept a full cities URL or a bare country slug and return a full URL."""
    text = url_or_slug.strip()
    if text.startswith("http"):
        return text
    
    return f"{BASE_URL}/en/{text}/"


def country_from_url(url: str) -> str:
    """Pull the country name out of a citypopulation.de URL slug.

    ``.../en/united-kingdom/cities/`` -> ``United Kingdom``.
    """
    match = re.search(r"/en/([^/]+)/", url)
    slug = match.group(1) if match else "unknown"
    return slug.replace("-", " ").replace("_", " ").title()


class BotChallengeError(RuntimeError):
    """Raised when citypopulation.de returns its anti-bot interstitial instead
    of real content. Not something to defeat — a signal to slow down / stop."""


def _is_bot_challenge(html: str) -> bool:
    """Detect the "Check for Humans" interstitial (a short page with that title)."""
    lowered = html.lower()
    return "check for humans" in lowered or (
        len(html) < 3000 and "verify" in lowered and "human" in lowered
    )


def fetch_html(url: str, session: requests.Session, timeout: int = 30) -> str:
    response = session.get(url, headers=_HEADERS, timeout=timeout)
    response.raise_for_status()
    # citypopulation.de is UTF-8; make sure requests doesn't mis-guess.
    response.encoding = response.apparent_encoding or "utf-8"
    html = response.text
    if _is_bot_challenge(html):
        raise BotChallengeError(
            f"{url} returned an anti-bot challenge page instead of data. "
            "The site is rate-limiting automated requests — increase --delay "
            "and/or run fewer URLs at a time."
        )
    return html


def _clean_population(raw: str) -> int | None:
    """Turn a population cell string into an int, or None if it isn't a number.

    Handles thousands separators (comma, space, non-breaking / thin spaces) and
    placeholder cells ("...", "-", "").
    """
    digits = re.sub(r"[^\d]", "", raw)
    return int(digits) if digits else None


def _looks_like_data_row(row: Tag) -> bool:
    """True if a row looks like a locality row: a name plus ≥2 numeric cells.

    This is markup-independent — it works whether or not the site tags cells
    with ``rname`` / ``rpop`` classes (the ``/cities/`` listing pages do; the
    census / admin-division pages use plain tables that don't).
    """
    cells = row.find_all(["td", "th"])
    if len(cells) < 2:
        return False
    has_name = row.find("a") is not None or _clean_population(cells[0].get_text()) is None
    numeric_cells = sum(1 for cell in cells if _clean_population(cell.get_text()) is not None)
    return bool(has_name) and numeric_cells >= 1


def _population_header_columns(table: Tag) -> int:
    """Count header cells that name a population column (e.g. "Population
    Census 2021-08-11" / "Population Estimate 2023"). This is the single most
    reliable fingerprint of the real data grid: the region-navigation menu can
    contain population *numbers*, but it has no such dated population *header*.
    """
    header = table.find("thead") or table
    count = 0
    for cell in header.find_all(["th", "td"]):
        text = cell.get_text(" ", strip=True).lower()
        if _DATE_RE.search(text) and (
            "population" in text or "census" in text or "estimate" in text
        ):
            count += 1
    return count


def _find_data_table(soup: BeautifulSoup) -> Tag | None:
    """Locate the cities / localities grid.

    A citypopulation.de page has several tables — the site's region-navigation
    menu (which can itself list populations), a one-row summary for the area,
    and the localities grid we want. We rank tables by, in order: number of
    dated population-header columns, then data rows (a name + numeric cells),
    then ``rpop`` cell count. Anchoring on the population *header* first means a
    navigation menu full of numbers can't win — it has no such header — and the
    many-row localities grid beats the one-row area summary.
    """
    tables = soup.find_all("table")
    if not tables:
        return None

    def table_score(table: Tag) -> tuple[int, int, int]:
        header_columns = _population_header_columns(table)
        data_rows = sum(1 for row in table.find_all("tr") if _looks_like_data_row(row))
        population_cells = sum(
            1
            for td in table.find_all("td")
            if any(cls.startswith("rpop") for cls in td.get("class", []))
        )
        return (header_columns, data_rows, population_cells)

    best_table = max(tables, key=table_score)
    # If nothing looks like a data table at all, report failure (not a nav menu).
    return best_table if table_score(best_table)[1] > 0 else None


def _header_dates(table: Tag) -> list[str]:
    """Ordered reference dates of the population columns, from the header row.

    Returns the date/year token of every header cell that looks like a
    population column. Used to label which date a row's figure came from.
    """
    header = table.find("thead") or table
    dates: list[str] = []
    for th in header.find_all("th"):
        text = th.get_text(" ", strip=True)
        match = _DATE_RE.search(text)
        if match and ("population" in text.lower() or "census" in text.lower()
                      or "estimate" in text.lower() or _DATE_RE.fullmatch(text)):
            dates.append(match.group(0))
    return dates


def _row_name(row: Tag) -> str | None:
    """The locality name for a data row.

    The name is always the first column. We take that cell's text directly
    rather than the first ``<a>`` — census rows end in a "→" detail link, and
    grabbing the first link can otherwise pick up that arrow instead of the name.
    """
    name_cell = row.find("td", class_="rname")
    if isinstance(name_cell, Tag):
        return name_cell.get_text(" ", strip=True)

    cells = row.find_all(["td", "th"])
    if cells:
        first_text = cells[0].get_text(" ", strip=True)
        # A real name isn't a pure number and isn't a lone symbol like "→".
        if first_text and _clean_population(first_text) is None and len(first_text) > 1:
            return first_text
    return None


def _row_population_cells(row: Tag) -> list[Tag]:
    """Population cells of a row, left (oldest) to right (newest)."""
    pop_cells = [
        td for td in row.find_all("td")
        if any(cls.startswith("rpop") for cls in td.get("class", []))
    ]
    if pop_cells:
        return pop_cells
    # Fallback: every cell after the first that parses as a number.
    return [td for td in row.find_all("td")[1:] if _clean_population(td.get_text())]


def parse_population_table(html: str, country: str, url: str) -> list[CityPopulation]:
    soup = BeautifulSoup(html, _PARSER)
    table = _find_data_table(soup)
    if table is None:
        return []

    column_dates = _header_dates(table)
    body = table.find("tbody") or table

    results: list[CityPopulation] = []
    for row in body.find_all("tr"):
        name = _row_name(row)
        if not name:
            continue

        pop_cells = _row_population_cells(row)
        if not pop_cells:
            continue

        # Walk right-to-left to the most recent populated figure.
        most_recent: int | None = None
        used_index: int | None = None
        for index in range(len(pop_cells) - 1, -1, -1):
            value = _clean_population(pop_cells[index].get_text())
            if value is not None:
                most_recent = value
                used_index = index
                break
        if most_recent is None:
            continue

        # Map the used column back to its header date when the counts line up.
        reference_date = ""
        if used_index is not None and len(column_dates) == len(pop_cells):
            reference_date = column_dates[used_index]

        results.append(CityPopulation(
            city=name,
            country=country,
            population=most_recent,
            reference_date=reference_date,
            source_url=url,
        ))

    return results


def debug_tables(html: str) -> None:
    """Print a summary of every table on the page so mis-selection is diagnosable.

    Shows each table's population-header column count, data-row count and its
    header text, plus which table would be chosen. Run with ``--debug``.
    """
    soup = BeautifulSoup(html, _PARSER)
    tables = soup.find_all("table")
    chosen = _find_data_table(soup)

    # Raw-HTML clues — vital when 0 tables are found, to tell apart a JS-rendered
    # page, a <div>/role="table" grid, or a JSON blob embedded in a <script>.
    lower_html = html.lower()
    table_count = lower_html.count("<table")
    tr_count = lower_html.count("<tr")
    role_count = lower_html.count('role="row"') + lower_html.count('role="gridcell"')
    js_gated = "yes" if "enable javascript" in lower_html else "no"
    print(f"html length: {len(html)} chars")
    print(f"raw counts: <table>={table_count} <tr>={tr_count} "
          f"role=row/gridcell={role_count} 'enable JavaScript'={js_gated}")
    # Show the markup around the first locality-ish sentinel so the container
    # (a <div>, a <script> data blob, etc.) is visible.
    for sentinel in ("Alice Springs", "Locality", "Urban Cent", "Population"):
        index = html.find(sentinel)
        if index != -1:
            start = max(0, index - 220)
            print(f"\ncontext around first {sentinel!r}:\n"
                  f"...{html[start:index + 120]}...\n")
            break
    else:
        print("\n(no locality/population sentinel text found in raw HTML — "
              "data is almost certainly loaded by JavaScript)")

    print(f"found {len(tables)} table(s)")
    for i, table in enumerate(tables):
        header = table.find("thead") or table
        header_row = header.find("tr")
        header_text = header_row.get_text(" | ", strip=True) if header_row else "(no header)"
        data_rows = sum(1 for row in table.find_all("tr") if _looks_like_data_row(row))
        marker = "  <-- CHOSEN" if table is chosen else ""
        print(f"[{i}] pop_header_cols={_population_header_columns(table)} "
              f"data_rows={data_rows} total_rows={len(table.find_all('tr'))}{marker}")
        print(f"     header: {header_text[:160]}")


def scrape(urls: list[str], delay: float = 1.5) -> pd.DataFrame:
    session = requests.Session()
    all_rows: list[CityPopulation] = []

    for i, raw in enumerate(urls):
        url = normalize_url(raw)
        country = country_from_url(url)
        try:
            html = fetch_html(url, session)
        except BotChallengeError as exc:
            print(f"[BLOCKED] {exc}", file=sys.stderr)
            print("[BLOCKED] stopping so we don't keep hammering the site.",
                  file=sys.stderr)
            break
        except requests.RequestException as exc:
            print(f"[WARN] failed to fetch {url}: {exc}", file=sys.stderr)
            continue

        rows = parse_population_table(html, country, url)
        print(f"[OK] {country}: {len(rows)} cities", file=sys.stderr)
        all_rows.extend(rows)

        if i < len(urls) - 1:
            time.sleep(delay)  # be polite between requests

    return pd.DataFrame(
        [vars(r) for r in all_rows],
        columns=["city", "country", "population", "reference_date", "source_url"],
    )


def _read_url_file(path: Path) -> list[str]:
    lines = path.read_text(encoding="utf-8").splitlines()
    return [line.strip() for line in lines if line.strip() and not line.startswith("#")]


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("urls", nargs="*", help="cities URLs or bare country slugs")
    parser.add_argument("--url-file", type=Path, help="file with one URL/slug per line")
    parser.add_argument("-o", "--output", type=Path, default=Path("city_populations.csv"))
    parser.add_argument("--delay", type=float, default=1.5,
                        help="seconds to wait between requests (default 1.5)")
    parser.add_argument("--debug", action="store_true",
                        help="print a per-table diagnostic for each page and exit")
    args = parser.parse_args()

    urls = list(args.urls)
    if args.url_file:
        urls.extend(_read_url_file(args.url_file))
    if not urls:
        parser.error("provide at least one URL/slug, or --url-file")

    if args.debug:
        session = requests.Session()
        for raw in urls:
            url = normalize_url(raw)
            print(f"\n=== {url} ===")
            debug_tables(fetch_html(url, session))
        return

    populations_df = scrape(urls, delay=args.delay)
    # utf-8-sig so the CSV opens cleanly in Excel (matches the rest of this repo).
    populations_df.to_csv(args.output, index=False, encoding="utf-8-sig")
    print(f"Wrote {len(populations_df)} rows to {args.output}")


if __name__ == "__main__":
    main()
