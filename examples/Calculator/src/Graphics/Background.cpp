#include "Graphics/Background.hpp"
#include "CalculatorShaders.hpp"
#include "CalculatorError.hpp"
#include "Canvas.hpp"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include "prx/libkernel/libkernel.hpp"
#include "prx/libc/include/Api.hpp"

#include <atomic>
#include <bit>

namespace {

constexpr std::size_t MemorySize = 32 * 1024 * 1024;
constexpr std::size_t Alignment = 131072;
constexpr std::size_t CommandWords = 16384;
constexpr std::size_t RegisterCapacity = 256;

void check(int result, const char* operation) {
    if (result != 0) throw CalculatorError{operation, result};
}

template<typename TValue>
TValue read(const void* base, std::size_t offset) {
    TValue result;
    memcpy_nid_postfix(&result, static_cast<const std::byte*>(base) + offset, sizeof(result));
    return result;
}

bool APS5_VABI exhausted(CommandBuffer*, std::uint32_t, void*) {
    throw CalculatorError{"AGC command buffer exhausted"};
}

}

Background::Background(int width, int height) : width(width), height(height) {
    check(sceAgcInit(11), "sceAgcInit");
    check(sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), MemorySize, Alignment, 12, &physicalAddress), "sceKernelAllocateDirectMemory");
    void* address = reinterpret_cast<void*>(0x240000000ull);
    const auto result = sceKernelMapDirectMemory(&address, MemorySize, 0x33, 0, physicalAddress, Alignment);
    if (result != 0) {
        check(sceKernelReleaseDirectMemory(physicalAddress, MemorySize), "sceKernelReleaseDirectMemory");
        check(result, "sceKernelMapDirectMemory");
    }
    memory = static_cast<std::byte*>(address);
    try {
        if ((reinterpret_cast<std::uintptr_t>(memory) >> 32) != 2 || ((reinterpret_cast<std::uintptr_t>(memory) + MemorySize - 1) >> 32) != 2) throw CalculatorError{"AGC requires memory in the shader address aperture"};
        memset_nid_postfix(memory, 0, MemorySize);
        const auto size = Canvas::StorageSize(width, height);
        pixels = {static_cast<std::uint32_t*>(allocate(size, Alignment)), size / 4};
        commands = static_cast<std::uint32_t*>(allocate(CommandWords * 4, 256));
        fence = static_cast<std::uint64_t*>(allocate(sizeof(std::uint64_t), 64));
        constants = static_cast<float*>(allocate(16, 16));
        context = static_cast<ShaderRegister*>(allocate(RegisterCapacity * sizeof(ShaderRegister), 256));
        primitive = static_cast<ShaderRegister*>(allocate(3 * sizeof(ShaderRegister), 256));
        vertex = createShader(CalculatorShaders::Vertex);
        fragment = createShader(CalculatorShaders::Fragment);
        createTarget();
    } catch (...) {
        check(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(memory), MemorySize), "sceKernelMunmap");
        check(sceKernelReleaseDirectMemory(physicalAddress, MemorySize), "sceKernelReleaseDirectMemory");
        throw;
    }
}

Background::~Background() noexcept(false) {
    if (pending) return;
    check(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(memory), MemorySize), "sceKernelMunmap");
    check(sceKernelReleaseDirectMemory(physicalAddress, MemorySize), "sceKernelReleaseDirectMemory");
}

void* Background::allocate(std::size_t bytes, std::size_t alignment) {
    const auto offset = (used + alignment - 1) & ~(alignment - 1);
    if (offset > MemorySize || bytes > MemorySize - offset) throw CalculatorError{"AGC allocation exceeds arena"};
    used = offset + bytes;
    return memory + offset;
}

Shader* Background::createShader(const ShaderAsset& asset) {
    auto* header = allocate(asset.Header.size(), 8);
    auto* code = allocate(asset.Code.size(), 256);
    memcpy_nid_postfix(header, asset.Header.data(), asset.Header.size());
    memcpy_nid_postfix(code, asset.Code.data(), asset.Code.size());
    Shader* shader = nullptr;
    check(sceAgcCreateShader(&shader, header, code), "sceAgcCreateShader");
    if (!shader || !shader->cx_registers || !shader->sh_registers) throw CalculatorError{"AGC returned an incomplete shader"};
    return shader;
}

