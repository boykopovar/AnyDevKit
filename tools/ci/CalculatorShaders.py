from pathlib import Path
import subprocess


def generate(adk, project, target):
    directory = project / "build/shader-assets" / target
    directory.mkdir(parents=True, exist_ok=False)
    output = ['#ifndef ANYDEVKIT_CALCULATOR_GENERATED_SHADERS_HPP', '#define ANYDEVKIT_CALCULATOR_GENERATED_SHADERS_HPP', '#include "Graphics/ShaderAsset.hpp"', 'namespace CalculatorShaders {']
    shaders = [("Background.vert", "vertex", "Vertex", 12), ("Background.frag", "fragment", "Fragment", 208), ("Interface.vert", "vertex", "InterfaceVertex", 64), ("Interface.frag", "fragment", "InterfaceFragment", 64)]
    for source, stage, name, constant_size in shaders:
        header = directory / (name + ".header")
        code = directory / (name + ".code")
        metadata = directory / (name + ".adk")
        command = [str(adk), "shader", "--input", str(project / "shaders" / source), "--stage", stage, "--target", target, "--header", str(header), "--code", str(code), "--metadata", str(metadata)]
        if stage == "fragment":
            command += ["--color-format", "4"]
        subprocess.run(command, check=True)
        lines = metadata.read_text().splitlines()
        if lines[0] != "adk-psbc-metadata-v1":
            raise RuntimeError("Unexpected shader metadata format")
        fields = {}
        for line in lines[1:]:
            key, value = line.split("=", 1)
            if key in fields:
                raise RuntimeError("Duplicate shader metadata field")
            fields[key] = int(value)
        if any(fields[key] for key in ["descriptor_binding_count", "vertex_buffer_table_valid", "view_index_valid", "draw_id_valid", "start_instance_valid"]):
            raise RuntimeError("Unexpected calculator shader resource layout")
        data = [0] * 16
        if fields["ngg_lds_layout_valid"]:
            data[fields["ngg_lds_layout_user_data_dword"]] = fields["ngg_lds_layout"]
        if fields["push_constants_valid"] and fields["push_constant_size"] > constant_size:
            raise RuntimeError("Unexpected calculator push constant size")
        for suffix, path in [("Header", header), ("Code", code)]:
            values = path.read_bytes()
            if not values:
                raise RuntimeError("Empty shader output")
            output.append("inline constexpr std::uint8_t " + name + suffix + "[]{" + ",".join(map(str, values)) + "};")
        slot = fields["push_constants_user_data_dword"] if fields["push_constants_valid"] else -1
        output.append("inline constexpr ShaderAsset " + name + "{" + name + "Header, " + name + "Code, {" + ",".join(map(str, data)) + "}, " + str(fields["user_sgpr_count"]) + ", " + str(slot) + "};")
    output += ["}", "#endif"]
    destination = project / "generated"
    destination.mkdir(exist_ok=False)
    (destination / "CalculatorShaders.hpp").write_text("\n".join(output) + "\n")
