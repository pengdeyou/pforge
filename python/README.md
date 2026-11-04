# Target Python builds

This directory holds the Python installations of the target platforms,
so that Pforge can build binaries for a platform other than the one it
runs on.

Only the newest version is kept, since that is the one that matches the
compiler. The version that is used is adapted automatically, so that a
newer release of the target Python does not break the build.

## Layout

    python/
        linux-x86_64/
            python/
                bin/python3
                include/python3.13/
                lib/libpython3.13.so
                lib/python3.13/

The `python` directory inside the target directory is the prefix of the
installation, and it holds the headers, the shared library, and the
standard library of the target.

## Creating it

Use the prefetch option, which downloads the matching build from the
`python-build-standalone` project:

```console
$ pforge --prefetch-python
```

This fills the `linux-x86_64` directory. It is not checked into the
repository, since it is large, and it can be recreated at any time.

## Why this is needed

The compiler needs the headers and the shared library of the target
Python, and it needs to know the ABI values that the target Python was
built with. Those values are read from the `sysconfigdata` module of the
target Python, which is a plain Python file, so no execution of the
target Python is needed. That is what makes it possible to build a
Linux binary on a macOS host without a container or an emulation
layer.

## Using it

Once the directory exists, the Linux target can be used:

```console
$ pforge --mode=standalone --target=linux main.py
```

Use `pforge --target-list` to see which targets are available and what
is still missing for them.
