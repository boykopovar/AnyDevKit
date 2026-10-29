#include "CommandLine/Help.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
namespace AnyDevKit::PublicSdk {
void Help::Print(std::string_view command) {
    constexpr std::string_view text = R"HELP(adk <command> [options]

init Name --deps directory [--directory path] [--sdk path] [--title-id ID]
  Creates editable CMake files, adk.json and app0/sce_sys/param.json.
package --target ps5 [--project directory] [build options]
  Reads adk.json; CLI options override project settings. Does not compile.
setup --target windows|linux|ps5 [--project directory]
  Not implemented. Always fails; install release dependencies manually.
update [--project directory]
  Not implemented. Always fails; no lock file or downloads are created.

build --input linked.elf --app0 directory --output file --workspace new-directory
      --format ffpfsc|pkg (--param param.json or <metadata>) <elf> [self] [pfs or pkg keys]
  Post-link processing only. The ELF entry must point to application main.
  Optional app0/sce_sys/icon0.png (512x512); generated when absent.
  Optional app0/sce_sys/pic0.png (3840x2160); omitted when absent.
  Produces PS5 ELF, fake-signed SELF, param.json, BC7 DDS and the archive.
  Prepared app0 and application.elf remain in workspace. No compiler is launched.
  app0 must not already contain eboot.bin, param.json, icon0.dds, pic0.dds or keystone.

nid --input binary --output binary [--exec [--only-sce]] [--library name]
    [--preserve-exports binary]... [--preserve-export-list text]...
  Automatically detects PE, ELF or Mach-O. Library mode requires --library.
  Uses AnyPS5 NID rules. Source remains unchanged.
exports --input binary --output text
ps5-elf --input linked.elf --output application.elf <elf>
eboot --input application.elf --output eboot.bin [self]
param --output param.json <metadata>
png --input image.png --output normalized.png
dds --input image.png --output image.dds
windows-startup --output startup.obj
  Emits the COFF thunk object. Link with AnyDevKitWindowsStartup through CMake.
pfs --input directory --output image [pfs]
ffpfsc --input directory --output image --workspace new-directory [pfs]
pkg --input prepared-app0 --output image --workspace new-directory <metadata> <keys>
    [--timestamp seconds]
shader --input source.glsl --stage vertex|fragment|compute --header header.bin --code code.bin
       --metadata bindings.adk [--include directory]... [shader options]
spirv --input shader.spv --stage vertex|fragment|compute --header header.bin --code code.bin
      --metadata bindings.adk [shader options]
shader-pipeline --vertex source.vert (--geometry source.geom or --control source.tesc --evaluation source.tese) --output new-directory
    [--entry name] [--address32-hi uint32] [--primitive-type uint32] [--include directory]...
    Tessellation requires --patch-control-points uint32.

agc --input machine-code.bin --metadata bindings.adk --header header.bin --code code.bin
  shader compiles GLSL; spirv compiles SPIR-V. Both use the AGC packager.
  agc only packages machine code and the versioned text binding metadata.
  Header and code are separate; runtime alignment requirements are 8 and 256.
  Advanced resource layouts remain available through the C++ ShaderCompiler API.

metadata:
  --title-id ID --content-id ID --concept-id ID
  --content-version NN.NNN.NNN --master-version NN.NN
  --default-language language --title language=text [--title language=text]...
  --firmware-version uint64 --sdk-version uint64
  [--category uint32] [--attribute uint32] [--attribute2 uint32] [--attribute3 uint32]
  [--flexible-memory bytes]
elf:
  --module-name name --file-name name --elf-sdk-version uint32
  --companion-sdk-version uint32
  Dependencies come from ELF DT_NEEDED: libc.prx -> libSceLibcInternal.sprx; .prx -> .sprx.
  [--select-import symbol,module,library]...
self:
  [--authority uint64 (default 0x3100000000000002)]
  [--application-version uint64 (default 0)] [--self-firmware-version uint64 (default 0)]
pfs:
  [--block-size bytes (default 65536)] [--timestamp seconds (default 0)]
  [--case-insensitive]
keys:
  --passcode-moduli file (7 concatenated big-endian 384-byte RSA public moduli)
  --mount-modulus file (one big-endian 384-byte RSA public modulus)
  --passcode hex32 --image-seed hex32
shader options:
  [--entry name (default main)] [--address32-hi uint32 (default 2)]
  [--primitive-type uint32 (default 4)] [--samples uint32 (default 1)]
  [--color-format uint32 (default 9 for fragment, 0 for vertex)]

Integers accept decimal or 0x hexadecimal. Repeat only the options marked [...].
Boolean flags accept an optional true or false value.
All outputs must be new; parent directories must exist. Unknown options fail.
)HELP";
    if (command.empty()) { std::cout << text; return; }
    const std::set<std::string_view> commands{"init", "setup", "update", "package", "build", "nid", "exports", "ps5-elf", "eboot", "param", "png", "dds", "windows-startup", "pfs", "ffpfsc", "pkg", "shader", "shader-pipeline", "spirv", "agc"};
    if (!commands.contains(command)) throw std::invalid_argument("Unknown command: " + std::string(command));
    const auto start = text.find("\n" + std::string(command) + " ");
    auto end = text.find('\n', start + 1);
    while (end != std::string_view::npos && end + 1 < text.size() && text[end + 1] == ' ') end = text.find('\n', end + 1);
    std::cout << "adk " << text.substr(start + 1, end - start - 1) << '\n';
    if (command == "package" || command == "build" || command == "pkg" || command == "param" || command == "ps5-elf" || command == "eboot" || command == "pfs" || command == "ffpfsc" || command == "shader" || command == "spirv") std::cout << text.substr(text.find("\nmetadata:"));
}
}
