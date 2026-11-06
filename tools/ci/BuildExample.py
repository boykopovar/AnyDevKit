import argparse
import json
from pathlib import Path
import shutil
import subprocess
import sys

from Calculator3DShaders import generate

parser = argparse.ArgumentParser()
parser.add_argument("--sdk", type=Path, required=True)
parser.add_argument("--deps", type=Path, required=True)
parser.add_argument("--directory", type=Path, required=True)
parser.add_argument("--target", choices=["windows", "linux", "ps5"], required=True)
parser.add_argument("--compiler", choices=["clang", "gcc"], default="clang")
parser.add_argument("--example", choices=["Calculator3D", "CppStandardLibrary", "BouncingTitle"], required=True)
options = parser.parse_args()
repository = Path(__file__).resolve().parents[2]
sdk = options.sdk.resolve(strict=True)
dependencies = options.deps.resolve(strict=True)
project = options.directory.resolve()
adk = sdk / "bin" / ("adk.exe" if sys.platform == "win32" else "adk")
metadata = json.loads((dependencies / "dependencies.json").read_text())
if options.target not in metadata["targets"]:
    raise RuntimeError("Target dependencies were not installed")
subprocess.run([str(adk), "init", options.example, "--directory", str(project), "--deps", str(dependencies)], check=True)
configuration = project / "adk.json"
settings = json.loads(configuration.read_text())
settings["targets"]["ps5"]["select-import"] = ["sceKernelMapDirectMemory,libkernel,libkernel"]
configuration.write_text(json.dumps(settings, indent=2) + "\n")
logs = project / "logs"
logs.mkdir()
if options.example == "BouncingTitle":
    sources = sorted(path.relative_to(project).as_posix() for path in (project / "src").rglob("*.cpp"))
else:
    example = repository / "examples" / options.example
    for name in ["src", "include", "shaders"]:
        shutil.rmtree(project / name)
    shutil.copytree(example / "src", project / "src", dirs_exist_ok=True)
    shutil.copytree(example / "include", project / "include")
    sources = sorted(path.relative_to(project).as_posix() for path in (project / "src").rglob("*.cpp"))
    includes = ["include"]
    if options.example == "Calculator3D":
        libraries = ["libc", "libkernel", "libSceVideoOut", "libSceSystemService", "libScePad", "libSceSysmodule", "libSceSaveData.native", "libSceUserService", "libSceAgc", "libSceAgcDriver"]
        shutil.copytree(example / "shaders", project / "shaders")
        generate(adk, project, options.target)
        includes.append("generated")
        definitions = "ANYDEVKIT_CALCULATOR3D_WIDTH=1920"
    else:
        libraries = ["libc", "libkernel", "libSceVideoOut", "libSceSystemService"]
        calculator3D = repository / "examples/Calculator3D"
        shutil.copytree(calculator3D / "include", project / "shared/include")
        for name in ["Canvas.cpp", "Display.cpp"]:
            shutil.copy2(calculator3D / "src" / name, project / "src" / name)
            sources.append("src/" + name)
        includes.append("shared/include")
        definitions = "ANYDEVKIT_CPP_HEADLESS=0"
    cmake = f"cmake_minimum_required(VERSION 3.22.1)\nproject({options.example} LANGUAGES C CXX)\nfind_package(AnyDevKit CONFIG REQUIRED)\n"
    cmake += f"adk_add_executable({options.example}\n" + "\n".join("    " + source for source in sources) + ")\n"
    cmake += f"adk_link_libraries({options.example} " + " ".join(libraries) + ")\n"
    cmake += f"target_include_directories({options.example} PRIVATE " + " ".join(includes) + ")\n"
    cmake += f"target_compile_definitions({options.example} PRIVATE {definitions})\n"
    cmake += f"target_compile_options({options.example} PRIVATE -fno-math-errno)\n"
    (project / "CMakeLists.txt").write_text(cmake)



def run(name, command):
    print(" ".join(map(str, command)), flush=True)
    with (logs / (name + ".log")).open("w", encoding="utf-8") as log:
        result = subprocess.run(list(map(str, command)), cwd=project, stdout=log, stderr=subprocess.STDOUT)
    if result.returncode:
        print((logs / (name + ".log")).read_text(encoding="utf-8", errors="replace"), flush=True)
        raise RuntimeError(f"{name} failed: {result.returncode}")


configuration = ["cmake", "--preset", options.target, "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON", f"-DADK_COMPILER={options.compiler}"]
run("configure", configuration)
commands = json.loads((project / "build" / options.target / "compile_commands.json").read_text())
if len(commands) < len(sources):
    raise RuntimeError("Missing example compilation commands")
headers = (dependencies / "include/anyps5").as_posix().lower()
for entry in commands:
    command = entry["command"].replace("\\", "/").lower()
    if headers not in command:
        raise RuntimeError("Application compilation does not use release headers")
    if "3rdparty/anyps5" in command:
        raise RuntimeError("Application compilation references the AnyPS5 submodule")
run("build", ["cmake", "--build", "--preset", options.target, "--parallel", "4"])
if options.target == "ps5" and options.compiler == "clang":
    run("ffpfsc", [adk, "package", "--target", "ps5"])
    output = project / "dist/ps5" / (options.example + ".ffpfsc")
elif options.target == "ps5":
    output = project / "build/ps5" / (options.example + ".elf")
else:
    suffix = ".exe" if options.target == "windows" else ""
    output = project / "dist" / options.target / (options.example + suffix)
if not output.is_file() or output.stat().st_size == 0:
    raise RuntimeError(f"Missing output: {output}")
if options.target != "ps5":
    libraries = output.parent / "libs"
    if not (libraries / "libc.prx").is_file():
        raise RuntimeError("Missing runtime in libs directory")
    if any(path.is_file() and (path.suffix in {".prx", ".dll", ".so"} or ".so." in path.name) for path in output.parent.iterdir()):
        raise RuntimeError("Runtime library outside libs directory")
    if options.target == "windows" and not (libraries / "libs.manifest").is_file():
        raise RuntimeError("Missing Windows runtime manifest")
print(f"Built {options.example} for {options.target}: {output}", flush=True)
