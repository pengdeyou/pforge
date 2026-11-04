"""Linux Python provisioning for cross platform packaging.

To build a Linux binary on a macOS or Windows host, the compiler needs
a Linux Python to take the ABI details from. That Python cannot be the
host one, so it has to come from somewhere.

The preferred way is the "python" directory that lives next to the
compiler source. It holds the install only builds of the target
platform, laid out like this::

    python/
        linux-x86_64/
            python/
                bin/python3
                include/python3.13/
                lib/libpython3.13.so
                lib/python3.13/

The directory holds only the newest version, since that is the one that
matches the compiler. A directory of this shape can be created with
"pforge --prefetch-python", which downloads the matching build from
the python-build-standalone project.

Since the Linux Python cannot be executed on a macOS or Windows host,
the ABI details are not queried by running it. They are read from the
"sysconfigdata" module of the target Python, which is a plain Python
file that contains all the values that the build of CPython was made
with. That way, no container runtime and no emulation is needed.
"""

import os
import re

from pforge.PythonVersions import python_version_full_str

from .FileOperations import getNormalizedPath, getNormalizedPathJoin


def getLinuxPythonDirectory():
    """Get the "python" directory that holds the target Python builds.

    Returns:
        str
    """
    # This file lives in "pforge/utils", so the directory is two levels
    # up from it.
    return getNormalizedPathJoin(
        os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))),
        "python",
    )


def getLinuxTargetDirectoryName(target_arch="x86_64"):
    """Get the name of the directory for a target architecture.

    Args:
        target_arch: str, "x86_64" or "arm64"

    Returns:
        str
    """
    if target_arch == "arm64":
        return "linux-aarch64"

    return "linux-x86_64"


def getLocalLinuxPythonPrefix(target_arch="x86_64"):
    """Get the prefix of a locally available Linux Python.

    Returns:
        str or None, the prefix path if it exists
    """
    prefix = getNormalizedPathJoin(
        getLinuxPythonDirectory(),
        getLinuxTargetDirectoryName(target_arch),
        "python",
    )

    if not isLinuxPythonPrefixComplete(prefix):
        return None

    return prefix


def isLinuxPythonPrefixComplete(python_prefix):
    """Check if the given prefix contains a usable Linux Python.

    Args:
        python_prefix: str or None

    Returns:
        bool
    """
    if python_prefix is None:
        return False

    if not os.path.isdir(python_prefix):
        return False

    # A usable prefix has a "bin" directory with a Python in it, and
    # the headers to compile against.
    bin_dir = os.path.join(python_prefix, "bin")

    if not os.path.isdir(bin_dir):
        return False

    has_python = any(
        os.path.isfile(os.path.join(bin_dir, name)) for name in ("python3", "python")
    )

    if not has_python:
        return False

    include_dir = os.path.join(python_prefix, "include")

    if not os.path.isdir(include_dir):
        return False

    return True


def getLinuxPythonIncludePath(python_prefix):
    """Get the include directory of a Linux Python.

    The headers are in a version specific sub directory, so it is
    searched for.

    Args:
        python_prefix: str

    Returns:
        str or None
    """
    include_dir = os.path.join(python_prefix, "include")

    if not os.path.isdir(include_dir):
        return None

    candidates = []

    for name in sorted(os.listdir(include_dir)):
        if name.startswith("python"):
            candidates.append(os.path.join(include_dir, name))

    if not candidates:
        return None

    # Prefer the version that matches the compiler.
    preferred = "python%s" % python_version_full_str

    for candidate in candidates:
        if os.path.basename(candidate) == preferred:
            return candidate

    return candidates[-1]


def getLinuxPythonLibraryPath(python_prefix):
    """Get the libpython of a Linux Python.

    Args:
        python_prefix: str

    Returns:
        str or None
    """
    lib_dir = os.path.join(python_prefix, "lib")

    if not os.path.isdir(lib_dir):
        return None

    for name in sorted(os.listdir(lib_dir)):
        if name.startswith("libpython") and ".so" in name:
            return os.path.join(lib_dir, name)

    return None


def _getSysconfigDataFile(python_prefix):
    """Find the sysconfigdata file of a Linux Python.

    That file holds the values that the CPython build was made with,
    and it is a plain Python file, so it can be read without executing
    the target Python.

    Args:
        python_prefix: str

    Returns:
        str or None
    """
    lib_dir = os.path.join(python_prefix, "lib")

    if not os.path.isdir(lib_dir):
        return None

    for name in sorted(os.listdir(lib_dir)):
        if not name.startswith("python"):
            continue

        package_dir = os.path.join(lib_dir, name)

        if not os.path.isdir(package_dir):
            continue

        try:
            entries = sorted(os.listdir(package_dir))
        except OSError:
            continue

        for entry in entries:
            if entry.startswith("_sysconfigdata") and entry.endswith(".py"):
                return os.path.join(package_dir, entry)

    return None


def getLinuxPythonSysConfig(python_prefix):
    """Read the sysconfigdata of a Linux Python.

    The values are read statically, without executing the target
    Python, since it cannot run on a macOS or Windows host.

    Args:
        python_prefix: str

    Returns:
        dict or None, the build time variables of the target Python
    """
    filename = _getSysconfigDataFile(python_prefix)

    if filename is None:
        return None

    with open(filename, "r", encoding="utf-8", errors="ignore") as f:
        content = f.read()

    # The file assigns the values with a "build_time_vars = {...}"
    # dictionary literal, so it is evaluated in an empty namespace,
    # which is safe, since it only contains literals.
    match = re.search(r"build_time_vars\s*=\s*(\{.*?\n\})", content, re.S)

    if match is None:
        return None

    try:
        # The content is a dictionary of literals only, it comes from
        # the target Python's own build configuration.
        return eval(  # pylint: disable=eval-used
            match.group(1), {"__builtins__": {}}, {}
        )
    except Exception:  # pylint: disable=broad-except
        return None


def getLinuxPythonVersionInfo(python_prefix):
    """Get the version information of a Linux Python.

    Args:
        python_prefix: str

    Returns:
        tuple or None
    """
    sys_config = getLinuxPythonSysConfig(python_prefix)

    if sys_config is None:
        return None

    return sys_config.get("VERSION")


def getLinuxPythonCacheDirectory():
    """Get the directory where downloaded Linux Python data is cached."""
    from .AppDirs import getCacheDir

    return getNormalizedPath(getCacheDir("pforge-linux-python", create=True))


#     Part of "Pforge", an optimizing Python compiler that is compatible and
#     integrates with CPython, but also works on its own.
#
#     Licensed under the GNU Affero General Public License, Version 3 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#        https://www.gnu.org/licenses/agpl-3.0.txt
#
#     See also: "Pforge Runtime Library Exception, Version 1.0" in file
#     "LICENSE-RUNTIME.txt" for additional permissions granted under Section 7.
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
