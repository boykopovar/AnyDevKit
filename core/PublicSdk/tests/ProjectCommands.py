import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
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


with tempfile.TemporaryDirectory(prefix="adk-project-", dir=Path(adk).parent) as temporary:
    root = Path(temporary)
    sdk = root / "sdk"
    shutil.copytree(options.templates, sdk / "share/AnyDevKit/templates")
    installed = sdk / "bin" / Path(adk).name
    installed.parent.mkdir()
    shutil.copy2(adk, installed)
    script = sdk / "tools/setup.py"
    script.parent.mkdir()
    script.write_text("import json, sys\nprint(json.dumps(sys.argv[1:]))\nraise SystemExit(7 if '--fail' in sys.argv else 0)\n")
    environment = dict(os.environ, PATH=str(Path(sys.executable).parent))
    forwarded = [str(root / 'deps with spaces'), '--target', 'windows', '--target', 'ps5', '--cache', 'quotes" and trailing\\', '& echo forbidden']
    result = subprocess.run([installed, 'setup', *forwarded], env=environment, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    assert json.loads(result.stdout) == forwarded
    result = subprocess.run([installed, 'setup', '--fail'], env=environment, capture_output=True, text=True)
    assert result.returncode != 0 and 'exited with code 7' in result.stderr
    result = subprocess.run([installed, 'setup', 'deps'], env=dict(os.environ, PATH=str(root)), capture_output=True, text=True)
    assert result.returncode != 0 and 'Python 3.11+ not found' in result.stderr
    script.unlink()
    result = subprocess.run([installed, 'setup', 'deps'], env=environment, capture_output=True, text=True)
    assert result.returncode != 0 and 'Missing SDK tools/setup.py' in result.stderr
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
