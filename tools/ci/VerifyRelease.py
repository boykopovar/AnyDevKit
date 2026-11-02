import argparse
import json
from pathlib import Path
import sys
import urllib.parse

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "core/PublicSdk/scripts"))
from dependency_setup.downloads import asset, checksum, request


def verify(info, directory):
    files = sorted(directory.iterdir())
    if not files or any(not path.is_file() or not path.name.endswith((".zip", ".tar.gz", ".ffpfsc")) for path in files):
        raise ValueError("Expected release archives only")
    if sorted(item["name"] for item in info["assets"]) != sorted(path.name for path in files):
        raise ValueError("GitHub release assets differ from local archives")
    for path in files:
        metadata = asset(info, path.name)
        if path.stat().st_size != metadata["size"] or checksum(path) != metadata["sha256"]:
            raise ValueError(f"GitHub release digest mismatch: {path.name}")
        print(f"Verified GitHub SHA-256: {path.name}", flush=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--directory", type=Path, required=True)
    options = parser.parse_args()
    tag = urllib.parse.quote(options.tag, safe="")
    with request(f"https://api.github.com/repos/{options.repository}/releases/tags/{tag}") as response:
        info = json.load(response)
    verify(info, options.directory)


if __name__ == "__main__":
    main()
