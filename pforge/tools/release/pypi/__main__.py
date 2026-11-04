#!/usr/bin/python -u
#     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file


"""Make PyPI upload of Pforge, and check success of it."""

import os
import shutil

from pforge.options.CommandLineOptionsTools import makeOptionsParser
from pforge.tools.environments.Virtualenv import withVirtualenv
from pforge.tools.release.Documentation import checkReleaseDocumentation
from pforge.tools.release.Release import (
    checkBranchName,
    makePforgeSourceDistribution,
)
from pforge.Tracing import tools_logger
from pforge.utils.InstalledPythons import findInstalledPython
from pforge.Version import getPforgeVersion


def _checkPforgeInVirtualenv(python):
    with withVirtualenv(
        "venv_pforge", logger=tools_logger, style="blue", python=python.getPythonExe()
    ) as venv:
        tools_logger.info("Installing Pforge into virtualenv:", style="blue")
        tools_logger.info("*" * 40, style="blue")
        venv.runCommand("python -m pip install ../dist/pforge*.tar.gz")
        tools_logger.info("*" * 40, style="blue")

        tools_logger.info("Compiling basic test with runner:", style="blue")
        tools_logger.info("*" * 40, style="blue")
        venv.runCommand(
            (
                "pforge%s ../tests/basics/AssertsTest.py"
                % ("2" if python.getPythonVersion()[0] == "2" else "")
            ),
            style="blue",
        )
        tools_logger.info("*" * 40, style="blue")

        tools_logger.info(
            "Compiling basic test with recommended -m mode:", style="blue"
        )
        tools_logger.info("*" * 40, style="blue")
        venv.runCommand(
            "python -m pforge ../tests/basics/AssertsTest.py",
            style="blue",
        )
        tools_logger.info("*" * 40, style="blue")


def main():
    pforge_version = getPforgeVersion()

    branch_name = checkBranchName()

    parser = makeOptionsParser(usage=None, epilog=None)

    parser.add_option(
        "--token",
        action="store",
        dest="token",
        help="""
Token to use for upload.
""",
    )

    parser.add_option(
        "--check",
        action="store_true",
        dest="check",
        help="""
Check if it would build, without uploading.
""",
    )

    options, positional_args = parser.parse_args()

    if positional_args:
        return tools_logger.sysexit(
            "This command takes no positional arguments, check help output."
        )

    # Only real main releases so far.
    if not options.check:
        assert branch_name == "main", branch_name
        assert "pre" not in pforge_version and "rc" not in pforge_version

    tools_logger.info("Working on Pforge %r." % pforge_version, style="blue")

    shutil.rmtree("check_pforge", ignore_errors=True)
    shutil.rmtree("dist", ignore_errors=True)

    tools_logger.info("Creating documentation.", style="blue")
    checkReleaseDocumentation()
    tools_logger.info("Creating source distribution.", style="blue")

    # spell-checker: ignore gztar
    dist_filenames = makePforgeSourceDistribution(
        formats=("gztar",),
        sign=False,
    )

    # Test with these Pythons if the installed package would work.
    required_python_versions = ("2.7", "3.10")

    pythons = []
    for python_version in required_python_versions:
        python = findInstalledPython(
            python_versions=python_version,
            module_specs=None,
        )

        if python is None:
            return tools_logger.sysexit(
                "Error, no installed Python found for version(s) %s." % python_version
            )

        pythons.append(python)

    for python in pythons:
        _checkPforgeInVirtualenv(python)

    assert os.system("twine check %s" % dist_filenames[0]) == 0

    if not options.check:
        tools_logger.info("Uploading source dist")

        assert (
            os.system(
                "twine upload --username=__token__ --password=%s %s"
                % (
                    options.token,
                    dist_filenames[0],
                )
            )
            == 0
        )
        tools_logger.info("Uploaded.")
    else:
        tools_logger.info("Checked OK, but not uploaded.")


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
