# Libgen Resource Skill Manual (`libgen-resource`)

This manual details the architecture, mirror fallback strategies, and command options for searching and downloading educational literature from Library Genesis into `resources/`.

______

## 1. Architecture & Resilient Mirror Resolution

Library Genesis mirrors change IP routing, cloud protection (Cloudflare/Nginx), and domain extensions frequently.

The skill integrates `libgen-api-enhanced` alongside direct HTTP scraping headers (`User-Agent`, `Referer`) to bypass basic CDN scrap blockers:

1. **Active Mirrors Tested**:
   - `https://libgen.li` (Active primary index and `ads.php` download gatekeeper).
   - `https://libgen.vg` (Active secondary mirror).
   - `https://libgen.is` / `https://libgen.rs` (Alternate catalog mirrors).

2. **Download Handshake**:
   - Query yields a list of candidates with mirror links.
   - For `libgen.li`, the target `ads.php?md5=<md5>` page is parsed to extract the direct `get.php?md5=<md5>&key=<key>` download endpoint.
   - The file is streamed with chunked writes into `resources/<author>_<title>.<ext>`.

______

## 2. CLI Tool Usage (`libgen_fetch.py`)

The skill provides an automated Typer CLI script at:
`.agents/skills/libgen-resource/scripts/libgen_fetch.py`

### 2.1 Searching

```bash
uv run python .agents/skills/libgen-resource/scripts/libgen_fetch.py search "<Book Title or Keywords>" [--limit 5] [--mirror https://libgen.li]
```

### 2.2 Downloading & BibTeX Registration

```bash
uv run python .agents/skills/libgen-resource/scripts/libgen_fetch.py download "<Book Title>" [--preferred-format pdf] [--output-dir resources] [--bib-file resources/references.bib]
```

- If direct download succeeds, the file is saved to `resources/` and referenced in `resources/references.bib`.
- If download times out (due to CDN bandwidth limits or CAPTCHA), the BibTeX entry is still recorded with the mirror link, allowing manual or out-of-band retrieval.
