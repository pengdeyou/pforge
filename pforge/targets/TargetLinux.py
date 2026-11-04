"""Target specification for Linux, to allow cross platform packaging.

Nuitka can compile for the platform it runs on, and has experimental
support for 32-bit Windows and WebAssembly. What is missing is the
ability to build a Linux binary on a macOS or Windows host, which is
exactly what a release of a Python application needs, since Linux is
where the application usually runs, while the developer works on macOS.

This module provides the target specification for that case.

The ABI details of the target are not queried by running a Linux
Python, because that cannot be done on a macOS or Windows host.
Instead they are read statically from the "sysconfigdata" of the Linux
Python in the "python" directory, which contains all the values that
the CPython build was made with. This means that no container runtime
and no emulation is needed for the ABI queries.
"""

import os

from pforge.options.Options import getLinuxPythonPrefixPath
from pforge.PythonVersions import getImportlibSubPackages
from pforge.Tracing import general
from pforge.utils.LinuxPython import (
    getLinuxPythonIncludePath,
    getLinuxPythonLibraryPath,
    getLinuxPythonVersionInfo,
    getLocalLinuxPythonPrefix,
)
from pforge.utils.Utils import isWin32Windows

from .Targets import TargetBase


class TargetLinux(TargetBase):
    """Target specification for the Linux ABI."""

    def __init__(self, name="x86_64-linux-gnu", target_arch="x86_64"):
        TargetBase.__init__(
            self,
            name=name,
            # These are the values of the Linux ABI on 64 bit
            # architectures, where "long" and "long long" are both 64
            # bits wide, and pointers are 64 bits wide as well.
            c_long_size=8,
            c_longlong_size=8,
            c_pointer_size=8,
            target_os="Linux",
            target_arch=target_arch,
            binary_format="elf",
            rpath_using=True,
            python_digit_bits=30,
        )

        self._python_prefix = None

    def _getPythonPrefix(self):
        """Get the prefix of the Linux Python to build against."""
        if self._python_prefix is None:
            # The user provided one wins, then the local "python"
            # directory of the source tree, and that is it.
            python_prefix = getLinuxPythonPrefixPath()

            if python_prefix is None:
                python_prefix = getLocalLinuxPythonPrefix(
                    target_arch=self.target_arch
                )

            self._python_prefix = python_prefix

        return self._python_prefix

    def getPythonPrefix(self):
        """Return the Linux Python prefix used by this target."""
        return self._getPythonPrefix()

    def getPythonDLLPath(self):
        """Return the Python shared library path used by this target."""
        python_prefix = self._getPythonPrefix()

        if python_prefix is None:
            return None

        return getLinuxPythonLibraryPath(python_prefix)

    def getSystemStaticLibPythonPath(self, python_debug):
        """Return the Linux static library path, if available."""
        python_prefix = self._getPythonPrefix()

        if python_prefix is None:
            return None

        # The static library is only in the "install_only" builds when
        # they are made with it, otherwise the shared one is used.
        for name in sorted(os.listdir(os.path.join(python_prefix, "lib"))):
            if name.startswith("libpython") and name.endswith(".a"):
                return os.path.join(python_prefix, "lib", name)

        return None

    def couldUseStaticLibPython(self, python_debug, dll_mode):
        """Return whether the Linux Python can use a static libpython."""
        return self.getSystemStaticLibPythonPath(python_debug) is not None

    def translateHostPathToTarget(self, host_path):
        """Translate a host path to a path usable by the target."""
        # Linux paths have the same layout on all hosts, only the path
        # separator differs, and that is handled by the compiler itself.
        return host_path

    def translateTargetPathToHost(self, target_path):
        """Translate a target path to a path usable on this host."""
        if os.path.exists(target_path):
            return target_path

        python_prefix = self._getPythonPrefix()

        if python_prefix is None:
            return target_path

        # The target Python lives in a directory of the source tree, so
        # its paths are already valid on this host.
        if target_path.startswith(python_prefix):
            return target_path

        return target_path

    def getTargetRunCommand(self, executable_path):
        """Return the command to run a Linux executable on this host."""
        if not isWin32Windows():
            return (executable_path,)

        # On Windows, a Linux binary needs a user mode Linux layer.
        return general.sysexit(
            "Error, running a Linux target requires a Linux host, use WSL2."
        )

    def getPythonSconsOptions(self):
        """Return the Linux Python paths for Scons."""
        python_prefix = self._getPythonPrefix()

        if python_prefix is None:
            return {}

        include_path = getLinuxPythonIncludePath(python_prefix)

        options = {
            "python_prefix": python_prefix,
        }

        if include_path is not None:
            options["python_include_path"] = include_path

        return options

    def checkPreconditions(self):
        """Check that the Linux target can be built on this host."""

        python_prefix = self._getPythonPrefix()

        if python_prefix is None:
            return general.sysexit("""\
Error, the Linux target needs a Linux Python to build against. Create the \
'python' directory next to the compiler source with 'pforge \
--prefetch-python', or point it at an existing installation with \
'--linux-python-prefix=DIRECTORY'.""")

        if not os.path.isdir(python_prefix):
            return general.sysexit(
                "Error, the Linux Python prefix '%s' does not exist."
                % python_prefix
            )

        include_path = getLinuxPythonIncludePath(python_prefix)

        if include_path is None:
            return general.sysexit(
                "Error, the Linux Python at '%s' has no include directory."
                % python_prefix
            )

        version_info = getLinuxPythonVersionInfo(python_prefix)

        if version_info is None:
            return general.sysexit(
                "Error, the Linux Python at '%s' has no sysconfigdata, so "
                "its ABI cannot be determined." % python_prefix
            )

        from pforge.PythonVersions import python_version_str

        target_version = ".".join(str(part) for part in version_info[:2])

        if target_version != python_version_str:
            return general.sysexit("""\
Error, the Linux Python is a Python %s, but Pforge is running with Python %s. \
The versions must match, since the compiler and the target Python have to \
agree on the ABI.""" % (target_version, python_version_str))

    def evaluateCompileTimeExpression(self, expression, setup_code):
        """Evaluate an expression for the target.

        The Linux Python cannot be executed on this host, so the
        expression is evaluated with the host Python. The values that
        are used in these expressions are about the module system and
        the Python version, which are the same for the target.
        """
        namespace = {"__builtins__": __builtins__}

        if setup_code:
            exec(setup_code, namespace)  # pylint: disable=exec-used

        return eval(expression, namespace, {})  # pylint: disable=eval-used

    def getPythonVersionInfo(self):
        """Return the target Python's version information."""
        python_prefix = self._getPythonPrefix()

        if python_prefix is None:
            return None

        return getLinuxPythonVersionInfo(python_prefix)

    def getModuleSubPackages(self, module_name):
        """Return importable subpackages for a module on the target."""
        assert module_name == "importlib", module_name
        return getImportlibSubPackages()

    def canImportModule(self, module_name):
        """Return whether a module is importable on the target."""
        try:
            from importlib.util import find_spec
        except ImportError:
            from pkgutil import get_loader

            return get_loader(module_name) is not None

        try:
            return find_spec(module_name) is not None
        except ImportError:
            return False

    @staticmethod
    def isHardModuleSupported(module_name):
        """Return whether a hard module is available for this target."""
        # pylint: disable=unused-argument
        return True


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
