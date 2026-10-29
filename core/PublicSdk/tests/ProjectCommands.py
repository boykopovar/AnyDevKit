import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--adk", required=True)
parser.add_argument("--templates", required=True)
options = parser.parse_args()
adk = str(Path(options.adk).resolve())


def run(*arguments, success=True):
    result = subprocess.run([adk, *map(str, arguments)], capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise AssertionError(result.stdout + result.stderr)
    return result


with tempfile.TemporaryDirectory(prefix="adk-project-") as temporary:
    root = Path(temporary)
    sdk = root / "sdk"
    shutil.copytree(options.templates, sdk / "share/AnyDevKit/templates")
    dependencies = root / "dependencies with spaces"
    dependencies.mkdir()
    project = root / "Demo"
    run("init", "Demo", "--directory", project, "--sdk", sdk, success=False)
    assert not project.exists()
    run("init", "Demo", "--directory", project, "--sdk", sdk, "--deps", dependencies, "--title-id", "FAKE26109")
    expected = ["CMakeLists.txt", "CMakePresets.json", "adk.json", "src/Main.cpp", "app0/sce_sys/param.json"]
    assert all((project / name).is_file() for name in expected)
    presets = json.loads((project / "CMakePresets.json").read_text())
    for preset in presets["configurePresets"]:
        cache = preset["cacheVariables"]
        assert Path(cache["ADK_DEPENDENCY_ROOT"]) == dependencies / preset["name"]
        assert Path(cache["ADK_SYSROOT"]) == dependencies / preset["name"] / "sysroot"
    assert "import-directory" not in json.loads((project / "adk.json").read_text())["targets"]["ps5"]
    metadata = project / "app0/sce_sys/param.json"
    original = metadata.read_bytes()
    run("param", "--param", metadata, "--output", root / "param.json")
    assert (root / "param.json").read_bytes() == original
    run("param", "--param", metadata, "--title", "en-US=Changed", "--output", root / "override.json")
    assert json.loads((root / "override.json").read_text())["localizedParameters"]["en-US"]["titleName"] == "Changed"
    assert metadata.read_bytes() == original
    before = sorted(str(path.relative_to(project)) for path in project.rglob("*"))
    for target in ["windows", "linux", "ps5"]:
        result = run("setup", "--target", target, "--project", project, success=False)
        assert "not implemented" in result.stderr
    run("update", "--project", project, success=False)
    assert before == sorted(str(path.relative_to(project)) for path in project.rglob("*"))
    run("init", "Demo", "--directory", project, "--sdk", sdk, success=False)
    assert metadata.read_bytes() == original
    for text in ['{"titleId":"A","titleId":"B"}', '{"unknown":true}', '{"titleId":', original.decode().replace('"attribute": 0', '"attribute": -1')]:
        bad = root / "bad.json"
        bad.write_text(text)
        run("param", "--param", bad, "--output", root / "bad-output.json", success=False)
        assert not (root / "bad-output.json").exists()
    config = project / "adk.json"
    value = json.loads(config.read_text())
    value["unknown"] = True
    config.write_text(json.dumps(value))
    run("package", "--target", "ps5", "--project", project, success=False)
    assert not (project / "dist").exists()
    for command in ["init", "setup", "update", "package", "shader", "shader-pipeline", "nid", "build"]:
        assert "adk " + command in run(command, "--help").stdout
    run("unknown", "--help", success=False)
print("Project configuration checks passed")
