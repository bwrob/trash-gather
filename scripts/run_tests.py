"""Test runner wrapper with colorful summary, verbose failure capture, and timestamped logs."""

import os
import re
import subprocess
import sys
from datetime import datetime
from pathlib import Path

# ANSI Escape Sequences
USE_COLOR = sys.stdout.isatty() and not os.environ.get("NO_COLOR")

RED = "\033[1;31m" if USE_COLOR else ""
GREEN = "\033[1;32m" if USE_COLOR else ""
YELLOW = "\033[1;33m" if USE_COLOR else ""
CYAN = "\033[1;36m" if USE_COLOR else ""
BOLD = "\033[1m" if USE_COLOR else ""
DIM = "\033[90m" if USE_COLOR else ""
RESET = "\033[0m" if USE_COLOR else ""

TEST_PATTERN = re.compile(r"^([a-zA-Z0-9_\-\/]+)\s+\[\s*(OK|ERROR|FAIL|SKIP)\s*\]")
SUMMARY_PATTERN = re.compile(r"^(\d+)\s+of\s+(\d+)\s+\((\d+)%\)\s+tests\s+successful")


def _flush_failure_log(
    failures_dir: Path,
    test_name: str,
    timestamp: str,
    seed: str,
    lines: list[str],
) -> Path:
    """Write recorded failure output lines with full diagnostic header to timestamped log."""
    safe_name = test_name.replace("/", "_")
    filename = f"{timestamp}_{safe_name}.log"
    log_path = failures_dir / filename

    header = [
        "================================================================================\n",
        f"TEST FAILURE:  {test_name}\n",
        f"TIMESTAMP:     {datetime.now().isoformat()}\n",
        f"PRNG SEED:     {seed}\n",
        f"REPRO COMMAND: ./bin/test_runner {test_name}\n",
        "================================================================================\n\n",
    ]

    with log_path.open("w", encoding="utf-8") as log_file:
        log_file.writelines(header)
        log_file.writelines(lines)
    return log_path


def main() -> int:
    """Run unit test binary, stream failures to .failures/, and print summary."""
    bin_path = Path("bin/test_runner")
    if not bin_path.is_file():
        sys.stderr.write(f"{RED}Error: test runner not found at {bin_path}{RESET}\n")
        return 1

    failures_dir = Path(".failures").resolve()
    failures_dir.mkdir(parents=True, exist_ok=True)
    run_timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

    # Pass verbose logging options to munit if not already provided
    user_args = sys.argv[1:]
    extra_args = []
    if "--log-visible" not in user_args:
        extra_args.extend(["--log-visible", "debug"])

    cmd = [str(bin_path), *extra_args, *user_args]

    env = os.environ.copy()
    env.setdefault(
        "ASAN_OPTIONS",
        "print_stacktrace=1:check_initialization_order=1:detect_stack_use_after_return=1:verbosity=0",
    )
    env.setdefault("UBSAN_OPTIONS", "print_stacktrace=1")

    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        env=env,
    )

    current_test_name: str | None = None
    current_lines: list[str] = []
    failed_tests: list[tuple[str, Path]] = []
    summary_info: tuple[int, int, int] | None = None
    current_seed: str = "unknown"

    if proc.stdout is not None:
        for line in proc.stdout:
            stripped = line.strip()

            if stripped.startswith("Running test suite with seed "):
                current_seed = stripped.split("with seed ")[-1].rstrip(".")
                print(f"{DIM}🌱 Test suite seeded with {current_seed}{RESET}\n")
                continue

            test_match = TEST_PATTERN.match(stripped)
            if test_match:
                if current_test_name is not None and current_lines:
                    log_path = _flush_failure_log(
                        failures_dir,
                        current_test_name,
                        run_timestamp,
                        current_seed,
                        current_lines,
                    )
                    failed_tests.append((current_test_name, log_path))
                    current_test_name = None
                    current_lines = []

                t_name, t_status = test_match.groups()
                if t_status in ("ERROR", "FAIL"):
                    current_test_name = t_name
                    current_lines = [line]
                continue

            sum_match = SUMMARY_PATTERN.match(stripped)
            if sum_match:
                if current_test_name is not None and current_lines:
                    log_path = _flush_failure_log(
                        failures_dir,
                        current_test_name,
                        run_timestamp,
                        current_seed,
                        current_lines,
                    )
                    failed_tests.append((current_test_name, log_path))
                    current_test_name = None
                    current_lines = []
                passed, total, pct = sum_match.groups()
                summary_info = (int(passed), int(total), int(pct))
                continue

            if current_test_name is not None:
                current_lines.append(line)

    if current_test_name is not None and current_lines:
        log_path = _flush_failure_log(
            failures_dir,
            current_test_name,
            run_timestamp,
            current_seed,
            current_lines,
        )
        failed_tests.append((current_test_name, log_path))

    return_code = proc.wait()

    # Print decorative summary block
    print("\n" + "═" * 80)
    print(f"{BOLD}📊 Test Suite Summary{RESET}")
    print("═" * 80)

    if failed_tests:
        passed_count = summary_info[0] if summary_info else 0
        total_count = summary_info[1] if summary_info else len(failed_tests)
        pct = (
            summary_info[2]
            if summary_info
            else int((passed_count / total_count) * 100 if total_count else 0)
        )
        fail_count = len(failed_tests)

        print(
            f"{RED}❌ {fail_count} test(s) failed{RESET} "
            f"({passed_count}/{total_count} passed, {pct}%)\n"
        )
        print(f"{BOLD}Failure Logs ({run_timestamp}):{RESET}")
        for t_name, log_path in failed_tests:
            print(f"  {RED}✖{RESET} {BOLD}{t_name}{RESET}")
            print(f"    {CYAN}↳ file://{log_path}{RESET}")
    else:
        total_count = summary_info[1] if summary_info else 0
        print(f"{GREEN}✅ All {total_count} tests passed cleanly! (100%){RESET}")

    print("═" * 80 + "\n")
    return return_code


if __name__ == "__main__":
    sys.exit(main())
