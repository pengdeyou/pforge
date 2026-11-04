"""Prefetching of target Python builds.

The "python" directory of the source tree holds the install only
builds of the target platforms, so that a Linux binary can be built on
a macOS host without needing a container runtime or an emulation
layer.

Only the newest version is kept, since that is the one that matches
the compiler, and older ones would only take up space.
"""

import os
import shutil
import tarfile

from pforge.PythonVersions import python_version_full_str
from pforge.Tracing import general

from .FileOperations import getNormalizedPathJoin, makePath, removeDirectory
from .LinuxPython import (
    getLinuxPythonDirectory,
    getLinuxTargetDirectoryName,
    isLinuxPythonPrefixComplete,
)

# The python-build-standalone project provides prebuilt CPython
# installations for many platforms, which is what is used here.
_PBS_RELEASE_URL = "https://github.com/astral-sh/python-build-standalone/releases"

_TARGET_TRIPLES = {
    "x86_64": "x86_64-unknown-linux-gnu",
    "arm64": "aarch64-unknown-linux-gnu",
}


def getPBSReleaseTag():
    """Get the tag of the newest python-build-standalone release.

    Returns:
        str or None
    """
    from .Download import withUrlOpen

    try:
        with withUrlOpen(
            _PBS_RELEASE_URL + "/latest",
            request_headers=None,
            timeout=30,
            allow_http_fallback=False,
        ) as response:
            url = response.geturl()
    except Exception as e:  # pylint: disable=broad-except
        general.info("Could not determine the newest release: %s" % e)
        return None

    # The redirect ends in "/tag/<tag>".
    if "/tag/" not in url:
        return None

    return url.rsplit("/tag/", 1)[1]


def getPBSAssetName(target_arch, release_tag, python_version=None):
    """Get the name of the install only asset for a target.

    Args:
        target_arch: str, "x86_64" or "arm64"
        release_tag: str, the release tag of python-build-standalone
        python_version: str or None, the Python version to use

    Returns:
        str or None
    """
    target_triple = _TARGET_TRIPLES.get(target_arch)

    if target_triple is None:
        return None

    if python_version is None:
        python_version = python_version_full_str

    return "cpython-%s+%s-%s-install_only.tar.gz" % (
        python_version,
        release_tag,
        target_triple,
    )


def getAvailablePythonVersions(release_tag):
    """Get the Python versions that a release provides for a target.

    The release does not necessarily provide the exact version that the
    compiler runs with, only the newest one of the same minor version
    usually, so the available ones are looked up.

    The release listing is large, and the response is often truncated
    while being read, so instead of parsing it, the candidate versions
    are probed directly with a request for their headers only.

    Args:
        release_tag: str, the release tag of python-build-standalone

    Returns:
        list of str, the Python versions available for Linux x86_64
    """
    from .Download import withUrlOpen

    minor_version = ".".join(python_version_full_str.split(".")[:2])

    versions = []

    for patch in range(30, -1, -1):
        version = "%s.%d" % (minor_version, patch)

        url = getPBSAssetURL(
            target_arch="x86_64",
            release_tag=release_tag,
            python_version=version,
        )

        try:
            with withUrlOpen(
                url,
                request_headers={"Range": "bytes=0-0"},
                timeout=30,
                allow_http_fallback=False,
            ) as response:
                if response.getcode() not in (200, 206):
                    continue
        except Exception:  # pylint: disable=broad-except
            continue

        versions.append(version)

    def versionKey(value):
        return tuple(int(part) for part in value.split("."))

    return sorted(versions, key=versionKey, reverse=True)


def getBestPythonVersion(release_tag):
    """Get the Python version to use for a target.

    The version that the compiler runs with is preferred, but if the
    release does not provide it, the newest version of the same minor
    version is used, since that is ABI compatible.

    Args:
        release_tag: str, the release tag of python-build-standalone

    Returns:
        str or None
    """
    available = getAvailablePythonVersions(release_tag)

    if not available:
        # Without the listing, the version of the compiler is the best
        # guess that can be made.
        return python_version_full_str

    if python_version_full_str in available:
        return python_version_full_str

    # Prefer the same minor version, since the ABI is the same.
    minor_version = ".".join(python_version_full_str.split(".")[:2])

    for version in available:
        if version.startswith(minor_version + "."):
            return version

    return available[0]


def getPBSAssetURL(target_arch, release_tag, python_version=None):
    """Get the download URL of the install only asset for a target.

    Args:
        target_arch: str, "x86_64" or "arm64"
        release_tag: str, the release tag of python-build-standalone
        python_version: str or None, the Python version to use

    Returns:
        str or None
    """
    asset_name = getPBSAssetName(
        target_arch=target_arch,
        release_tag=release_tag,
        python_version=python_version,
    )

    if asset_name is None:
        return None

    return "%s/download/%s/%s" % (
        _PBS_RELEASE_URL,
        release_tag,
        asset_name.replace("+", "%2B"),
    )


