# Sources Index Template (`.sources/INDEX.md`)

This template defines the mandatory structure and schema for `.sources/INDEX.md`. When books, manuals, or standard references are added to `.sources/`, they must be cataloged in `.sources/INDEX.md` following this structure.

______

## Schema Specification

```markdown
# Bibliography & Reference Sources Index (`.sources/INDEX.md`)

This index catalogs reference volumes, technical manuals, and standard specifications available in `.sources/`. Each entry provides a high-level focus summary, complete Table of Contents, and concise pedagogical descriptions of each chapter/section to guide pre-flight milestone reading.

______________________________________________________________________

## 📚 Quick Reference & Catalog

| Author & Title | Format & Size | Core Pedagogical Focus | Key Relevancy to trash-gather |
| :--- | :--- | :--- | :--- |
| **<Author>**<br>*<Title>* | `<Format>`, `<Size>` | <High-level focus> | <Tag: Memory Safety / Allocators / Calling Conventions / etc.> |

______________________________________________________________________

## 1. <Book Title>

- **Author(s)**: <Full author name(s)>
- **Publisher / Year / Edition**: <Metadata>
- **File**: `.sources/<filename>`
- **Pedagogical Scope**: <2-3 sentence overview of the book's philosophy and systems engineering value.>

### Chapter Guides & Descriptive Table of Contents

#### Chapter <N>: <Chapter Title> (pp. X–Y / Level / Part)
- **<Section Number> <Section Title>**: <1-2 sentence pedagogical description of what this section explains, highlighting concrete machine mechanics, memory invariants, or C language rules.>
- ...

---

## 2. <Next Book Title>
...
```

______

## Content Guidelines for Descriptions

1. **Concrete Systems Mechanics**: Descriptions should not just restate the title. Explain *what* the section covers (e.g. ABI register conventions, cache alignment, heap metadata, UB traps).
1. **trash-gather Applicability**: Explicitly mention if a section covers mechanisms directly implemented in `trash-gather` (e.g., flexible array members for tuples, tagged unions for objects, stack frame layout for variadics, mark-and-sweep GC foundations, ASan verification).
1. **Accurate Page / Level Coordinates**: Use the actual page numbers (for PDFs) or Levels/Chapters (for EPUBs) to enable quick navigation for the developer.
