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
    slug = text.strip("/")
    return f"{BASE_URL}/en/{slug}/cities/"


def country_from_url(url: str) -> str:
    """Pull the country name out of a citypopulation.de URL slug.

    ``.../en/united-kingdom/cities/`` -> ``United Kingdom``.
    """
    match = re.search(r"/en/([^/]+)/", url)
    slug = match.group(1) if match else "unknown"
    return slug.replace("-", " ").replace("_", " ").title()


def fetch_html(url: str, session: requests.Session, timeout: int = 30) -> str:
    response = session.get(url, headers=_HEADERS, timeout=timeout)
    response.raise_for_status()
    # citypopulation.de is UTF-8; make sure requests doesn't mis-guess.
    response.encoding = response.apparent_encoding or "utf-8"
    return response.text


def _clean_population(raw: str) -> int | None:
    """Turn a population cell string into an int, or None if it isn't a number.

    Handles thousands separators (comma, space, non-breaking / thin spaces) and
    placeholder cells ("...", "-", "").
    """
    digits = re.sub(r"[^\d]", "", raw)
    return int(digits) if digits else None


def _find_data_table(soup: BeautifulSoup) -> Tag | None:
    """Locate the cities grid.

    A citypopulation.de page usually has several tables — a small subdivisions
    grid (often first) plus the larger cities grid we actually want. We pick the
    table with the most population cells (``td.rpop*``), tie-broken by row count,
    which reliably selects the cities grid rather than the subdivisions one.
    Falls back to the table with the most rows if none expose ``rpop`` cells.
    """
    tables = soup.find_all("table")
    if not tables:
        return None

    def table_score(table: Tag) -> tuple[int, int]:
        population_cells = sum(
            1
            for td in table.find_all("td")
            if any(cls.startswith("rpop") for cls in td.get("class", []))
        )
        return (population_cells, len(table.find_all("tr")))

    return max(tables, key=table_score)


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
    """The locality name for a data row."""
    name_cell = row.find("td", class_="rname")
    if isinstance(name_cell, Tag):
        return name_cell.get_text(" ", strip=True)
    # Fallback: first cell containing a link (name is always linked).
    link = row.find("a")
    if isinstance(link, Tag):
        return link.get_text(" ", strip=True)
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


def scrape(urls: list[str], delay: float = 1.5) -> pd.DataFrame:
    session = requests.Session()
    all_rows: list[CityPopulation] = []

    for i, raw in enumerate(urls):
        url = normalize_url(raw)
        country = country_from_url(url)
        try:
            html = fetch_html(url, session)
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
    args = parser.parse_args()

    urls = list(args.urls)
    if args.url_file:
        urls.extend(_read_url_file(args.url_file))
    if not urls:
        parser.error("provide at least one URL/slug, or --url-file")

    populations_df = scrape(urls, delay=args.delay)
    # utf-8-sig so the CSV opens cleanly in Excel (matches the rest of this repo).
    populations_df.to_csv(args.output, index=False, encoding="utf-8-sig")
    print(f"Wrote {len(populations_df)} rows to {args.output}")


if __name__ == "__main__":
    main()
