"""Benchmark of the compiler start-up time.

This measures the time it takes to import the compiler and get to the
point where the command line options are parsed, which is what a user
experiences for every single invocation.

It is used to verify that changes to the import structure do not slow
the compiler down.

Run it with:

    python -m pforge.optimizations.bench_startup
"""

import os
import subprocess
import sys
import time

REPEATS = 5


def measureOne(python_path, package_name):
    """Measure the start-up time for one package, in seconds."""
    code = (
        "import sys; sys.path.insert(0, %r); "
        "sys.argv=['x', '--version']; "
        "from %s.__main__ import main; "
        "exec('try:\\n main()\\nexcept BaseException:\\n pass')"
        % (
            python_path,
            package_name,
        )
    )

    times = []

    for _ in range(REPEATS):
        start = time.perf_counter()
        subprocess.run(
            [sys.executable, "-c", code],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
        )
        times.append(time.perf_counter() - start)

    times.sort()

    return times[len(times) // 2]


def main():
    here = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

    print("  Compiler start-up (median of %d runs):" % REPEATS)

    for package_name in ("pforge",):
        seconds = measureOne(here, package_name)
        print("    %-10s %8.1f ms" % (package_name, seconds * 1000))


if __name__ == "__main__":
    main()
