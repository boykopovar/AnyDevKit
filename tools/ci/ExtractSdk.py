import argparse
import hashlib
from pathlib import Path
import shutil

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
options.directory.mkdir(parents=True, exist_ok=False)
shutil.unpack_archive(archive, options.directory)
