#include "prx/libkernel/Time/include/Time.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "Graphics/Renderer.hpp"
#include "Calculator3DShaders.hpp"
#include "Calculator3DError.hpp"
#include "Canvas.hpp"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include <cstring>

#include <atomic>
#include <bit>

namespace {

constexpr std::size_t MemorySize = 32 * 1024 * 1024;
constexpr std::size_t Alignment = 131072;
constexpr std::size_t CommandWords = 65536;
constexpr std::size_t RegisterCapacity = 256;
constexpr std::size_t RegionCapacity = 8192;

void check(int result, const char* operation) {
    if (result != 0) throw Calculator3DError{operation, result};
}

template<typename TValue>
TValue read(const void* base, std::size_t offset) {
    TValue result;
    std::memcpy(&result, static_cast<const std::byte*>(base) + offset, sizeof(result));
    return result;
}

bool APS5_VABI exhausted(CommandBuffer*, std::uint32_t, void*) {
    throw Calculator3DError{"AGC command buffer exhausted"};
}

}

Renderer::Renderer(int width, int height) : width(width), height(height) {
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
        if ((reinterpret_cast<std::uintptr_t>(memory) >> 32) != 2 || ((reinterpret_cast<std::uintptr_t>(memory) + MemorySize - 1) >> 32) != 2) throw Calculator3DError{"AGC requires memory in the shader address aperture"};
        std::memset(memory, 0, MemorySize);
        commands = static_cast<std::uint32_t*>(allocate(CommandWords * 4, 256));
        fence = static_cast<std::uint64_t*>(allocate(sizeof(std::uint64_t), 64));
        constants = static_cast<float*>(allocate((4 + BackgroundPhysics::Scene::TransformFloats) * sizeof(float), 16));
        context = static_cast<ShaderRegister*>(allocate(RegisterCapacity * sizeof(ShaderRegister), 256));
        primitive = static_cast<ShaderRegister*>(allocate(3 * sizeof(ShaderRegister), 256));
        vertex = createShader(Calculator3DShaders::Vertex);
        fragment = createShader(Calculator3DShaders::Fragment);
        createPipeline(context, contextCount, primitive, vertex, fragment);
        interfaceVertex = createShader(Calculator3DShaders::InterfaceVertex);
        interfaceFragment = createShader(Calculator3DShaders::InterfaceFragment);
        interfaceContext = static_cast<ShaderRegister*>(allocate(RegisterCapacity * sizeof(ShaderRegister), 256));
        interfacePrimitive = static_cast<ShaderRegister*>(allocate(3 * sizeof(ShaderRegister), 256));
        interfaceCommands = static_cast<DrawCommand*>(allocate(Canvas::Capacity * sizeof(DrawCommand), alignof(DrawCommand)));
        backgroundRegions = static_cast<std::array<float, 4>*>(allocate(RegionCapacity * sizeof(std::array<float, 4>), 16));
        backgroundDescriptor = static_cast<std::uint32_t*>(allocate(32, 16));
        const auto regionAddress = reinterpret_cast<std::uintptr_t>(backgroundRegions);
        backgroundDescriptor[0] = static_cast<std::uint32_t>(regionAddress);
        backgroundDescriptor[1] = static_cast<std::uint32_t>(regionAddress >> 32);
        backgroundDescriptor[3] = 0x31016fac;
        const auto frameAddress = reinterpret_cast<std::uintptr_t>(constants);
        backgroundDescriptor[4] = static_cast<std::uint32_t>(frameAddress);
        backgroundDescriptor[5] = static_cast<std::uint32_t>(frameAddress >> 32);
        backgroundDescriptor[6] = (4 + BackgroundPhysics::Scene::TransformFloats) * sizeof(float);
        backgroundDescriptor[7] = 0x31016fac;
        interfaceDescriptor = static_cast<std::uint32_t*>(allocate(16, 16));
        const auto commandAddress = reinterpret_cast<std::uintptr_t>(interfaceCommands);
        interfaceDescriptor[0] = static_cast<std::uint32_t>(commandAddress);
        interfaceDescriptor[1] = static_cast<std::uint32_t>(commandAddress >> 32);
        interfaceDescriptor[3] = 0x31016fac;
        createPipeline(interfaceContext, interfaceContextCount, interfacePrimitive, interfaceVertex, interfaceFragment);
        slots[0] = {commands, fence, constants, context, interfaceContext, interfaceDescriptor, interfaceCommands, backgroundRegions, backgroundDescriptor, used, false};
        std::construct_at(fence, 0);

        for (std::size_t index = 1; index < slots.size(); ++index) {
            auto& slot = slots[index];
            slot.Commands = static_cast<std::uint32_t*>(allocate(CommandWords * sizeof(std::uint32_t), Alignment));
            slot.Fence = static_cast<std::uint64_t*>(allocate(sizeof(std::uint64_t), 64));
            std::construct_at(slot.Fence, 0);
            slot.Constants = static_cast<float*>(allocate((4 + BackgroundPhysics::Scene::TransformFloats) * sizeof(float), 16));
            slot.Context = static_cast<ShaderRegister*>(allocate(RegisterCapacity * sizeof(ShaderRegister), 256));
            std::memcpy(slot.Context, context, contextCount * sizeof(ShaderRegister));
            slot.InterfaceContext = static_cast<ShaderRegister*>(allocate(RegisterCapacity * sizeof(ShaderRegister), 256));
            std::memcpy(slot.InterfaceContext, interfaceContext, interfaceContextCount * sizeof(ShaderRegister));
            slot.InterfaceCommands = static_cast<DrawCommand*>(allocate(Canvas::Capacity * sizeof(DrawCommand), alignof(DrawCommand)));
            slot.BackgroundRegions = static_cast<std::array<float, 4>*>(allocate(RegionCapacity * sizeof(std::array<float, 4>), 16));
            slot.BackgroundDescriptor = static_cast<std::uint32_t*>(allocate(32, 16));
            const auto regionsAddress = reinterpret_cast<std::uintptr_t>(slot.BackgroundRegions);
            const auto constantsAddress = reinterpret_cast<std::uintptr_t>(slot.Constants);
            const std::array backgroundValues{static_cast<std::uint32_t>(regionsAddress), static_cast<std::uint32_t>(regionsAddress >> 32), 0u, 0x31016facu, static_cast<std::uint32_t>(constantsAddress), static_cast<std::uint32_t>(constantsAddress >> 32), static_cast<std::uint32_t>((4 + BackgroundPhysics::Scene::TransformFloats) * sizeof(float)), 0x31016facu};
            std::memcpy(slot.BackgroundDescriptor, backgroundValues.data(), sizeof(backgroundValues));
            slot.InterfaceDescriptor = static_cast<std::uint32_t*>(allocate(16, 16));
            const auto commandsAddress = reinterpret_cast<std::uintptr_t>(slot.InterfaceCommands);
            const std::array interfaceValues{static_cast<std::uint32_t>(commandsAddress), static_cast<std::uint32_t>(commandsAddress >> 32), 0u, 0x31016facu};
            std::memcpy(slot.InterfaceDescriptor, interfaceValues.data(), sizeof(interfaceValues));
            slot.Bytes = used - static_cast<std::size_t>(reinterpret_cast<std::byte*>(slot.Commands) - memory);
        }
    } catch (...) {
        check(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(memory), MemorySize), "sceKernelMunmap");
        check(sceKernelReleaseDirectMemory(physicalAddress, MemorySize), "sceKernelReleaseDirectMemory");
        throw;
    }
}

