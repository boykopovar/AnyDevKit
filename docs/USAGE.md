# Usage

> Planned workflow. Not yet available.

Extract the latest [SDK](https://github.com/boykopovar/AnyDevKit/releases/latest). Add its `bin` directory to `PATH`. Install Python, CMake, Ninja, Clang and LLD.

`adk` does not download dependencies yet. Use `tools/setup.py` or download the latest [AnyPS5](https://github.com/boykopovar/AnyPS5/releases/latest) and [C++ runtime](https://github.com/boykopovar/llvm-cxx-ps/releases/latest) releases manually.

```sh
python tools/setup.py D:/AdkDependencies
adk init MyApp --dependencies D:/AdkDependencies
cd MyApp
```

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
