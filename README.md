# AnyDevKit

SDK for developing cross-platform games and applications for PS5, Windows and Linux using the [AnyPS5](https://github.com/boykopovar/AnyPS5) [libs](https://github.com/boykopovar/AnyPS5/tree/5a2744c55efcedf4c5f1b190af46fccbe5df1310/core/libs/prx) API.

- Public API headers from AnyPS5.
- Executable import processing with [nid_patcher](https://github.com/boykopovar/AnyPS5/tree/5a2744c55efcedf4c5f1b190af46fccbe5df1310/core/libs/nid).
- GLSL compilation to PS5 shaders through [glslang](https://github.com/KhronosGroup/glslang/tree/ed883c736e0d5633242218c534264b7eea28b27e) and [opengnm-psbc](https://github.com/mpereiraesaa/opengnm-psbc/tree/4fe7264a0cab9feb531a156e8a810b3e39cfe42c).

## Status

![anydevkit_3dcalc](https://gist.github.com/user-attachments/assets/64808b84-7edf-4598-a253-20efb1e1992e)

[3D calculator](examples/Calculator) (compiled from source) with buttons, vibration feedback, save data and **[AGC shaders](examples/Calculator/shaders)** installs via ffpfsc and works (include two images when building). All rendering runs on GPU.

Documentation and instructions will be added later.

## License

This project is licensed under the GNU General Public License version 2 only.
