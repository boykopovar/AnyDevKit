#include "Application.hpp"
#include "CommandLine/Arguments.hpp"
#include "Commands/BuildCommand.hpp"
#include "Commands/MetadataCommand.hpp"
#include "Commands/NidCommand.hpp"
#include "Commands/PackagingCommand.hpp"
#include "Commands/ShaderCommand.hpp"
#include <iostream>
#include <map>
#include <stdexcept>
namespace AnyDevKit::PublicSdk {
void Application::Run(std::span<const std::string> arguments) {
    if (arguments.empty() || (arguments.size() == 1 && (arguments.front() == "--help" || arguments.front() == "help"))) {
        std::cout << R"(adk <command> [options]

build --input linked.elf --app0 directory --output file --workspace new-directory
      --format ffpfsc|pkg <metadata> <elf> [self] [pfs or pkg keys]
  Post-link processing only. The ELF entry must point to application main.
  Requires app0/sce_sys/icon0.png (512x512) and pic0.png (3840x2160).
  Produces PS5 ELF, fake-signed SELF, param.json, BC7 DDS and the archive.
  Prepared app0 and application.elf remain in workspace. No compiler is launched.
  app0 must not already contain eboot.bin, param.json, icon0.dds or pic0.dds.

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
shader --input source.glsl --stage vertex|fragment --header header.bin --code code.bin
       [--include directory]... [shader options]
agc --input shader.spv --stage vertex|fragment --header header.bin --code code.bin
    [shader options]
  shader compiles GLSL; agc compiles SPIR-V. Both use the AGC packager.
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
  --companion-sdk-version uint32 --import filename=decrypted-module [...]
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
All outputs must be new; parent directories must exist. Unknown options fail.
)";
        return;
    }
    using Handler = void (*)(std::string_view, Arguments&);
    const std::map<std::string, Handler> commands{
        {"build", BuildCommand::Run}, {"nid", NidCommand::Run}, {"exports", NidCommand::Run},
        {"ps5-elf", PackagingCommand::Run}, {"eboot", PackagingCommand::Run},
        {"pfs", PackagingCommand::Run}, {"ffpfsc", PackagingCommand::Run}, {"pkg", PackagingCommand::Run},
        {"param", MetadataCommand::Run}, {"png", MetadataCommand::Run}, {"dds", MetadataCommand::Run},
        {"windows-startup", MetadataCommand::Run}, {"shader", ShaderCommand::Run}, {"agc", ShaderCommand::Run}
    };
    const auto found = commands.find(arguments.front());
    if (found == commands.end()) throw std::invalid_argument("Unknown command: " + arguments.front());
    Arguments options(arguments.subspan(1));
    found->second(arguments.front(), options);
}
}
