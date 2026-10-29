# Usage

## Install

Install Clang, LLD, CMake 3.22.1 or later and Ninja. Windows also requires MinGW headers and GNU objcopy.

Extract the latest [AnyDevKit release](https://github.com/boykopovar/AnyDevKit/releases/latest). Add its `bin` directory to `PATH`.

## Create

```sh
adk init Calculator
cd Calculator
```

| File | Purpose |
|---|---|
| `src/Main.cpp` | Application code |
| `CMakeLists.txt` | Sources, libraries and shaders |
| `CMakePresets.json` | Toolchain and dependency paths |
| `app0/sce_sys/param.json` | Application metadata |
| `adk.json` | Packaging settings |

The template contains a minimal console program. Its generated `FAKE` title ID is derived from the project name and is not guaranteed unique. Change it before distribution, or pass `--title-id` to `adk init`.

## Dependencies

`adk setup` and `adk update` are not implemented. Both fail without downloading files or creating a lock file.

Install dependencies manually from the latest [C++ standard library](https://github.com/boykopovar/llvm-cxx-ps/releases/latest) and [AnyPS5](https://github.com/boykopovar/AnyPS5/releases/latest) releases. Use matching platform packages.

Set `ADK_DEPENDENCY_ROOT` and `ADK_SYSROOT` in `CMakePresets.json`.

| Directory under `ADK_DEPENDENCY_ROOT` | Contents |
|---|---|
| `anycxx` | Extracted C++ platform package |
| `imports` | Link-time import libraries |
| `runtime` | Desktop AnyPS5 libraries and dependencies |
| `modules` | PS5 modules for the import catalogue |

Windows import files use `libName.a`; Linux and PS5 use `libName.prx`. Import libraries must be prepared separately; the template does not generate them. PS5 catalogue modules and desktop runtime binaries are distinct inputs.

`ADK_SYSROOT` points to MinGW on Windows or the target C-header sysroot for ELF builds.

## Windows

```sh
cmake --preset windows
cmake --build --preset windows
```

Run `dist/windows/Calculator.exe`. CMake patches its imports and copies the configured runtime directory beside it.

## Linux

Use the Linux SDK package.

```sh
cmake --preset linux
cmake --build --preset linux
```

Run `dist/linux/Calculator`.

## PS5

```sh
cmake --preset ps5
cmake --build --preset ps5
adk package --target ps5
```

Output: `dist/ps5/Calculator.ffpfsc`.

The output file and workspace must be new. Remove previous outputs explicitly or select new paths. Packaging never invokes the application compiler.

## Configuration

Edit `param.json` for metadata and `adk.json` for packaging. Supported metadata fields appear in the template; `kernel.flexibleMemorySize` is optional. Unknown fields, duplicate JSON keys and invalid values are rejected.

A valid `param.json` is packaged byte-for-byte unless metadata is explicitly overridden through the CLI. The source file is never changed.

Precedence: explicit CLI option, project configuration, documented default. Configuration paths are relative to the project; CLI paths are relative to the working directory.

```sh
adk package --target ps5 --output dist/ps5/Other.ffpfsc --workspace build/other-package
```

All existing ELF, SELF, PFS and PKG options remain available. For PKG, set `format` and `output` and supply `passcode-moduli`, `mount-modulus`, `passcode` and `image-seed`. Remove PFS-only options when switching to PKG.

```cmake
adk_link_libraries(Calculator libScePad libSceVideoOut)
```

## Assets

Place application files in `app0`.

| Optional image | Dimensions |
|---|---|
| `app0/sce_sys/icon0.png` | 512 x 512 |
| `app0/sce_sys/pic0.png` | 3840 x 2160 |

A missing icon is generated. A missing background is omitted. Invalid images fail packaging.

## Shaders

```cmake
adk_add_shaders(Calculator shaders/Main.vert shaders/Main.frag shaders/Work.comp)
```

Use `INCLUDE_DIRECTORIES` for includes and `OPTIONS` for additional shader CLI options.

```cmake
adk_add_shader_pipeline(Calculator Geometry
    VERTEX shaders/Main.vert
    GEOMETRY shaders/Main.geom
)
adk_add_shader_pipeline(Calculator Tessellation
    VERTEX shaders/Patch.vert
    CONTROL shaders/Patch.tesc
    EVALUATION shaders/Patch.tese
    OPTIONS --patch-control-points 3
)
```

Outputs are under the build directory's `shaders` directory. The target's `ADK_SHADER_DIRECTORIES` property lists them. Embed the files or copy them into `app0` before packaging. All platforms use PS5 shader assets.

## Reference

```sh
adk --help
adk package --help
adk shader --help
adk shader-pipeline --help
```

Individual NID, metadata, image, ELF, SELF, PFS, FFPFSC and PKG commands remain available.
