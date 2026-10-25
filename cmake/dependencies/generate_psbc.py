import pathlib
import os
import re
import subprocess
import sys


source = pathlib.Path(sys.argv[1]).resolve()
output = pathlib.Path(sys.argv[2]).resolve()
spirv = pathlib.Path(sys.argv[3]).resolve()
vulkan = pathlib.Path(sys.argv[4]).resolve()


def generate(script, destination, arguments=(), stdout=True, suffix="", additionalOutputs=()):
    destination = output / destination
    destination.parent.mkdir(parents=True, exist_ok=True)
    destinations = [destination, *additionalOutputs]
    existing = {path: (path.read_bytes(), path.stat()) for path in destinations if path.is_file()}
    command = [sys.executable, "-X", "utf8", str(source / script), *map(str, arguments)]
    if stdout:
        result = subprocess.run(command, cwd=source, check=True, stdout=subprocess.PIPE)
        destination.write_bytes(result.stdout + suffix.encode("utf-8"))
    else:
        subprocess.run(command, cwd=source, check=True)
    for path in destinations:
        if not path.is_file():
            raise RuntimeError(f"Generator did not produce {path}")
        if path in existing and path.read_bytes() == existing[path][0]:
            stat = existing[path][1]
            os.utime(path, ns=(stat.st_atime_ns, stat.st_mtime_ns))


for name in ("nir_opcodes_h", "nir_opcodes_c", "nir_constant_expressions", "nir_builder_opcodes_h"):
    target = name.removesuffix("_h").removesuffix("_c")
    extension = ".h" if name.endswith("_h") else ".c"
    generate(f"src/compiler/nir/{name}.py", f"src/compiler/nir/{target}{extension}")

for name, extension in (("nir_intrinsics_h", ".h"), ("nir_intrinsics_c", ".c"), ("nir_intrinsics_indices_h", ".h"), ("nir_opt_algebraic", ".c")):
    target = name.removesuffix("_h").removesuffix("_c") + extension
    destination = output / "src/compiler/nir" / target
    generate(f"src/compiler/nir/{name}.py", destination, ("--out", destination), False)

for name, target in (("aco_opcodes_h", "aco_opcodes.h"), ("aco_opcodes_cpp", "aco_opcodes.cpp"), ("aco_builder_h", "aco_builder.h")):
    generate(f"src/amd/compiler/{name}.py", f"src/amd/compiler/{target}")

grammar = source / "src/compiler/spirv/spirv.core.grammar.json"
spirvOutput = output / "src/compiler/spirv"
generate("src/compiler/spirv/spirv_info_gen.py", spirvOutput / "spirv_info.c", ("--out-c", spirvOutput / "spirv_info.c", "--out-h", spirvOutput / "spirv_info.h", "--json", grammar), False, additionalOutputs=(spirvOutput / "spirv_info.h",))
generate("src/compiler/spirv/vtn_generator_ids_h.py", spirvOutput / "vtn_generator_ids.h", (spirv / "include/spirv/spir-v.xml", spirvOutput / "vtn_generator_ids.h"), False)
generate("src/compiler/spirv/vtn_gather_types_c.py", spirvOutput / "vtn_gather_types.c", (grammar, spirvOutput / "vtn_gather_types.c"), False)

makefile = (source / "Makefile").read_text(encoding="utf-8").replace("\\\n", " ")
match = re.search(r"^AMD_REGISTER_FILES\s*=([^\n]+)", makefile, re.MULTILINE)
if not match:
    raise RuntimeError("PSBC register input list is missing")
registerFiles = [source / item for item in match.group(1).split()]
defines = re.findall(r'@echo "(#define [^"]+)" >> \$@', makefile)
if not defines:
    raise RuntimeError("PSBC register export definitions are missing")
generate("src/amd/registers/makeregheader.py", "src/amd/common/amdgfxregs.h", ("--sort", "address", "--guard", "AMDGPUregs", *registerFiles), suffix="\n" + "\n".join(defines) + "\n")
generate("src/amd/common/sid_tables.py", "src/amd/common/sid_tables.h", (source / "src/amd/registers/pkt3.json", *registerFiles))
formatData = source / "src/util/format/u_format.yaml"
generate("src/util/format/u_format_table.py", "src/util/format/u_format_gen.h", ("--enums", formatData))
generate("src/util/format/u_format_table.py", "src/util/format/u_format_pack.h", ("--header", formatData))
generate("src/util/format/u_format_table.py", "src/util/format/u_format_table.c", (formatData,))
packetInputs = [source / f"src/amd/packets/{name}_gfx{version}.{extension}" for version in (11, 12) for name, extension in (("cp_pm4_table_data", "json"), ("pm4_it_opcodes", "h"))]
for version in (11, 12):
    generate("src/amd/packets/parse_cp_pm4_table_data_json.py", f"src/amd/common/amd_cp_packets_gfx{version}.h", (*packetInputs, f"gfx{version}", "packets_h"))
for extension in ("h", "c"):
    destination = output / f"src/compiler/builtin_types.{extension}"
    generate(f"src/compiler/builtin_types_{extension}.py", destination, (destination,), False)
generate("src/util/format_srgb.py", "src/util/format_srgb.c")
generate("src/amd/common/gfx10_format_table.py", "src/amd/common/gfx10_format_table.c", (formatData, source / "src/amd/registers/gfx10-rsrc.json", source / "src/amd/registers/gfx11-rsrc.json"))
generate("src/util/process_shader_stats.py", "src/util/shader_stats.h", (source / "src/util/shader_stats.rnc", source / "src/util/shader_stats.xml"))
destination = output / "src/vulkan/util/vk_struct_type_cast.h"
generate("src/vulkan/util/vk_struct_type_cast_gen.py", destination, ("--xml", vulkan / "registry/vk.xml", "--out", destination, "--beta", "false"), False)
