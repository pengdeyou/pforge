"""Cross platform packaging support for Pforge.

Nuitka has a cross compilation framework with ``--target`` that allows
compiling for a target other than the host, but it is highly experimental
and the user experience around it is poor:

- The available targets are only mentioned in a help text, there is no
  way to ask what is actually possible on this machine.
- The preconditions of a target (a WASI Python, a Windows Python, the
  zig compiler) are checked deep inside the build, and the error
  messages do not tell the user how to satisfy them.
- There is no way to build for several targets in one go, which is what
  a release of a Python application usually needs.

This module adds the missing piece on top of the existing target
specifications: a declarative description of the target matrix, a
pre-flight check that tells the user exactly what is missing and how to
get it, and the ability to drive a multi-target build.

The checks here are deliberately non-invasive. They never change how a
build works, they only make the failure mode understandable, and they
allow a build to be requested for a set of targets in one command.
"""

import os

from pforge.utils.Utils import isMacOS, isWin32Windows


class TargetRequirement(object):
    """One requirement that has to be met before a target can be built."""

    def __init__(self, name, description, how_to_satisfy):
        self.name = name
        self.description = description
        self.how_to_satisfy = how_to_satisfy

    def isSatisfied(self):
        """Check whether the requirement is met, return (bool, detail)."""
        raise NotImplementedError

    def getName(self):
        return self.name

    def getDescription(self):
        return self.description

    def getHowToSatisfy(self):
        return self.how_to_satisfy


class CommandRequirement(TargetRequirement):
    """A requirement that an external command needs to be available."""

    def __init__(self, name, description, how_to_satisfy, probe):
        TargetRequirement.__init__(self, name, description, how_to_satisfy)

        self.probe = probe

    def isSatisfied(self):
        from pforge.utils.Execution import getExecutablePath

        path = getExecutablePath(self.probe)

        if path is None:
            return False, "not found in PATH"

        if not os.access(path, os.X_OK):
            return False, "found at '%s' but not executable" % path

        return True, path


class PathRequirement(TargetRequirement):
    """A requirement that a directory or file needs to exist."""

    def __init__(self, name, description, how_to_satisfy, path, want_directory=True):
        TargetRequirement.__init__(self, name, description, how_to_satisfy)

        self.path = path
        self.want_directory = want_directory

    def isSatisfied(self):
        if self.path is None:
            return False, "no path given"

        if self.want_directory:
            if not os.path.isdir(self.path):
                return False, "directory '%s' does not exist" % self.path
        else:
            if not os.path.isfile(self.path):
                return False, "file '%s' does not exist" % self.path

        return True, self.path


class TargetDescription(object):
    """Declarative description of a target that can be built for."""

    def __init__(
        self,
        name,
        description,
        supported_hosts,
        requirements,
        notes=None,
    ):
        self.name = name
        self.description = description
        self.supported_hosts = supported_hosts
        self.requirements = requirements
        self.notes = notes

    def getName(self):
        return self.name

    def getDescription(self):
        return self.description

    def getNotes(self):
        return self.notes

    def isSupportedOnHost(self):
        """Can this host build for this target at all?"""
        from pforge.utils.Utils import getArchitecture

        host = "%s-%s" % (
            "windows" if isWin32Windows() else "macos" if isMacOS() else "linux",
            getArchitecture(),
        )

        return host in self.supported_hosts

    def checkRequirements(self):
        """Check all requirements, return list of (requirement, ok, detail)."""
        result = []

        for requirement in self.requirements:
            ok, detail = requirement.isSatisfied()

            result.append((requirement, ok, detail))

        return result

    def getMissingRequirements(self):
        """Get only the requirements that are not met."""
        return [
            (requirement, detail)
            for requirement, ok, detail in self.checkRequirements()
            if not ok
        ]


def _getZigRequirement():
    from pforge.utils.Execution import getExecutablePath

    return CommandRequirement(
        name="zig",
        description="the zig compiler, used as the C compiler for the target",
        how_to_satisfy="Install zig from https://ziglang.org/download/ and "
        "make sure it is in your PATH.",
        probe=getExecutablePath("zig") or "zig",
    )


def _getWindowsPythonRequirement():
    from pforge.options.Options import getWin32PythonExePath

    return PathRequirement(
        name="windows-python",
        description="a 32-bit Windows Python installation, used to "
        "determine the target Python and its library paths",
        how_to_satisfy="Pass '--win32-python-exe=PATH' pointing at a 32-bit "
        "Windows python.exe, e.g. from winlibs.com.",
        path=getWin32PythonExePath(),
        want_directory=False,
    )


def _getWasiPythonRequirement():
    from pforge.options.Options import getWasiPythonPrefixPath

    # The prefix is only inspected, it is not created here, since that
    # would ask the user for a download while merely listing the
    # targets.
    from pforge.utils.WasiPython import _isWasiPythonPrefixComplete
    from pforge.utils.Download import getDownloadCacheDir
    from pforge.utils.FileOperations import getNormalizedPathJoin
    from pforge.PythonVersions import python_version_str

    prefix = getWasiPythonPrefixPath()

    if prefix is None:
        # It is downloaded on demand, so it is not there yet, unless a
        # previous run has already cached it.
        cache_prefix = getNormalizedPathJoin(
            getDownloadCacheDir(),
            "wasi-python",
            python_version_str,
            "prefix",
        )

        if _isWasiPythonPrefixComplete(cache_prefix, python_version_str):
            prefix = cache_prefix

    return PathRequirement(
        name="wasi-python",
        description="a WASI CPython installation, used as the target Python",
        how_to_satisfy="Let Pforge download a matching prebuilt WASI Python "
        "by omitting '--wasi-python-prefix', or point it at an existing "
        "installation with '--wasi-python-prefix=DIRECTORY'.",
        path=prefix,
        want_directory=True,
    )


