"""Benchmark for the tag set hot path of the optimizer.

The optimizer signals changes with tags on every optimization step, and
these signals are checked against the set of allowed tags. This benchmark
measures that path in isolation, so improvements can be verified without
having to compile a large code base.

Run it with:

    python -m pforge.optimizations.bench_tags
"""

import timeit

from pforge.optimizations.Tags import TagSet, allowed_tags


def makeSignal():
    """Create a signal similar to the ones used in real optimization."""
    return " ".join(allowed_tags[:4])


def runCheckOnly(count):
    """Membership check only, without the set update."""
    allowed = allowed_tags
    tag_set = TagSet()

    for _ in range(count):
        for tag in allowed_tags:
            tag_set.add(tag)


def runSignal(count):
    """The full signal path, as used by the optimizer."""
    tag_set = TagSet()
    signal = makeSignal()

    for _ in range(count):
        tag_set.onSignal(signal)


def main():
    count = 200000

    for name, func in (("add (per tag)", runCheckOnly), ("onSignal", runSignal)):
        seconds = timeit.timeit(lambda: func(count), number=1)

        print(
            "  %-16s %8.1f ms for %d rounds (%.3f us per round)"
            % (name, seconds * 1000, count, seconds * 1e6 / count)
        )


if __name__ == "__main__":
    main()
