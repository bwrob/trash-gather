"""Search and download educational books and resources from Library Genesis.

Uses libgen-api-enhanced and resilient mirror resolution to find books,
save downloadable copies into resources/, and format BibTeX bibliography entries.
"""

from __future__ import annotations

import re
from pathlib import Path
from typing import Annotated
from urllib.parse import urljoin

import requests
import typer
from bs4 import BeautifulSoup
from libgen_api_enhanced.search_request import SearchRequest, SearchType

app = typer.Typer(
    help="Search and download book resources from Library Genesis and manage BibTeX references.",
    add_completion=False,
)

BROWSER_HEADERS = {
    "User-Agent": (
        "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/124.0.0.0 Safari/537.36"
    ),
    "Accept": "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8",
}


def format_resource_filename(author: str, year: str, title: str, extension: str) -> str:
    """Format filename strictly as <author_surname>_<year>_<short_title_slug>.<ext>."""
    first_author = author.split(";")[0].split(",")[0].strip()
    author_surname = re.sub(r"[^\w]", "", first_author).lower() or "author"
    year_digits = re.sub(r"\D", "", year)[:4] or "nodate"
    # Take first 1-4 key title words
    words = [w.lower() for w in re.findall(r"\b[A-Za-z0-9]+\b", title)[:4]]
    title_slug = "_".join(words) or "resource"
    ext_clean = extension.lstrip(".").lower()
    return f"{author_surname}_{year_digits}_{title_slug}.{ext_clean}"


def generate_bibtex_key(author: str, year: str, title: str) -> str:
    """Generate a clean BibTeX citation key."""
    first_author = author.split(";")[0].split(",")[0].strip()
    author_part = re.sub(r"[^\w]", "", first_author).lower() or "anon"
    year_part = re.sub(r"\D", "", year)[:4] or "nodate"
    first_word = re.findall(r"\b[A-Za-z]{3,}\b", title)
    title_word = first_word[0].lower() if first_word else "book"
    return f"{author_part}{year_part}{title_word}"


def format_bibtex_entry(
    key: str,
    title: str,
    author: str,
    year: str,
    publisher: str = "",
    file_path: str = "",
    url: str = "",
) -> str:
    """Format a BibTeX @book entry."""
    lines = [
        f"@book{{{key},",
        f"  title     = {{{title}}},",
        f"  author    = {{{author}}},",
        f"  year      = {{{year}}},",
    ]
    if publisher:
        lines.append(f"  publisher = {{{publisher}}},")
    if file_path:
        lines.append(f"  file      = {{{file_path}}},")
    if url:
        lines.append(f"  url       = {{{url}}},")
    lines.append("}\n")
    return "\n".join(lines)


def fetch_search_results(query: str, mirror: str = "https://libgen.li") -> list[dict[str, str]]:
    """Search Libgen directly with custom headers to prevent CDN blocking."""
    params = {
        "req": query,
        "columns[]": "t",
        "objects[]": ["f", "e", "s", "a", "p", "w"],
        "topics[]": ["l", "c", "f", "a", "m", "r", "s"],
        "res": "25",
        "filesuns": "all",
    }
    url = f"{mirror}/index.php"
    response = requests.get(
        url,
        params=params,
        cookies={"covers": "on"},
        headers=BROWSER_HEADERS,
        timeout=15,
    )
    response.raise_for_status()

    soup = BeautifulSoup(response.text, "html.parser")
    table = soup.find("table", {"id": "tablelibgen"})
    if not table:
        return []

    req_helper = SearchRequest(query, SearchType.TITLE, mirror=mirror)
    books = list(req_helper.get_books(table))

    results = []
    for b in books:
        results.append(
            {
                "id": b.id,
                "title": b.title,
                "author": b.author,
                "publisher": b.publisher,
                "year": b.year,
                "extension": b.extension,
                "size": b.size,
                "mirrors": b.mirrors,
            }
        )
    return results


def resolve_direct_download_url(ads_url: str) -> str | None:
    """Extract direct download link from a libgen ads.php page."""
    try:
        resp = requests.get(ads_url, headers=BROWSER_HEADERS, timeout=15)
        resp.raise_for_status()
        soup = BeautifulSoup(resp.text, "html.parser")
        for a in soup.find_all("a", href=True):
            href = a["href"]
            if "get.php" in href or "download" in a.text.lower() or a.text.strip() == "GET":
                return urljoin(ads_url, href)
    except Exception as e:
        typer.echo(f"Warning: Failed to extract download URL from {ads_url}: {e}", err=True)
    return None


