#     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file


"""Pforge version related stuff."""

version_string = """\
Pforge V1.0.0
Copyright (C) 2026 Kay Hayen."""


def getPforgeVersion():
    """Return Pforge version as a string.

    This should not be used for >= comparisons directly.
    """
    return version_string.split()[1][1:]


# Sanity check.
assert getPforgeVersion()[-1].isdigit(), getPforgeVersion()


def parsePforgeVersionToTuple(version):
    """Return Pforge version as a tuple.

    This can also not be used for precise comparisons, even with rc versions,
    but it's not actually a version.
    """

    if "rc" in version:
        rc_number = int(version[version.find("rc") + 2 :] or "0")
        version = version[: version.find("rc")]

        is_final = False
    else:
        rc_number = 0
        is_final = True

    result = version.split(".")
    if len(result) == 2:
        result.append("0")

    result = [int(digit) for digit in result]
    result.extend((is_final, rc_number))
    return tuple(result)


def getPforgeVersionTuple():
    """Return Pforge version as a tuple.

    This can also not be used for precise comparisons, even with rc versions,
    but it's not actually a version. The format is used what is used for
    "__compiled__" values.
    """

    return parsePforgeVersionToTuple(version=getPforgeVersion())


def getPforgeVersionYear():
    """The year of Pforge copyright for use in generations."""

    return int(version_string.split()[4])


def getCommercialVersion():
    """Return Pforge commercial version if installed."""
    try:
        from pforge.tools.commercial import Version
    except ImportError:
        return None
    else:
        return Version.__version__


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
