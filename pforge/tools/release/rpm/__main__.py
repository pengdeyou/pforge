#     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file


"""OpenSUSE Build Service (OSC) upload release tool.

Uploads Pforge branches adapting the RPM configuration to the different
projects on OSC.
"""

import os
import shutil
import sys

from pforge.tools.Basics import goHome
from pforge.tools.release.Release import (
    checkBranchName,
    cleanupSourceDistributionState,
)
from pforge.Tracing import tools_logger
from pforge.utils.FileOperations import (
    copyFile,
    getFileContents,
    makePath,
    putTextFileContents,
    renameFile,
    withDirectoryChange,
)
from pforge.Version import getPforgeVersion


def main():
    goHome()

    pforge_version = getPforgeVersion()
    branch_name = checkBranchName()

    shutil.rmtree("build", ignore_errors=True)
    makePath("build")

    cleanupSourceDistributionState()

    # Used by rpmbuild, spell-checker: ignore rpmbuild
    makePath(os.path.expanduser("~/rpmbuild/SOURCES"))

    # Upload stable releases to OpenSUSE Build Service:
    if (
        branch_name.startswith("release")
        or branch_name.startswith("hotfix")
        or branch_name == "main"
    ):
        rpm_project_name = "Pforge"
        spec_suffix = ""
    elif branch_name == "develop":
        rpm_project_name = "Pforge-Develop"
        spec_suffix = "-develop"
    elif branch_name == "factory":
        rpm_project_name = "Pforge-Factory"
        spec_suffix = "-factory"
    else:
        return tools_logger.sysexit("Skipping RPM build for branch '%s'" % branch_name)

    with withDirectoryChange("build"):
        tools_logger.info(
            "Building source distribution for %s %s"
            % (rpm_project_name, pforge_version)
        )
        # spell-checker: ignore gztar,rpmlintrc
        assert os.system("%s ../setup.py sdist --formats=gztar" % sys.executable) == 0
        pforge_dist_filename = "Pforge%s-%s.tar.gz" % (spec_suffix, pforge_version)
        renameFile(
            source_filename=os.path.join("..", "dist", pforge_dist_filename),
            dest_filename=os.path.join("/root/rpmbuild/SOURCES", pforge_dist_filename),
        )

        copyFile(
            source_path=os.path.join(os.path.dirname(__file__), "pforge-rpmlintrc"),
            dest_path="/root/rpmbuild/SOURCES/pforge-rpmlintrc",
        )

        # Adapt the spec file dynamically to version and project name of Pforge being built.
        spec_contents = getFileContents(
            os.path.join(os.path.dirname(__file__), "pforge.spec")
        )
        spec_contents = spec_contents.replace("PROJECT_NAME", "pforge" + spec_suffix)
        spec_contents = spec_contents.replace("PROJECT_VERSION", pforge_version)

        spec_filename = "pforge%s.spec" % spec_suffix
        putTextFileContents(
            filename=spec_filename,
            contents=spec_contents,
        )

        os.system("rpmbuild --target x86_64 -bb %s" % spec_filename)


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
