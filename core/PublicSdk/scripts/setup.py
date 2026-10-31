import argparse
import json
from pathlib import Path
import shutil
import tempfile
import zipfile

from dependency_setup.downloads import asset, download, extract, extract_deb_headers, release, safe_path, write_file
from dependency_setup.elf_imports import prepare_elf_import


def windows_headers(cache, destination):
    version = "15.2.0posix-14.0.0-ucrt-r7"
    name = "winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64ucrt-14.0.0-r7.zip"
    source = {"url": f"https://github.com/brechtsanders/winlibs_mingw/releases/download/{version}/{name}", "sha256": "cb2fbad6162540cdf5e1facdce08d4dac359e8cf64f7f696a99274291763b815", "size": 267593703}
    archive = download(source, cache)
    with zipfile.ZipFile(archive) as entries:
        for entry in entries.infolist():
            path = safe_path(entry.filename)
            if not entry.is_dir() and path.parts[:3] == ("mingw64", "x86_64-w64-mingw32", "include"):
                with entries.open(entry) as stream:
                    write_file(destination, Path(*path.parts[2:]), stream, 0o644)
    if not (destination / "include/stdlib.h").is_file():
        raise ValueError("Windows sysroot has no stdlib.h")


def elf_headers(cache, destination):
    sources = [
        ("g/glibc/libc6-dev_2.36-9+deb12u14_amd64.deb", "0218fc2befcd784c1b0c6292c0a137ce89fad054efaa579ad083bee0f2c01aae"),
        ("l/linux/linux-libc-dev_6.1.176-1_amd64.deb", "8bb258735b9dffbb111da778ebdd024750878e435ffd9dfcadcb6762ede6b4cf")]
    for path, digest in sources:
        archive = download({"url": "https://deb.debian.org/debian/pool/main/" + path, "sha256": digest}, cache)
        extract_deb_headers(archive, destination, required_only=path.startswith("l/linux/"))
    if not (destination / "usr/include/stdlib.h").is_file():
        raise ValueError("ELF sysroot has no stdlib.h")


def unpack_asset(info, name, cache, directory):
    extract(download(asset(info, name), cache), directory)
    return directory


def prepare(target, root, scratch, cache, anyps5, anycxx):
    directory = root / target
    platform = "windows" if target == "windows" else "linux"
    native = scratch / platform
    if not native.exists():
        unpack_asset(anyps5, f"prx-{platform}-unpatched-{anyps5['tag_name']}.zip", cache, native)
    libraries = native / "libs"
    if not (libraries / "libc.prx").is_file():
        raise ValueError("Unpatched release is missing libc.prx")
    imports = directory / "imports"
    imports.mkdir(parents=True)
    for library in sorted(libraries.glob("*.prx")):
        if target == "windows":
            source = libraries / "implib" / ("lib" + library.stem + ".dll.a")
            destination = imports / (library.stem + ".a")
        else:
            source = library
            destination = imports / library.name
        if not source.is_file():
            raise ValueError(f"Missing release import library: {source}")
        if target == "windows":
            shutil.copy2(source, destination)
        else:
            prepare_elf_import(source, destination)
    if target != "ps5":
        runtime = scratch / (platform + "-runtime")
        unpack_asset(anyps5, f"prx-{platform}-{anyps5['tag_name']}.zip", cache, runtime)
        shutil.copytree(runtime / "libs", directory / "runtime")
    suffix = "zip" if target == "windows" else "tar.gz"
    version = anycxx["tag_name"].removeprefix("v")
    unpack_asset(anycxx, f"llvm-cxx-ps-{version}-{target}-x86_64.{suffix}", cache, directory / "anycxx")
    if not (directory / "anycxx/lib/cmake/AnyLibcxx/AnyLibcxxConfig.cmake").is_file():
        raise ValueError("C++ release is missing its CMake configuration")
    if target == "windows":
        windows_headers(cache, directory / "sysroot")
    else:
        elf_headers(cache, directory / "sysroot")
    print(f"Prepared {target}", flush=True)


def main():
    parser = argparse.ArgumentParser(description="Install release libraries and C headers for AnyDevKit. No target selects all platforms.")
    parser.add_argument("directory", type=Path)
    parser.add_argument("--target", choices=("windows", "linux", "ps5"), action="append")
    parser.add_argument("--anyps5-version", default="latest")
    parser.add_argument("--anycxx-version", default="latest")
    parser.add_argument("--cache", type=Path)
    options = parser.parse_args()
    root = options.directory.resolve()
    if root.exists():
        raise ValueError(f"Dependency directory already exists: {root}")
    root.parent.mkdir(parents=True, exist_ok=True)
    cache = options.cache.resolve() if options.cache else root.parent / ".adk-downloads"
    targets = list(dict.fromkeys(options.target or ["windows", "linux", "ps5"]))
    anyps5 = release("boykopovar/AnyPS5", options.anyps5_version)
    anycxx = release("boykopovar/llvm-cxx-ps", options.anycxx_version)
    with tempfile.TemporaryDirectory(prefix="adk-setup-", dir=root.parent) as temporary:
        scratch = Path(temporary)
        staging = scratch / "dependencies"
        headers = unpack_asset(anyps5, f"prx-headers-{anyps5['tag_name']}.zip", cache, scratch / "headers")
        shutil.copytree(headers / "include", staging / "include")
        shutil.copy2(headers / "LICENSE", staging / "AnyPS5-LICENSE")
        for target in targets:
            prepare(target, staging, scratch, cache, anyps5, anycxx)
        metadata = {"schemaVersion": 1, "anyps5": anyps5["tag_name"], "anycxx": anycxx["tag_name"], "targets": targets}
        (staging / "dependencies.json").write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
        staging.rename(root)
    print(f"Dependencies installed: {root}", flush=True)


if __name__ == "__main__":
    main()
