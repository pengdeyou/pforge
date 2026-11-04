#     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file


"""OpenSUSE Build Service (OSC) upload release tool.

Uploads Pforge branches adapting the RPM configuration to the different
projects on OSC.
"""

import os
import shutil
import sys

from pforge.tools.environments.Virtualenv import withVirtualenv
from pforge.tools.release.Documentation import checkReleaseDocumentation
from pforge.tools.release.Release import (
    checkBranchName,
    cleanupSourceDistributionState,
)
from pforge.Tracing import tools_logger
from pforge.utils.InstalledPythons import findInstalledPython
from pforge.Version import getPforgeVersion


def main():
    pforge_version = getPforgeVersion()
    branch_name = checkBranchName()

    shutil.rmtree("dist", ignore_errors=True)
    shutil.rmtree("build", ignore_errors=True)

    cleanupSourceDistributionState()

    checkReleaseDocumentation()
    # spell-checker: ignore gztar
    assert os.system("%s setup.py sdist --formats=gztar" % sys.executable) == 0

    # Upload stable releases to OpenSUSE Build Service:
    if (
        branch_name.startswith("release")
        or branch_name.startswith("hotfix")
        or branch_name == "main"
    ):
        osc_project_name = "Pforge"
        spec_suffix = ""
    elif branch_name == "develop":
        osc_project_name = "Pforge-Unstable"
        spec_suffix = "-unstable"
    elif branch_name == "factory":
        osc_project_name = "Pforge-experimental"
        spec_suffix = "-experimental"
    else:
        sys.exit("Skipping OSC for branch '%s'" % branch_name)

    installed_python = findInstalledPython(
        python_versions=("3.10",),
        module_specs=None,
    )

    with withVirtualenv(
        "venv_pforge",
        logger=tools_logger,
        style="blue",
        python=installed_python.getPythonExe(),
    ) as venv:
        venv.runCommand("python -m pip install osc 'setuptools<68'")

        # Stage the "osc" checkout from the ground up,
        # spell-checker: ignore addremove,kayhayen,rpmlintrc
        venv.runCommand(
            """\
rm -rf osc && mkdir osc && cd osc && \
osc checkout home:kayhayen %(osc_project_name)s && \
rm home:kayhayen/%(osc_project_name)s/* && \
cp ../dist/Pforge-*.tar.gz home:kayhayen/%(osc_project_name)s/ && \
sed -e s/PROJECT_VERSION/%(pforge_version)s/ ../pforge/tools/release/rpm/pforge.spec \
    >home:kayhayen/%(osc_project_name)s/pforge%(spec_suffix)s.spec && \
sed -i home:kayhayen/%(osc_project_name)s/pforge%(spec_suffix)s.spec -e \
    's/Name: *PROJECT_NAME/Name: pforge%(spec_suffix)s/' && \
cp ../pforge/tools/release/rpm/pforge-rpmlintrc home:kayhayen/%(osc_project_name)s/ && \
cd home:kayhayen/%(osc_project_name)s/ && \
osc addremove -r && \
echo 'New release' >ci_message && \
osc ci --file ci_message
"""
            % {
                "pforge_version": pforge_version,
                "osc_project_name": osc_project_name,
                "spec_suffix": spec_suffix,
            },
            keep_cwd=True,
        )


if __name__ == "__main__":
    main()

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
