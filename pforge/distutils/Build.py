#     Copyright 2026, Kay Hayen, mailto:kay.hayen@gmail.com find license text at end of file


"""Pforge python -m build integration"""

import contextlib
import os

import setuptools.build_meta

if not hasattr(setuptools.build_meta, "suppress_known_deprecation"):

    @contextlib.contextmanager
    def suppress_known_deprecation():
        yield

else:
    suppress_known_deprecation = setuptools.build_meta.suppress_known_deprecation


# reusing private "build" package code, pylint: disable=protected-access
class PforgeBuildMetaBackend(setuptools.build_meta._BuildMetaBackend):
    def build_wheel(
        self, wheel_directory, config_settings=None, metadata_directory=None
    ):
        # Allow falling back to setuptools when the `build_with_pforge` configuration setting is set to true.
        if config_settings:
            build_with_pforge = config_settings.pop("build_with_pforge", "true").lower()

            if build_with_pforge not in ("true", "false"):
                raise ValueError(
                    "When passing the 'build_with_pforge' setting, it must either be 'true' or 'false'."
                )

            if build_with_pforge == "false":
                return super().build_wheel(
                    wheel_directory, config_settings, metadata_directory
                )

        os.environ["PFORGE_TOML_FILE"] = os.path.join(os.getcwd(), "pyproject.toml")

        with suppress_known_deprecation():
            return self._build_with_temp_dir(
                ["bdist_pforge"], ".whl", wheel_directory, config_settings
            )


_BACKEND = PforgeBuildMetaBackend()

get_requires_for_build_wheel = _BACKEND.get_requires_for_build_wheel
get_requires_for_build_sdist = _BACKEND.get_requires_for_build_sdist
prepare_metadata_for_build_wheel = _BACKEND.prepare_metadata_for_build_wheel
build_wheel = _BACKEND.build_wheel
build_sdist = _BACKEND.build_sdist

LEGACY_EDITABLE = getattr(setuptools.build_meta, "LEGACY_EDITABLE", False)

if not LEGACY_EDITABLE:
    get_requires_for_build_editable = _BACKEND.get_requires_for_build_editable
    prepare_metadata_for_build_editable = _BACKEND.prepare_metadata_for_build_editable
    build_editable = _BACKEND.build_editable

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
