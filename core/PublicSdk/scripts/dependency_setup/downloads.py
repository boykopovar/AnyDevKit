import hashlib
import io
import json
import os
import re
from pathlib import Path, PurePosixPath
import shutil
import stat
import tarfile
import urllib.parse
import urllib.request
import zipfile


def request(url):
    if urllib.parse.urlparse(url).scheme != "https":
        raise ValueError(f"HTTPS required: {url}")
    headers = {"User-Agent": "AnyDevKit-dependencies", "Accept": "application/vnd.github+json"}
    token = os.environ.get("GITHUB_TOKEN")
    if token and urllib.parse.urlparse(url).hostname == "api.github.com":
        headers["Authorization"] = "Bearer " + token
    response = urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=120)
    if urllib.parse.urlparse(response.url).scheme != "https":
        response.close()
        raise ValueError("Download redirected outside HTTPS")
    return response


def release(repository, version):
    suffix = "latest" if version == "latest" else "tags/" + urllib.parse.quote(version, safe="")
    with request(f"https://api.github.com/repos/{repository}/releases/{suffix}") as response:
        result = json.load(response)
    if result["draft"] or result["prerelease"]:
        raise ValueError(f"Expected a published stable release: {repository}")
    return result


def asset(release_info, name):
    matches = [item for item in release_info["assets"] if item["name"] == name]
    if len(matches) != 1:
        raise ValueError(f"Missing or ambiguous release asset: {name}")
    item = matches[0]
    digest = item.get("digest", "")
    if not digest.startswith("sha256:") or len(digest) != 71:
        raise ValueError(f"Missing SHA-256 for release asset: {name}")
    return {"url": item["browser_download_url"], "sha256": digest[7:], "size": item["size"]}


