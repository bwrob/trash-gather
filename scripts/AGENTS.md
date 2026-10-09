# Scripts Guidelines (`scripts/AGENTS.md`)

This directory contains Python developer tooling, linters, scaffolding utilities, and DAG generators.

- **Mandate**: When any script accepts command-line input, options, flags, or arguments, **NEVER** use Python's built-in `argparse` module.
- **Framework**: Always use [`typer`](https://typer.tiangolo.com/) with type annotations.
- **Typing & Strictness**:
  - The repository enforces `pyrefly` (strict mode) and `ruff`.
  - Use modern Python 3.12+ type hints (`list[str]`, `Path`, `str | None`).
  - Use `typing_extensions.Annotated` or `typer.Option` / `typer.Argument` for parameter metadata and descriptions.
- **Entry Points**:
  - Define `app = typer.Typer(...)` or invoke `typer.run(...)`.
  - Handle exit codes cleanly via `raise typer.Exit(code=...)`.
