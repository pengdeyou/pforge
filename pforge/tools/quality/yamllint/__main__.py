#!/usr/bin/env python
#     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file


"""Main program for yamllint checker tool."""

import os
import sys

# Unchanged, running from checkout, use the parent directory, the pforge
# package ought be there.
sys.path.insert(
    0, os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
)

# isort:start

from pforge.format.YamlChecker import checkYamlSchema
from pforge.options.CommandLineOptionsTools import makeOptionsParser
from pforge.tools.Basics import goHome
from pforge.tools.quality.ScanSources import scanTargets
from pforge.Tracing import my_print, tools_logger
from pforge.utils.FileOperations import resolveShellPatternToFilenames


def main():
    parser = makeOptionsParser(usage="%prog [options]", epilog=None)

    parser.add_option(
        "--update-checksum",
        dest="update_checksum",
        action="store_true",
        default=False,
        help="""\
Update the version checksum after checking, so Pforge knowns it can be trusted.""",
    )

    parser.add_option(
        "--assume-yes-for-downloads",
        action="store_true",
        dest="assume_yes_for_downloads",
        default=False,
        help="""\
Allow download and execution of tools if needed. Default is %default.""",
    )

    options, positional_args = parser.parse_args()

    if not positional_args:
        positional_args = [
            "pforge/plugins/standard/*.yml",
            "pforge/plugins/commercial/*.yml",
        ]

    my_print("Working on:", positional_args)

    positional_args = sum(
        (
            resolveShellPatternToFilenames(positional_arg)
            for positional_arg in positional_args
        ),
        [],
    )

    goHome()

    filenames = list(
        scanTargets(
            positional_args,
            suffixes=(".yaml",),
        )
    )
    if not filenames:
        tools_logger.sysexit("No files found.")

    for filename in filenames:
        checkYamlSchema(
            filename=filename,
            effective_filename=filename,
            update=options.update_checksum,
            assume_yes_for_downloads=options.assume_yes_for_downloads,
            logger=tools_logger,
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