def checksum(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()


def download(source, cache):
    name = Path(urllib.parse.unquote(urllib.parse.urlparse(source["url"]).path)).name
    size = source.get("size")
    if not name or len(source["sha256"]) != 64:
        raise ValueError("Invalid download metadata")
    cache.mkdir(parents=True, exist_ok=True)
    destination = cache / name
    if destination.exists():
        if destination.is_symlink() or (size is not None and destination.stat().st_size != size) or checksum(destination) != source["sha256"]:
            raise ValueError(f"Cached download does not match SHA-256: {destination}")
        print(f"Verified cache: {name}", flush=True)
        return destination
    temporary = destination.with_name(name + ".partial")
    if temporary.exists():
        raise ValueError(f"Incomplete download already exists: {temporary}")
    print(f"Downloading {name}", flush=True)
    try:
        with request(source["url"]) as response, temporary.open("xb") as output:
            total = 0
            progress = 0
            digest = hashlib.sha256()
            while block := response.read(1024 * 1024):
                total += len(block)
                if size is not None and total > size:
                    raise ValueError(f"Download exceeds expected size: {name}")
                output.write(block)
                digest.update(block)
                if total - progress >= 32 * 1024 * 1024:
                    print(f"{name}: {total // 1048576} MiB", flush=True)
                    progress = total
        if (size is not None and total != size) or digest.hexdigest() != source["sha256"]:
            raise ValueError(f"Download checksum or size mismatch: {name}")
        temporary.rename(destination)
    finally:
        temporary.unlink(missing_ok=True)
    return destination


def safe_path(name):
    if "\\" in name or ":" in name or "\x00" in name:
        raise ValueError(f"Unsafe archive path: {name}")
    path = PurePosixPath(name)
    if path.is_absolute() or ".." in path.parts:
        raise ValueError(f"Unsafe archive path: {name}")
    return path


def write_file(root, relative, source, mode):
    destination = root.joinpath(*relative.parts)
    destination.parent.mkdir(parents=True, exist_ok=True)
    with destination.open("xb") as output:
        shutil.copyfileobj(source, output)
    destination.chmod(mode & 0o777 | 0o600)


def extract_tar(archive, root, accept=lambda name: True):
    links = []
    for member in archive:
        relative = safe_path(member.name)
        if not relative.parts or not accept(relative.as_posix()):
            continue
        destination = root.joinpath(*relative.parts)
        if member.isdir():
            destination.mkdir(parents=True, exist_ok=True)
        elif member.isfile():
            with archive.extractfile(member) as source:
                write_file(root, relative, source, member.mode)
        elif member.issym() or member.islnk():
            target = PurePosixPath(member.linkname)
            if target.is_absolute() or "\\" in member.linkname or ":" in member.linkname:
                raise ValueError(f"Unsafe archive link: {member.name}")
            resolved = root.joinpath(*(relative.parent / target if member.issym() else target).parts).resolve()
            if not resolved.is_relative_to(root.resolve()):
                raise ValueError(f"Archive link escapes destination: {member.name}")
            links.append((destination, resolved))
        else:
            raise ValueError(f"Unsupported archive entry: {member.name}")
    while links:
        remaining = []
        for destination, target in links:
            if target.is_file():
                with target.open("rb") as source:
                    write_file(root, destination.relative_to(root), source, target.stat().st_mode)
            else:
                remaining.append((destination, target))
        if len(remaining) == len(links):
            raise ValueError(f"Unresolved archive links: {remaining}")
        links = remaining


def extract(path, root):
    root.mkdir(parents=True, exist_ok=True)
    if path.name.endswith(".zip"):
        with zipfile.ZipFile(path) as archive:
            for member in archive.infolist():
                relative = safe_path(member.filename)
                if not relative.parts:
                    continue
                mode = member.external_attr >> 16
                if stat.S_ISLNK(mode):
                    raise ValueError(f"Unsupported ZIP symlink: {member.filename}")
                if member.is_dir():
                    root.joinpath(*relative.parts).mkdir(parents=True, exist_ok=True)
                else:
                    with archive.open(member) as source:
                        write_file(root, relative, source, mode)
    else:
        with tarfile.open(path, "r:*") as archive:
            extract_tar(archive, root)


def extract_deb_headers(path, root, required_only=False):
    data = path.read_bytes()
    if not data.startswith(b"!<arch>\n"):
        raise ValueError(f"Invalid Debian archive: {path}")
    offset = 8
    found = False
    while offset < len(data):
        header = data[offset:offset + 60]
        if len(header) != 60 or header[58:] != b"`\n":
            raise ValueError("Malformed Debian archive member")
        size = int(header[48:58])
        name = header[:16].decode("ascii").strip().rstrip("/")
        start = offset + 60
        end = start + size
        if size < 0 or end > len(data):
            raise ValueError("Truncated Debian archive")
        offset = end + size % 2
        if name.startswith("data.tar"):
            if found:
                raise ValueError("Duplicate Debian data archive")
            found = True
            with tarfile.open(fileobj=io.BytesIO(data[start:end]), mode="r:*") as archive:
                members = archive.getmembers()
                headers = {safe_path(m.name).as_posix().removeprefix("usr/include/"): m for m in members if m.isfile() and safe_path(m.name).as_posix().startswith("usr/include/")}
                selected = set()
                if required_only:
                    pending = [p.read_bytes() for p in (root / "usr/include").rglob("*.h")]
                    while pending:
                        for name in re.findall(rb'#\s*include\s*[<"]([^>"]+)', pending.pop()):
                            key = name.decode("ascii")
                            if key.startswith("asm/"):
                                key = "x86_64-linux-gnu/" + key
                            if key in headers and key not in selected:
                                selected.add(key)
                                pending.append(archive.extractfile(headers[key]).read())
                extract_tar(archive, root, lambda name: name.startswith("usr/include/") and (not required_only or name.removeprefix("usr/include/") in selected) or name.startswith("usr/share/doc/") and name.endswith("/copyright"))
    if not found:
        raise ValueError("Debian package has no data archive")
