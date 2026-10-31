# Usage

Extract the latest [SDK](https://github.com/boykopovar/AnyDevKit/releases/latest). Add its `bin` directory to `PATH`. Install Python 3.11+, CMake 3.22+, Ninja, Clang and LLD.

`adk setup` is not implemented. `tools/setup.py` downloads [AnyPS5 v0.1.4](https://github.com/boykopovar/AnyPS5/releases/tag/v0.1.4) and [C++ runtime v0.1.3](https://github.com/boykopovar/llvm-cxx-ps/releases/tag/v0.1.3) and prepares C headers and import libraries. To use manually downloaded archives, pass `--cache <directory>`.

```sh
python tools/setup.py D:/AdkDependencies
adk init MyApp --deps D:/AdkDependencies
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

## Package for PS5

```sh
adk package --target ps5
```

Output: `dist/ps5/MyApp.ffpfsc`.

An absent `app0/sce_sys/icon0.png` is generated. `pic0.png` is optional.
