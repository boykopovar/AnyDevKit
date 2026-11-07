#include "Graphics/Pipeline.hpp"
#include "Error.hpp"
#include "prx/libSceAgc/libSceAgc.hpp"
#include <array>
#include <bit>
#include <cstring>
#include <algorithm>

namespace {

template<typename TValue>
TValue read(const void* base, std::size_t offset) {
    TValue result;
    std::memcpy(&result, static_cast<const std::byte*>(base) + offset, sizeof(result));
    return result;
}

}

Pipeline::Pipeline(Memory& memory, int width, int height, Shader* vertex, Shader* fragment) {
    context = static_cast<ShaderRegister*>(memory.Allocate(ContextCapacity * sizeof(ShaderRegister), 256));
    primitive = static_cast<ShaderRegister*>(memory.Allocate(3 * sizeof(ShaderRegister), 256));
    const auto* defaults = sceAgcGetRegisterDefaults();
    if (!defaults) throw Error{"AGC register defaults are missing"};
    const auto* table = read<const ShaderRegister* const*>(defaults, 0);
    const auto* types = read<const std::uint32_t*>(defaults, 48);
    const auto count = read<std::uint32_t>(defaults, 56);
    if (!table || !types || count == 0 || count > 1024) throw Error{"Invalid AGC register defaults"};
    const ShaderRegister* target = nullptr;

    for (std::uint32_t i = 0; i < count; ++i) {
        if (types[i * 2] != 0x38e92c91u) continue;
        if (target || (types[i * 2 + 1] & 3u) != 0 || (types[i * 2 + 1] >> 2) >= count) throw Error{"Invalid AGC color target defaults"};
        target = table[types[i * 2 + 1] >> 2];
    }

    if (!target) throw Error{"AGC color target defaults are missing"};
    constexpr std::array<std::uint32_t, 16> offsets{0x318, 0x31b, 0x31c, 0x31d, 0x31e, 0x31f, 0x321, 0x323, 0x324, 0x325, 0x390, 0x398, 0x3a0, 0x3a8, 0x3b0, 0x3b8};
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        if (target[i].offset != offsets[i]) throw Error{"Unexpected AGC color register layout"};
        context[contextCount++] = target[i];
    }

    context[0].value = 0;
    context[1].value &= 0xfc001fffu;
    context[2].value = (context[2].value & ~0x1005df7cu) | 0x8828u;
    context[3].value &= ~0x1f000u;
    context[4].value = (context[4].value & ~0x18026cu) | 0x48u;
    context[5].value = context[6].value = context[9].value = 0;
    context[10].value = context[10].value & ~0xffu;
    context[11].value &= ~0xffu;
    context[12].value &= ~0xffu;
    context[13].value &= ~0xffu;
    context[14].value = static_cast<std::uint32_t>(height - 1) | (static_cast<std::uint32_t>(width - 1) << 14);
    context[15].value = (context[15].value & ~0x4707dfffu) | 0x4506c000u;
    Check(sceAgcLinkShaders(context + contextCount, primitive, nullptr, vertex, fragment, 4), "sceAgcLinkShaders");
    contextCount += 34;
    if (contextCount + vertex->num_cx_registers + fragment->num_cx_registers > ContextCapacity) throw Error{"AGC shader register storage exhausted"};

    for (const auto* shader : {vertex, fragment}) {
        for (std::uint32_t i = 0; i < shader->num_cx_registers; ++i) context[contextCount++] = shader->cx_registers[i];
    }

    const auto extent = static_cast<std::uint32_t>(width) | (static_cast<std::uint32_t>(height) << 16);
    const auto halfWidth = std::bit_cast<std::uint32_t>(static_cast<float>(width) * 0.5f);
    const auto halfHeight = std::bit_cast<std::uint32_t>(static_cast<float>(height) * 0.5f);
    const std::array state{
        ShaderRegister{0x10f, halfWidth}, ShaderRegister{0x110, halfWidth},
        ShaderRegister{0x111, halfHeight}, ShaderRegister{0x112, halfHeight},
        ShaderRegister{0x113, 0x3f800000}, ShaderRegister{0x114, 0},
        ShaderRegister{0xb4, 0}, ShaderRegister{0xb5, 0x3f800000},
        ShaderRegister{0x90, 0x80000000}, ShaderRegister{0x91, extent},
        ShaderRegister{0x94, 0x80000000}, ShaderRegister{0x95, extent},
        ShaderRegister{0xc, 0}, ShaderRegister{0xd, extent},
        ShaderRegister{0x8e, 15}, ShaderRegister{0x200, 0},
        ShaderRegister{0x202, 0xcc0010},
        ShaderRegister{0x205, 0x240}, ShaderRegister{0x206, 0x43f},
        ShaderRegister{0x204, 0x1080000}, ShaderRegister{0x292, 0x22},
        ShaderRegister{0x2f9, 0x2d}, ShaderRegister{0x1e0, 0},
        ShaderRegister{0x2fa, 0x3f800000}, ShaderRegister{0x2fb, 0x3f800000},
        ShaderRegister{0x2fc, 0x3f800000}, ShaderRegister{0x2fd, 0x3f800000}
    };
    if (contextCount + state.size() > ContextCapacity) throw Error{"AGC register storage exhausted"};

    for (const auto value : state) context[contextCount++] = value;
}

void Pipeline::Bind(CommandBuffer& buffer, void* target, std::span<ShaderRegister> storage) {
    if (storage.size() < contextCount) throw Error{"Pipeline snapshot storage exhausted"};
    std::copy_n(context, contextCount, storage.data());
    const auto address = reinterpret_cast<std::uintptr_t>(target);
    if (address % Memory::Alignment != 0 || address >> 48 != 0) throw Error{"Invalid display target address"};
    storage[0].value = static_cast<std::uint32_t>(address >> 8);
    storage[10].value = (storage[10].value & ~0xffu) | static_cast<std::uint32_t>(address >> 40);
    sceAgcDcbSetCxRegistersIndirect(&buffer, storage.data(), contextCount);
    sceAgcDcbSetUcRegistersIndirect(&buffer, primitive, 3);
}

