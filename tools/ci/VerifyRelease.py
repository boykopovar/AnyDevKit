import argparse
import json
from pathlib import Path
import sys

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


def find_release(repository, tag):
    matches = []
    page = 1
    while True:
        with request(f"https://api.github.com/repos/{repository}/releases?per_page=100&page={page}") as response:
            releases = json.load(response)
        matches.extend(info for info in releases if info["tag_name"] == tag)
        if len(releases) < 100:
            break
        page += 1
    if len(matches) != 1:
        raise ValueError(f"Missing or ambiguous release: {tag}")
    release_id = matches[0]["id"]
    with request(f"https://api.github.com/repos/{repository}/releases/{release_id}") as response:
        info = json.load(response)
    if info["tag_name"] != tag:
        raise ValueError("Release tag changed during verification")
    return info


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repository", required=True)
    parser.add_argument("--tag", required=True)
    parser.add_argument("--directory", type=Path, required=True)
    options = parser.parse_args()
    info = find_release(options.repository, options.tag)
    verify(info, options.directory)


if __name__ == "__main__":
    main()

