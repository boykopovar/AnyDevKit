import argparse
from pathlib import Path
import shutil

parser = argparse.ArgumentParser()
parser.add_argument("--packages", type=Path, required=True)
parser.add_argument("--directory", type=Path, required=True)
options = parser.parse_args()
archives = list(options.packages.glob("AnyDevKit-*.zip")) + list(options.packages.glob("AnyDevKit-*.tar.gz"))
if len(archives) != 1:
    raise RuntimeError("Expected one SDK archive")
archive = archives[0]
options.directory.mkdir(parents=True, exist_ok=False)
shutil.unpack_archive(archive, options.directory)

