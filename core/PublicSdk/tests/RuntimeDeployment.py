import argparse
from pathlib import Path
import subprocess
import tempfile
import xml.etree.ElementTree as xml

parser = argparse.ArgumentParser()
parser.add_argument("--clang", required=True)
parser.add_argument("--cmake", required=True)
parser.add_argument("--readobj", required=True)
parser.add_argument("--deploy", required=True)
parser.add_argument("--directory", type=Path, required=True)
options = parser.parse_args()


def run(command, success=True):
    result = subprocess.run(list(map(str, command)), capture_output=True, text=True)
    if (result.returncode == 0) != success:
        raise AssertionError(result.stdout + result.stderr)
    return result


options.directory.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="runtime-", dir=options.directory) as temporary:
    for platform in ["windows", "linux"]:
        root = Path(temporary) / platform
        runtime = root / "runtime"
        output = root / "output"
        runtime.mkdir(parents=True)
        output.mkdir()
        windows = platform == "windows"
        compiler = [options.clang, "--target=" + ("x86_64-pc-windows-msvc" if windows else "x86_64-unknown-linux-gnu"), "-fuse-ld=lld", "-nostdlib"]
        export = '__declspec(dllexport) ' if windows else ''

        def library(name, symbol, dependency=None):
            source = root / (name + ".cpp")
            declaration = 'extern "C" int Leaf();' if dependency else ''
            source.write_text(declaration + '\nextern "C" ' + export + 'int ' + symbol + '() { return ' + ('Leaf()' if dependency else '7') + '; }\n')
            binary = runtime / (name + ".prx")
            flags = ["-shared", "-Wl,/noentry", "-Wl,/implib:" + str(root / (name + ".lib"))] if windows else ["-shared", "-fPIC", "-Wl,-soname," + binary.name]
            inputs = [source]
            if dependency:
                inputs.append(root / (dependency + ".lib") if windows else runtime / (dependency + ".prx"))
            run(compiler + flags + inputs + ["-o", binary])

        library("libLeaf", "Leaf")
        library("libMiddle", "Middle", "libLeaf")
        library("libPlugin", "Plugin", "libLeaf")
        library("libUnused", "Unused")
        source = root / "Main.cpp"
        source.write_text('extern "C" int Middle();\nextern "C" int Entry() { return Middle(); }\n')
        binary = output / ("App.exe" if windows else "App")
        flags = ["-Wl,/entry:Entry", "-Wl,/subsystem:console"] if windows else ["-Wl,--entry=Entry", "-Wl,--as-needed", "-Wl,--allow-shlib-undefined"]
        dependency = root / "libMiddle.lib" if windows else runtime / "libMiddle.prx"
        unused = root / "libUnused.lib" if windows else runtime / "libUnused.prx"
        run(compiler + flags + [source, dependency, unused, "-o", binary])
        command = [options.cmake, "-DREADOBJ=" + options.readobj, "-DEXECUTABLE=" + str(binary), "-DTARGET_PLATFORM=" + platform, "-DRUNTIME_DIRECTORY=" + str(runtime), "-DOUTPUT_DIRECTORY=" + str(output)]

        def deploy(extra="", success=True):
            return run(command + ["-DEXTRA_LIBRARIES=" + extra, "-P", options.deploy], success)

        def verify(names):
            actual = {path.name for path in (output / "libs").glob("*.prx")}
            if actual != names:
                raise AssertionError((platform, actual, names))
            if windows:
                manifest = xml.parse(output / "libs/libs.manifest")
                entries = {node.attrib["name"] for node in manifest.findall("{urn:schemas-microsoft-com:asm.v1}file")}
                if entries != names:
                    raise AssertionError((entries, names))

        deploy()
        verify({"libMiddle.prx", "libLeaf.prx"})
        deploy("libPlugin.prx")
        verify({"libMiddle.prx", "libLeaf.prx", "libPlugin.prx"})
        deploy()
        verify({"libMiddle.prx", "libLeaf.prx"})
        (runtime / "libLeaf.prx").rename(root / "libLeaf.prx")
        result = deploy(success=False)
        if "Missing runtime dependency" not in result.stderr:
            raise AssertionError(result.stderr)
        verify({"libMiddle.prx", "libLeaf.prx"})
        (root / "libLeaf.prx").rename(runtime / "libLeaf.prx")
        if windows:
            source.write_text('extern "C" int Middle();\nextern "C" __declspec(dllimport) int Plugin();\nextern "C" void* __delayLoadHelper2(void*, void*) { return nullptr; }\nextern "C" int Entry() { return Middle() + Plugin(); }\n')
            run(compiler + flags + ["-Wl,/delayload:libPlugin.prx", source, dependency, root / "libPlugin.lib", "-o", binary])
            deploy()
            verify({"libMiddle.prx", "libLeaf.prx", "libPlugin.prx"})
        print(platform + ": runtime deployment checks passed")