def _getZigRequirement():
    from pforge.utils.Execution import getExecutablePath

    path = getExecutablePath("zig")

    return CommandRequirement(
        name="zig",
        description="the zig compiler, used as the C compiler for the target, "
        "it provides the C library and headers of the target platform",
        how_to_satisfy="Install zig from https://ziglang.org/download/ and make "
        "sure it is in your PATH, then use 'pforge --zig' to select it.",
        probe=path if path is not None else "zig",
    )


def _getLinuxPythonRequirement():
    from pforge.utils.LinuxPython import getLocalLinuxPythonPrefix

    prefix = getLocalLinuxPythonPrefix(target_arch="x86_64")

    return PathRequirement(
        name="linux-python",
        description="a Linux Python installation to build against, with "
        "the headers and the shared library of the target",
        how_to_satisfy="Create the 'python' directory next to the compiler "
        "source with 'pforge --prefetch-python', or point at an existing "
        "installation with '--linux-python-prefix=DIRECTORY'.",
        path=prefix,
        want_directory=True,
    )


def _getWasiSDKRequirement():
    # Only the user provided path is inspected, the download is not
    # triggered while merely listing the targets.
    from pforge.options.Options import getWasiSDKPath

    try:
        path = getWasiSDKPath()
    except Exception:  # pylint: disable=broad-except
        path = None

    return PathRequirement(
        name="wasi-sdk",
        description="the WASI SDK, providing the C compiler and sysroot "
        "for the wasm32-wasi target",
        how_to_satisfy="Let Pforge download WASI SDK 20 by omitting "
        "'--wasi-sdk-path', or point it at an existing installation with "
        "'--wasi-sdk-path=DIRECTORY'.",
        path=path,
        want_directory=True,
    )


def _getTargetMatrix():
    """Build the declarative target matrix."""
    from pforge.utils.Utils import getArchitecture

    architecture = getArchitecture()

    targets = []

    targets.append(
        TargetDescription(
            name="native",
            description="the platform this compiler runs on",
            supported_hosts=("windows-x86_64", "macos-x86_64", "macos-arm64", "linux-x86_64"),
            requirements=(),
        )
    )

    targets.append(
        TargetDescription(
            name="wasm32-wasi",
            description="WebAssembly with WASI, for browsers and WASI runtimes",
            supported_hosts=(
                "windows-x86_64",
                "macos-x86_64",
                "macos-arm64",
                "linux-x86_64",
            ),
            requirements=(_getWasiPythonRequirement(), _getWasiSDKRequirement()),
            notes="The target Python and the WASI SDK are downloaded on "
            "demand.",
        )
    )

    targets.append(
        TargetDescription(
            name="i686-w64-mingw32",
            description="32-bit Windows, using the mingw32 target triple",
            supported_hosts=("windows-x86_64", "linux-x86_64"),
            requirements=(_getWindowsPythonRequirement(), _getZigRequirement()),
            notes="On non-Windows hosts the zig compiler is used as the C "
            "compiler.",
        )
    )

    targets.append(
        TargetDescription(
            name="x86_64-linux-gnu",
            description="64-bit Linux, built on a macOS or Windows host",
            supported_hosts=(
                "windows-x86_64",
                "macos-x86_64",
                "macos-arm64",
                "linux-x86_64",
            ),
            requirements=(_getLinuxPythonRequirement(), _getZigRequirement()),
            notes="The Linux Python is taken from the 'python' directory of "
            "the source tree, use 'pforge --prefetch-python' to create it. "
            "The C compiler needs to be able to produce Linux binaries, "
            "which the zig compiler does, use 'pforge --zig' for that.",
        )
    )

    if architecture in ("x86_64", "arm64"):
        targets.append(
            TargetDescription(
                name="macos-universal",
                description="a macOS binary that contains both x86_64 and arm64",
                supported_hosts=("macos-x86_64", "macos-arm64"),
                requirements=(),
                notes="Requires a universal Python installation.",
            )
        )

    return targets


def getTargetMatrix():
    """Get all known targets with their requirement status."""
    return _getTargetMatrix()


def getTargetDescription(target_name):
    """Get one target by name, or None if unknown."""
    for target in _getTargetMatrix():
        if target.getName() == target_name:
            return target

    return None


def getKnownTargetNames():
    """Get the names of all known targets."""
    return [target.getName() for target in _getTargetMatrix()]


def formatTargetReport(target_names=None):
    """Create a human readable report of the target matrix.

    This is what the user sees for "--target-list", and it is also used
    to explain why a particular target cannot be built.
    """
    targets = _getTargetMatrix()

    if target_names:
        wanted = set(target_names)

        targets = [t for t in targets if t.getName() in wanted]

    lines = []

    for target in targets:
        lines.append("Target '%s':" % target.getName())
        lines.append("    %s" % target.getDescription())

        if not target.isSupportedOnHost():
            lines.append("    Status: not supported on this host")
            lines.append("")

            continue

        missing = target.getMissingRequirements()

        if not missing:
            lines.append("    Status: ready")
        else:
            lines.append("    Status: missing prerequisites")
            for requirement, detail in missing:
                lines.append("      - %s: %s" % (requirement.getName(), detail))
                lines.append("        How to satisfy: %s" % requirement.getHowToSatisfy())

        notes = target.getNotes()

        if notes:
            lines.append("    Notes: %s" % notes)

        lines.append("")

    return "\n".join(lines)


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