Renderer::~Renderer() noexcept(false) {
    for (auto& slot : slots) waitSlot(slot);
    check(sceKernelMunmap(reinterpret_cast<std::uintptr_t>(memory), MemorySize), "sceKernelMunmap");
    check(sceKernelReleaseDirectMemory(physicalAddress, MemorySize), "sceKernelReleaseDirectMemory");
}

void* Renderer::allocate(std::size_t bytes, std::size_t alignment) {
    const auto offset = (used + alignment - 1) & ~(alignment - 1);
    if (offset > MemorySize || bytes > MemorySize - offset) throw Calculator3DError{"AGC allocation exceeds arena"};
    used = offset + bytes;
    return memory + offset;
}

Shader* Renderer::createShader(const ShaderAsset& asset) {
    auto* header = allocate(asset.Header.size(), 8);
    auto* code = allocate(asset.Code.size(), 256);
    std::memcpy(header, asset.Header.data(), asset.Header.size());
    std::memcpy(code, asset.Code.data(), asset.Code.size());
    Shader* shader = nullptr;
    check(sceAgcCreateShader(&shader, header, code), "sceAgcCreateShader");
    if (!shader || !shader->cx_registers || !shader->sh_registers) throw Calculator3DError{"AGC returned an incomplete shader"};
    return shader;
}