@app.command()
def search(
    query: Annotated[str, typer.Argument(help="Search query (title, author, or keywords)")],
    mirror: Annotated[str, typer.Option(help="Libgen mirror URL to query")] = "https://libgen.li",
    limit: Annotated[int, typer.Option(help="Maximum number of results to display")] = 5,
) -> None:
    """Search Library Genesis for educational books."""
    typer.echo(f"Searching Library Genesis for '{query}'...")
    try:
        results = fetch_search_results(query, mirror=mirror)
    except Exception as e:
        typer.echo(f"Error connecting to {mirror}: {e}", err=True)
        raise typer.Exit(code=1) from e

    if not results:
        typer.echo("No books found matching query.")
        return

    typer.echo(f"\nFound {len(results)} results (showing top {min(limit, len(results))}):\n")
    for idx, b in enumerate(results[:limit], 1):
        typer.echo(f"[{idx}] {b['title']}")
        typer.echo(f"    Author:    {b['author']}")
        typer.echo(f"    Year:      {b['year']} | Ext: {b['extension']} | Size: {b['size']}")
        typer.echo(f"    Primary:   {b['mirrors'][0] if b['mirrors'] else 'N/A'}\n")


@app.command()
def download(
    query: Annotated[str, typer.Argument(help="Book title or keywords to search and download")],
    output_dir: Annotated[
        Path, typer.Option(help="Output directory to store downloaded file")
    ] = Path("resources"),
    bib_file: Annotated[Path, typer.Option(help="Path to BibTeX bibliography file")] = Path(
        "resources/references.bib"
    ),
    mirror: Annotated[str, typer.Option(help="Libgen mirror URL")] = "https://libgen.li",
    preferred_format: Annotated[
        str, typer.Option(help="Preferred file format (e.g. pdf, epub)")
    ] = "pdf",
) -> None:
    """Search for a book, download the best match, and register it in resources/references.bib."""
    typer.echo(f"Searching for '{query}' to download...")
    try:
        results = fetch_search_results(query, mirror=mirror)
    except Exception as e:
        typer.echo(f"Error searching {mirror}: {e}", err=True)
        raise typer.Exit(code=1) from e

    if not results:
        typer.echo("No matching books found to download.", err=True)
        raise typer.Exit(code=1)

    matching = [b for b in results if b["extension"].lower() == preferred_format.lower()]
    target = matching[0] if matching else results[0]

    typer.echo("\nTarget Book Selected:")
    typer.echo(f"  Title:  {target['title']}")
    typer.echo(f"  Author: {target['author']}")
    typer.echo(f"  Year:   {target['year']} ({target['extension']}, {target['size']})")

    download_url = None
    for mirror_link in target["mirrors"]:
        if "ads.php" in mirror_link:
            download_url = resolve_direct_download_url(mirror_link)
            if download_url:
                break

    output_dir.mkdir(parents=True, exist_ok=True)
    filename = format_resource_filename(
        author=target["author"],
        year=target["year"],
        title=target["title"],
        extension=target["extension"],
    )
    dest_path = output_dir / filename

    bib_key = generate_bibtex_key(target["author"], target["year"], target["title"])
    download_success = False

    if download_url:
        typer.echo(f"\nDownloading from direct mirror: {download_url}")
        try:
            headers = dict(BROWSER_HEADERS)
            headers["Referer"] = target["mirrors"][0]
            with requests.get(download_url, headers=headers, stream=True, timeout=30) as r:
                r.raise_for_status()
                with open(dest_path, "wb") as f:
                    for chunk in r.iter_content(chunk_size=65536):
                        if chunk:
                            f.write(chunk)
            typer.echo(f"✓ Downloaded successfully: {dest_path}")
            download_success = True
        except Exception as e:
            typer.echo(f"Download stream error: {e}", err=True)
    else:
        typer.echo("Could not resolve automated direct download link.", err=True)

    bib_file.parent.mkdir(parents=True, exist_ok=True)
    existing_bib = bib_file.read_text(encoding="utf-8") if bib_file.exists() else ""
    if f"@{bib_key}" not in existing_bib and f"{{{bib_key}," not in existing_bib:
        entry = format_bibtex_entry(
            key=bib_key,
            title=target["title"],
            author=target["author"],
            year=target["year"],
            publisher=target["publisher"],
            file_path=str(dest_path) if download_success else "",
            url=target["mirrors"][0] if target["mirrors"] else "",
        )
        with open(bib_file, "a", encoding="utf-8") as f:
            f.write(f"\n{entry}")
        typer.echo(f"✓ Appended BibTeX entry [{bib_key}] to {bib_file}")
    else:
        typer.echo(f"BibTeX entry [{bib_key}] already exists in {bib_file}")


if __name__ == "__main__":
    app()
