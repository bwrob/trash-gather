# Educational Resources & Literature (`resources/`)

This directory houses referenced literature, books, papers, and BibTeX citations supporting the implementation and learning journey of `trash-gather`.

- Detailed chapter guides and syllabus outlines: [`INDEX.md`](./INDEX.md)
- Complete BibTeX citation database: [`references.bib`](./references.bib)

______

## 📏 Naming Convention

All literature files stored in this directory strictly adhere to the following convention:

```text
resources/<author_surname>_<year>_<short_title_slug>.<ext>
```

- **`<author_surname>`**: Lowercase ASCII last name of primary author (e.g. `reek`, `seacord`, `gustedt`, `jones`).
- **`<year>`**: 4-digit publication year (e.g. `1998`, `2020`, `2023`).
- **`<short_title_slug>`**: Concise title slug separated by underscores (e.g. `pointers_on_c`, `effective_c`, `modern_c`).
- **`<ext>`**: Document format extension (`.pdf`, `.epub`, `.djvu`).
- **Character Invariant**: Strictly lowercase alphanumeric and underscores (`[a-z0-9_]`). No spaces, punctuation, parentheses, or scraper prefixes.

Each file corresponds 1:1 with an entry in [`references.bib`](./references.bib) where the citation key is `<author_surname><year><short_title>`.

______

## 📚 Bibliography Catalog (`references.bib`)

- **`jones2023garbage`**: *The Garbage Collection Handbook: The Art of Automatic Memory Management* (2nd Edition, 2023) by Richard Jones, Antony Hosking, Eliot Moss. CRC Press. [`jones_2023_garbage_collection_handbook.pdf`](./jones_2023_garbage_collection_handbook.pdf)
- **`nystrom2021crafting`**: *Crafting Interpreters* (2021) by Robert Nystrom. Genever Benning. [`nystrom_2021_crafting_interpreters.pdf`](./nystrom_2021_crafting_interpreters.pdf)
- **`reek1998pointers`**: *Pointers on C* (1998) by Kenneth A. Reek. Addison-Wesley. [`reek_1998_pointers_on_c.pdf`](./reek_1998_pointers_on_c.pdf)
- **`seacord2020effective`**: *Effective C: An Introduction to Professional C Programming* (2020) by Robert C. Seacord. No Starch Press. [`seacord_2020_effective_c.pdf`](./seacord_2020_effective_c.pdf)
- **`gustedt2023modern`**: *Modern C* (3rd Edition, 2023) by Jens Gustedt. Manning Publications. [`gustedt_2023_modern_c.epub`](./gustedt_2023_modern_c.epub)
- **`sedgewick1998algorithms`**: *Algorithms in C, Parts 1--4: Fundamentals, Data Structures, Sorting, Searching* (3rd Edition, 1998) by Robert Sedgewick. Addison-Wesley Professional. [`sedgewick_1998_algorithms_in_c.djvu`](./sedgewick_1998_algorithms_in_c.djvu)