void Renderer::createPipeline(ShaderRegister* context, std::uint32_t& contextCount, ShaderRegister* primitive, Shader* vertex, Shader* fragment) {
    const auto* defaults = sceAgcGetRegisterDefaults();
    if (!defaults) throw Calculator3DError{"AGC register defaults are missing"};
    const auto* table = read<const ShaderRegister* const*>(defaults, 0);
    const auto* types = read<const std::uint32_t*>(defaults, 48);
    const auto count = read<std::uint32_t>(defaults, 56);
    if (!table || !types || count == 0 || count > 1024) throw Calculator3DError{"Invalid AGC register defaults"};
    const ShaderRegister* target = nullptr;

    for (std::uint32_t i = 0; i < count; ++i) {
        if (types[i * 2] != 0x38e92c91u) continue;
        if (target || (types[i * 2 + 1] & 3u) != 0 || (types[i * 2 + 1] >> 2) >= count) throw Calculator3DError{"Invalid AGC color target defaults"};
        target = table[types[i * 2 + 1] >> 2];
    }

    if (!target) throw Calculator3DError{"AGC color target defaults are missing"};
    constexpr std::array<std::uint32_t, 16> offsets{0x318, 0x31b, 0x31c, 0x31d, 0x31e, 0x31f, 0x321, 0x323, 0x324, 0x325, 0x390, 0x398, 0x3a0, 0x3a8, 0x3b0, 0x3b8};
    for (std::size_t i = 0; i < offsets.size(); ++i) {
        if (target[i].offset != offsets[i]) throw Calculator3DError{"Unexpected AGC color register layout"};
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
    check(sceAgcLinkShaders(context + contextCount, primitive, nullptr, vertex, fragment, 4), "sceAgcLinkShaders");
    contextCount += 34;
    if (contextCount + vertex->num_cx_registers + fragment->num_cx_registers > RegisterCapacity) throw Calculator3DError{"AGC shader register storage exhausted"};

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
    if (contextCount + state.size() > RegisterCapacity) throw Calculator3DError{"AGC register storage exhausted"};

    for (const auto value : state) context[contextCount++] = value;
}

void Renderer::waitSlot(DrawSlot& slot) {
    if (!slot.Pending) return;
    const auto start = sceKernelGetProcessTime();
    std::atomic_ref completion(*slot.Fence);

    while (completion.load(std::memory_order_acquire) != 1) {
        if (sceKernelGetProcessTime() - start > 5000000) throw Calculator3DError{"AGC rendering timed out"};
        check(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
    }

    slot.Pending = false;
}

void Renderer::selectSlot(std::size_t index) {
    auto& slot = slots.at(index);
    waitSlot(slot);
    commands = slot.Commands;
    fence = slot.Fence;
    constants = slot.Constants;
    context = slot.Context;
    interfaceContext = slot.InterfaceContext;
    interfaceDescriptor = slot.InterfaceDescriptor;
    interfaceCommands = slot.InterfaceCommands;
    backgroundRegions = slot.BackgroundRegions;
    backgroundDescriptor = slot.BackgroundDescriptor;
}

void Renderer::Draw(Display& display, float seconds, const Canvas& canvas) {
    selectSlot(slotIndex);
    const auto pixels = display.Acquire();
    const auto address = reinterpret_cast<std::uintptr_t>(pixels.data());
    if (pixels.size_bytes() != Canvas::StorageSize(width, height) || address % Alignment != 0 || address >> 48 != 0) throw Calculator3DError{"Invalid AGC display target"};

    for (auto* registers : {context, interfaceContext}) {
        registers[0].value = static_cast<std::uint32_t>(address >> 8);
        registers[10].value = (registers[10].value & ~0xffu) | static_cast<std::uint32_t>(address >> 40);
    }

    const auto draws = canvas.Commands();
    if (draws.size() > Canvas::Capacity) throw Calculator3DError{"Too many interface draws"};
    const auto regions = visibility.Update(width, height, draws);
    if (regions.size() > RegionCapacity) throw Calculator3DError{"Background region capacity exceeded"};
    std::memcpy(backgroundRegions, regions.data(), regions.size_bytes());
    backgroundDescriptor[2] = static_cast<std::uint32_t>(regions.size_bytes());
    interfaceDescriptor[2] = static_cast<std::uint32_t>(draws.size_bytes());
    std::memcpy(interfaceCommands, draws.data(), draws.size_bytes());
    constants[0] = static_cast<float>(width);
    constants[1] = static_cast<float>(height);
    constants[2] = seconds;
    physics.Update(seconds);
    physics.WriteTransforms(std::span<float, BackgroundPhysics::Scene::TransformFloats>{constants + 4, BackgroundPhysics::Scene::TransformFloats});
    std::atomic_ref completion(*fence);
    completion.store(0, std::memory_order_release);
    CommandBuffer buffer{commands, commands + CommandWords, commands, commands + CommandWords, exhausted, nullptr, 0};
    sceAgcDcbContextStateOp(&buffer, 0);
    sceAgcDcbAcquireMem(&buffer, 1, 0, 0x4381, shaderMemoryPending ? memory : reinterpret_cast<std::byte*>(commands), shaderMemoryPending ? MemorySize : slots[slotIndex].Bytes, 40);
    sceAgcDcbSetCxRegistersIndirect(&buffer, context, contextCount);
    sceAgcDcbSetUcRegistersIndirect(&buffer, primitive, 3);

    for (const auto* shader : {vertex, fragment}) sceAgcDcbSetShRegistersIndirect(&buffer, shader->sh_registers, shader->num_sh_registers);
    const auto bind = [&](const ShaderAsset& asset, std::uint32_t base, const void* pushConstants, const void* descriptors) {
        for (std::uint32_t i = 0; i < asset.UserDataCount; ++i) {
            auto value = static_cast<int>(i) == asset.PushConstantSlot ? static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(pushConstants)) : asset.UserData[i];
            if (static_cast<int>(i) == asset.DescriptorSlot) value = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(descriptors));
            sceAgcDcbSetShRegisterDirect(&buffer, {base + i, value});
        }
    };
    bind(Calculator3DShaders::Vertex, 0x8c, constants, backgroundDescriptor);
    bind(Calculator3DShaders::Fragment, 0xc, constants, backgroundDescriptor);
    sceAgcDcbSetNumInstances(&buffer, 1);
    if (!regions.empty()) sceAgcDcbDrawIndexAuto(&buffer, static_cast<std::uint32_t>(regions.size() * 6), 0);
    sceAgcDcbSetCxRegistersIndirect(&buffer, interfaceContext, interfaceContextCount);
    sceAgcDcbSetUcRegistersIndirect(&buffer, interfacePrimitive, 3);

    for (const auto* shader : {interfaceVertex, interfaceFragment}) sceAgcDcbSetShRegistersIndirect(&buffer, shader->sh_registers, shader->num_sh_registers);
    bind(Calculator3DShaders::InterfaceVertex, 0x8c, interfaceCommands, interfaceDescriptor);
    bind(Calculator3DShaders::InterfaceFragment, 0xc, interfaceCommands, interfaceDescriptor);
    sceAgcDcbDrawIndexAuto(&buffer, static_cast<std::uint32_t>(draws.size() * 6), 0);
    const auto flip = display.PrepareFlip();
    sceAgcDcbSetFlip(&buffer, flip.Handle, flip.BufferIndex, 1, flip.Argument);
    sceAgcCbReleaseMem(&buffer, 0x2d, 0x70f, 0, 0, reinterpret_cast<Label*>(fence), 2, 1, 0, 0, 3, 0);
    const Packet packet{commands, static_cast<std::uint32_t>(buffer.cursor_up - commands), 0, {}};
    std::atomic_thread_fence(std::memory_order_seq_cst);
    slots[slotIndex].Pending = true;
    check(sceAgcDriverSubmitDcb(&packet), "sceAgcDriverSubmitDcb");
    shaderMemoryPending = false;
    slotIndex = (slotIndex + 1) % slots.size();
    display.CompleteFlip();
}