def prefetchLinuxPython(target_arch="x86_64", python_version=None):
    """Download the Linux Python into the "python" directory.

    Only the newest version is kept, so an existing installation of a
    different version is replaced.

    Args:
        target_arch: str, "x86_64" or "arm64"
        python_version: str or None, the Python version to use

    Returns:
        str, the prefix that was created
    """
    from pforge.options.Options import assumeYesForDownloads

    from .Download import getCachedDownload, getDownloadMode

    release_tag = getPBSReleaseTag()

    if release_tag is None:
        general.sysexit(
            "Error, could not determine the newest python-build-standalone release."
        )

    if python_version is None:
        python_version = getBestPythonVersion(release_tag)

    url = getPBSAssetURL(
        target_arch=target_arch,
        release_tag=release_tag,
        python_version=python_version,
    )

    if url is None:
        general.sysexit("Error, unsupported target architecture '%s'." % target_arch)

    general.info(
        "Downloading the Linux Python for '%s' from '%s'." % (target_arch, url)
    )

    target_directory = getNormalizedPathJoin(
        getLinuxPythonDirectory(),
        getLinuxTargetDirectoryName(target_arch),
    )

    makePath(target_directory)

    getCachedDownload(
        name="linux-python-%s" % target_arch,
        url=url,
        binary="linux-python-%s.tar.gz" % target_arch,
        unzip=False,
        flatten=False,
        is_arch_specific=target_arch,
        specificity=url.rsplit("/", 1)[-1],
        message="Downloading the Linux Python for '%s'" % target_arch,
        reject=None,
        download_mode=getDownloadMode(assumeYesForDownloads()),
    )

    # The cached download is in the download cache, it needs to be
    # extracted into the target directory.
    from .Download import getDownloadCacheDir

    archive = getNormalizedPathJoin(
        getDownloadCacheDir(),
        "linux-python-%s.tar.gz" % target_arch,
        target_arch,
        url.rsplit("/", 1)[-1],
    )

    if not os.path.isfile(archive):
        # The cache layout depends on the arguments used, so the file is
        # searched for as a fallback.
        archive = _findCachedArchive(target_arch)

    if archive is None:
        general.sysexit(
            "Error, the download of the Linux Python did not produce an "
            "archive to extract."
        )

    _extractInstallOnly(archive, target_directory)

    prefix = getNormalizedPathJoin(target_directory, "python")

    if not isLinuxPythonPrefixComplete(prefix):
        general.sysexit(
            "Error, the extracted Linux Python at '%s' is not complete." % prefix
        )

    general.info("The Linux Python is now available at '%s'." % prefix)

    return prefix


def _findCachedArchive(target_arch):
    """Find the downloaded archive in the download cache.

    Args:
        target_arch: str

    Returns:
        str or None
    """
    from .Download import getDownloadCacheDir

    cache_dir = getNormalizedPathJoin(
        getDownloadCacheDir(), "linux-python-%s.tar.gz" % target_arch
    )

    if not os.path.isdir(cache_dir):
        return None

    for dir_path, _dir_names, file_names in os.walk(cache_dir):
        for file_name in file_names:
            if file_name.endswith(".tar.gz"):
                return getNormalizedPathJoin(dir_path, file_name)

    return None


def _extractInstallOnly(archive, target_directory):
    """Extract an install only archive into the target directory.

    Args:
        archive: str, the archive to extract
        target_directory: str, where to extract it to
    """
    # Remove a previous installation, so that only the newest version
    # is kept.
    if os.path.isdir(target_directory):
        removeDirectory(
            target_directory,
            logger=None,
            ignore_errors=True,
            extra_recommendation=None,
        )

    makePath(target_directory)

    with tarfile.open(archive, "r:gz") as tar:
        for member in tar.getmembers():
            # The archive is trusted, but still, no absolute paths and
            # no traversal outside of the target directory.
            name = member.name

            if name.startswith("/") or ".." in name.split("/"):
                continue

            if not member.isfile() and not member.isdir():
                continue

            tar.extract(member, target_directory)

    # The archive has a "python" directory at its top, which becomes
    # the prefix of the installation.
    extracted_prefix = getNormalizedPathJoin(target_directory, "python")

    if os.path.isdir(extracted_prefix):
        # Move it one level up, so that the layout is the expected one.
        temporary = getNormalizedPathJoin(target_directory, ".python-tmp")

        shutil.move(extracted_prefix, temporary)

        removeDirectory(
            target_directory,
            logger=None,
            ignore_errors=True,
            extra_recommendation=None,
        )

        makePath(target_directory)

        shutil.move(temporary, getNormalizedPathJoin(target_directory, "python"))


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