void Background::createTarget() {
    const auto* defaults = sceAgcGetRegisterDefaults();
    if (!defaults) throw CalculatorError{"AGC register defaults are missing"};
    const auto* table = read<const ShaderRegister* const*>(defaults, 0);
    const auto* types = read<const std::uint32_t*>(defaults, 48);
    const auto count = read<std::uint32_t>(defaults, 56);
    if (!table || !types || count == 0 || count > 1024) throw CalculatorError{"Invalid AGC register defaults"};
    const ShaderRegister* target = nullptr;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (types[i * 2] != 0x38e92c91u) continue;
        if (target || (types[i * 2 + 1] & 3u) != 0 || (types[i * 2 + 1] >> 2) >= count) throw CalculatorError{"Invalid AGC color target defaults"};
        target = table[types[i * 2 + 1] >> 2];
    }
    if (!target) throw CalculatorError{"AGC color target defaults are missing"};
    constexpr std::array<std::uint32_t, 16> offsets{0x318, 0x31b, 0x31c, 0x31d, 0x31e, 0x31f, 0x321, 0x323, 0x324, 0x325, 0x390, 0x398, 0x3a0, 0x3a8, 0x3b0, 0x3b8};
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        if (target[i].offset != offsets[i]) throw CalculatorError{"Unexpected AGC color register layout"};
        context[contextCount++] = target[i];
    }
    const auto address = reinterpret_cast<std::uintptr_t>(pixels.data());
    context[0].value = static_cast<std::uint32_t>(address >> 8);
    context[1].value &= 0xfc001fffu;
    context[2].value = (context[2].value & ~0x1005df7cu) | 0x8828u;
    context[3].value &= ~0x1f000u;
    context[4].value = (context[4].value & ~0x18026cu) | 0x48u;
    context[5].value = context[6].value = context[9].value = 0;
    context[10].value = (context[10].value & ~0xffu) | static_cast<std::uint32_t>(address >> 40);
    context[11].value &= ~0xffu;
    context[12].value &= ~0xffu;
    context[13].value &= ~0xffu;
    context[14].value = static_cast<std::uint32_t>(height - 1) | (static_cast<std::uint32_t>(width - 1) << 14);
    context[15].value = (context[15].value & ~0x4707dfffu) | 0x4506c000u;
    check(sceAgcLinkShaders(context + contextCount, primitive, nullptr, vertex, fragment, 4), "sceAgcLinkShaders");
    contextCount += 34;
    if (contextCount + vertex->num_cx_registers + fragment->num_cx_registers > RegisterCapacity) throw CalculatorError{"AGC shader register storage exhausted"};
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
    if (contextCount + state.size() > RegisterCapacity) throw CalculatorError{"AGC register storage exhausted"};
    for (const auto value : state) context[contextCount++] = value;
}

std::span<const std::uint32_t> Background::Draw(float seconds) {
    if (pending) throw CalculatorError{"Previous AGC submission is still pending"};
    constants[0] = static_cast<float>(width);
    constants[1] = static_cast<float>(height);
    constants[2] = seconds;
    std::atomic_ref completion(*fence);
    completion.store(0, std::memory_order_release);
    CommandBuffer buffer{commands, commands + CommandWords, commands, commands + CommandWords, exhausted, nullptr, 0};
    sceAgcDcbContextStateOp(&buffer, 0);
    sceAgcDcbAcquireMem(&buffer, 1, 0, 0x4381, memory, MemorySize, 40);
    sceAgcDcbSetCxRegistersIndirect(&buffer, context, contextCount);
    sceAgcDcbSetUcRegistersIndirect(&buffer, primitive, 3);
    for (const auto* shader : {vertex, fragment}) sceAgcDcbSetShRegistersIndirect(&buffer, shader->sh_registers, shader->num_sh_registers);
    const auto bind = [&](const ShaderAsset& asset, std::uint32_t base) {
        for (std::uint32_t i = 0; i < asset.UserDataCount; ++i) {
            const auto value = static_cast<int>(i) == asset.PushConstantSlot ? static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(constants)) : asset.UserData[i];
            sceAgcDcbSetShRegisterDirect(&buffer, {base + i, value});
        }
    };
    bind(CalculatorShaders::Vertex, 0x8c);
    bind(CalculatorShaders::Fragment, 0xc);
    sceAgcDcbSetNumInstances(&buffer, 1);
    sceAgcDcbDrawIndexAuto(&buffer, 3, 0);
    sceAgcCbReleaseMem(&buffer, 0x2d, 0x70f, 0, 0, reinterpret_cast<Label*>(fence), 2, 1, 0, 0, 3, 0);
    const Packet packet{commands, static_cast<std::uint32_t>(buffer.cursor_up - commands), 0, {}};
    std::atomic_thread_fence(std::memory_order_seq_cst);
    pending = true;
    check(sceAgcDriverSubmitDcb(&packet), "sceAgcDriverSubmitDcb");
    const auto start = sceKernelGetProcessTime();
    while (completion.load(std::memory_order_acquire) != 1) {
        if (sceKernelGetProcessTime() - start > 5000000) throw CalculatorError{"AGC rendering timed out"};
        check(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
    }
    pending = false;
    return pixels;
}
