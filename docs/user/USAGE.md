# Usage

Extract the latest [SDK](https://github.com/boykopovar/AnyDevKit/releases/latest). Add its `bin` directory to `PATH`. Install Python 3.11+ (optional for auto-setup), CMake 3.22+, Ninja, Clang and LLD.

`adk setup` has no native implementation; it runs `tools/setup.py` using system Python. Otherwise, run the script manually with Python 3.11+. `tools/setup.py` downloads [AnyPS5 v0.1.4](https://github.com/boykopovar/AnyPS5/releases/tag/v0.1.4) and [C++ runtime v0.1.6](https://github.com/boykopovar/llvm-cxx-ps/releases/tag/v0.1.6) and prepares C headers and import libraries. To use manually downloaded archives, pass `--cache <directory>`.

```sh
adk setup .adk
adk init MyApp --deps .adk
cd MyApp
```

Without `--target`, setup prepares all three platforms. The destination must not exist.

Edit `src/Main.cpp` and `app0/sce_sys/param.json`.

## Build

Replace `<target>` with `windows`, `linux` or `ps5`.

```sh
cmake --preset <target>
cmake --build --preset <target>
```

Desktop executable: `dist/windows/MyApp.exe` or `dist/linux/MyApp`.

Only imported runtime libraries and their dependencies are copied to `libs`. Declare modules loaded by name with `adk_runtime_libraries(MyApp libName)` in `CMakeLists.txt`.

## Package for PS5

```sh
adk package --target ps5
```

Output: `dist/ps5/MyApp.ffpfsc`.

An absent `app0/sce_sys/icon0.png` is generated. `pic0.png` and `pic1.png` are optional (3840x2160). An absent `pic1.png` uses `pic0.png`.

## Complete example

`adk init` creates a project from the [BouncingTitle](../../core/PublicSdk/templates/BouncingTitle) template, which renders the project name on the GPU and animates it bouncing across the screen.

```bat
adk setup .adk
adk init MyApp --deps .adk
cd MyApp

cmake --preset windows
cmake --build --preset windows
dist\windows\MyApp.exe

cmake --preset ps5
cmake --build --preset ps5
adk package --target ps5
```

PS5 image: `dist/ps5/MyApp.ffpfsc`.
