"""
Project Configuration & Single Source of Truth.
Reads C and C++ language standards and build configurations from justfile.
"""

import os
import re
from pathlib import Path

WORKSPACE_ROOT: Path = Path(__file__).resolve().parent.parent
JUSTFILE_PATH: Path = WORKSPACE_ROOT / "justfile"


def _extract_from_justfile(var_name: str, default: str) -> str:
    """Extract a variable assignment from justfile using regex."""
    if not JUSTFILE_PATH.is_file():
        return default
    content = JUSTFILE_PATH.read_text(encoding="utf-8")
    match = re.search(rf'^(?:export\s+)?{var_name}\s*:=\s*"([^"]+)"', content, re.MULTILINE)
    if match:
        return match.group(1)
    return default


def get_c_standard() -> str:
    """Return the configured C language standard (e.g. 'c17')."""
    if "C_STD" in os.environ:
        return os.environ["C_STD"]
    return _extract_from_justfile("C_STD", default="c17")


def get_cpp_standard() -> str:
    """Return the configured C++ language standard (e.g. 'c++17')."""
    if "CPP_STD" in os.environ:
        return os.environ["CPP_STD"]
    return _extract_from_justfile("CPP_STD", default="c++17")
