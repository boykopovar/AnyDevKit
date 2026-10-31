import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--packages", type=Path, required=True)
parser.add_argument("--directory", type=Path, required=True)
options = parser.parse_args()
checksums = list(options.packages.glob("AnyDevKit-*.sha256"))
if len(checksums) != 1:
    raise RuntimeError("Expected one SDK archive checksum")
digest, name = checksums[0].read_text().strip().split("  ", 1)
archive = options.packages / name
if archive.name != name or hashlib.sha256(archive.read_bytes()).hexdigest() != digest:
    raise RuntimeError("Invalid SDK archive checksum")
root = options.directory.resolve()
root.mkdir(parents=True, exist_ok=False)
sdk = root / "sdk"
shutil.unpack_archive(archive, sdk)
adk = sdk / "bin" / ("adk.exe" if sys.platform == "win32" else "adk")
startup = "libAnyDevKitWindowsStartup.a" if sys.platform == "win32" else "libAnyDevKitLinuxStartup.a"
for path in [adk, sdk / "lib" / startup, sdk / "LICENSE", sdk / "tools/setup.py", sdk / "lib/cmake/AnyDevKit/AnyDevKitConfig.cmake", sdk / "share/AnyDevKit/docs/USAGE.md"]:
    if not path.is_file():
        raise RuntimeError(f"Missing installed file: {path}")
subprocess.run([adk, "--help"], check=True, cwd=root)
subprocess.run([sys.executable, sdk / "tools/setup.py", "--help"], check=True, cwd=root)
dependencies = root / "dependencies"
dependencies.mkdir()
subprocess.run([adk, "init", "Smoke", "--deps", dependencies], check=True, cwd=root)
project = root / "Smoke"
presets = json.loads((project / "CMakePresets.json").read_text())
if {preset["name"] for preset in presets["configurePresets"]} != {"windows", "linux", "ps5"}:
    raise RuntimeError("Missing platform presets")
for preset in presets["configurePresets"]:
    variables = preset["cacheVariables"]
    if Path(variables["AnyDevKit_DIR"]).resolve() != sdk / "lib/cmake/AnyDevKit":
        raise RuntimeError("Project did not discover the installed SDK")
    if not Path(variables["CMAKE_TOOLCHAIN_FILE"]).is_file():
        raise RuntimeError("Missing installed toolchain")
source = Path(__file__).resolve().parents[2] / "shader/compiler/tests/shaders/Triangle.vert"
subprocess.run([adk, "shader", "--input", source, "--stage", "vertex", "--header", root / "shader.header", "--code", root / "shader.code", "--metadata", root / "shader.adk"], check=True, cwd=root)
for name in ["shader.header", "shader.code", "shader.adk"]:
    if not (root / name).stat().st_size:
        raise RuntimeError(f"Empty shader output: {name}")
subprocess.run([sys.executable, Path(__file__).with_name("ProjectCommands.py").resolve(), "--adk", adk, "--templates", sdk / "share/AnyDevKit/templates"], check=True, cwd=root, env=os.environ | {"TMP": str(root), "TEMP": str(root), "TMPDIR": str(root)})
print("Installed SDK verification passed")
