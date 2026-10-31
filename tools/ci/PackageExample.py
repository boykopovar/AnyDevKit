import argparse
import hashlib
from pathlib import Path
import re
import shutil
import tarfile
import zipfile

parser = argparse.ArgumentParser()
parser.add_argument("--project", type=Path, required=True)
parser.add_argument("--deps", type=Path, required=True)
parser.add_argument("--directory", type=Path, required=True)
parser.add_argument("--target", choices=["windows", "linux", "ps5"], required=True)
parser.add_argument("--example", choices=["Calculator", "CppStandardLibrary"], required=True)
parser.add_argument("--version", required=True)
options = parser.parse_args()
if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9._-]*", options.version):
    raise ValueError("Invalid release version")
options.directory.mkdir(parents=True, exist_ok=False)
name = f"{options.example}-{options.version}-{options.target}-x86_64"
source = options.project / "dist" / options.target
if options.target == "ps5":
    package = options.directory / (name + ".ffpfsc")
    shutil.copy2(source / (options.example + ".ffpfsc"), package)
else:
    suffix = ".exe" if options.target == "windows" else ""
    if not (source / (options.example + suffix)).is_file():
        raise RuntimeError("Missing desktop executable")
    if not (source / "libc.prx").is_file():
        raise RuntimeError("Missing desktop runtime")
    parameter = options.project / "app0/sce_sys/param.json"
    if not parameter.is_file():
        raise RuntimeError("Missing application metadata")
    repository = Path(__file__).resolve().parents[2]
    extras = [(parameter, "app0/sce_sys/param.json"), (repository / "LICENSE", "LICENSE"), (options.deps / "AnyPS5-LICENSE", "licenses/AnyPS5-LICENSE"), (options.deps / options.target / "anycxx/LICENSE.TXT", "licenses/llvm-cxx-ps-LICENSE.TXT")]
    if options.target == "windows":
        package = options.directory / (name + ".zip")
        with zipfile.ZipFile(package, "x", compression=zipfile.ZIP_DEFLATED) as archive:
            for path in sorted(source.rglob("*")):
                if path.is_file():
                    archive.write(path, path.relative_to(source).as_posix())
            for path, destination in extras:
                archive.write(path, destination)
    else:
        package = options.directory / (name + ".tar.gz")
        def permissions(entry):
            if entry.name == options.example:
                entry.mode = 0o755
            return entry

        with tarfile.open(package, "x:gz") as archive:
            for path in sorted(source.iterdir()):
                archive.add(path, arcname=path.name, filter=permissions)
            for path, destination in extras:
                archive.add(path, arcname=destination)
if package.stat().st_size == 0:
    raise RuntimeError("Empty example package")
with package.open("rb") as stream:
    digest = hashlib.file_digest(stream, "sha256").hexdigest()
package.with_name(package.name + ".sha256").write_text(f"{digest}  {package.name}\n")
print(package)
