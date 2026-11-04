#     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file


"""Ability to restart Pforge, needed for removing site module effects and using Python PGO after compile.

Note: This avoids imports at all costs, such that initial startup doesn't do more
than necessary.
"""

import os
import sys

from pforge.Tracing import general

from .Execution import callExecProcess


def setLaunchingPforgeProcessEnvironmentValue(environment_variable_name, value):
    # spell-checker: ignore getpid
    os.environ[environment_variable_name] = str(os.getpid()) + ":" + value


def reExecutePforge(pgo_filename):
    # Execute with full path as the process name, so it can find itself and its
    # libraries.
    args = [sys.executable, sys.executable]

    if sys.version_info >= (3, 7) and sys.flags.utf8_mode:
        args += ["-X", "utf8"]

    if sys.version_info >= (3, 11):
        args += ["-X", "frozen_modules=off"]

    if "pforge.__main__" in sys.modules:
        our_filename = sys.modules["pforge.__main__"].__file__
    else:
        our_filename = sys.modules["__main__"].__file__

    args += ["-S", our_filename]

    setLaunchingPforgeProcessEnvironmentValue(
        "PFORGE_BINARY_NAME", sys.modules["__main__"].__file__
    )
    setLaunchingPforgeProcessEnvironmentValue(
        "PFORGE_PACKAGE_HOME",
        os.path.dirname(os.path.abspath(sys.modules["pforge"].__path__[0])),
    )

    if pgo_filename is not None:
        args.append("--pgo-python-input=%s" % pgo_filename)
    else:
        setLaunchingPforgeProcessEnvironmentValue("PFORGE_SYS_PREFIX", sys.prefix)

    # Same arguments as before.
    args += sys.argv[1:]

    from pforge.importing.PreloadedPackages import (
        detectPreLoadedPackagePaths,
        detectPthImportedPackages,
    )

    os.environ["PFORGE_NAMESPACES"] = repr(detectPreLoadedPackagePaths())

    if "site" in sys.modules:
        site_filename = sys.modules["site"].__file__
        if site_filename.endswith(".pyc"):
            site_filename = site_filename[:-4] + ".py"

        setLaunchingPforgeProcessEnvironmentValue("PFORGE_SITE_FILENAME", site_filename)

        # Note: As side effect, this might modify the "sys.path" too.
        os.environ["PFORGE_PTH_IMPORTED"] = repr(detectPthImportedPackages())

        user_site = getattr(sys.modules["site"], "USER_SITE", None)
        if user_site is not None:
            os.environ["PFORGE_USER_SITE"] = repr(user_site)

    setLaunchingPforgeProcessEnvironmentValue("PFORGE_PYTHONPATH", repr(sys.path))

    # In some environments, initial "sys.path" does not contain enough to load
    # "ast" module, which however we use to decode "PFORGE_PYTHONPATH", this
    # helps solve the chicken and egg problem.
    import ast

    setLaunchingPforgeProcessEnvironmentValue(
        "PFORGE_PYTHONPATH_AST", os.path.dirname(ast.__file__)
    )

    if sys.flags.no_site:
        os.environ["PFORGE_NOSITE_FLAG"] = "1"

    os.environ["PYTHONHASHSEED"] = "0"

    setLaunchingPforgeProcessEnvironmentValue("PFORGE_RE_EXECUTION", "1")

    # Does not return:
    callExecProcess(
        args=args,
        shell=False,
        logger=general,
    )


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
