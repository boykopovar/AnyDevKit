#include "Graphics/Renderer.hpp"
#include "Graphics/Error.hpp"
#include "Calculator3DShaders.hpp"
#include "prx/libkernel/Time/include/Time.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "prx/libSceAgc/libSceAgc.hpp"
#include "prx/libSceAgcDriver/Submit/include/Dcb.hpp"
#include <atomic>
#include <cstring>
#include <algorithm>

namespace {

constexpr std::size_t CommandWords = 4096;
bool APS5_VABI exhausted(CommandBuffer*, std::uint32_t, void*) { throw Calculator3DError{"AGC command buffer exhausted"}; }
void bindShader(CommandBuffer& buffer, std::uint32_t base, const std::array<std::uint32_t, 16>& values, std::uint32_t count, int slot, const void* descriptors) {
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto value = static_cast<int>(index) == slot ? static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(descriptors)) : values[index];
        sceAgcDcbSetShRegisterDirect(&buffer, {base + index, value});
    }
}

}

Renderer::Renderer(int width, int height) : memory(8 * Memory::Alignment, 0x240000000ull) {
    const auto address = reinterpret_cast<std::uintptr_t>(memory.Data());
    if (address >> 32 != 2 || (address + memory.Size() - 1) >> 32 != 2) throw Calculator3DError{"GPU memory is outside shader aperture"};
    Check(sceAgcInit(11), "sceAgcInit");
    static_assert(sizeof(DrawCommand) == 64);
    static_assert(Calculator3DShaders::InterfaceVertex.PushConstantSlot < 0);
    static_assert(Calculator3DShaders::InterfaceFragment.PushConstantSlot < 0);
    static_assert(Calculator3DShaders::InterfaceVertex.DescriptorSlot >= 0);

    for (auto& slot : slots) {
        commands = static_cast<std::uint32_t*>(memory.Allocate(CommandWords * sizeof(std::uint32_t), 256));
        fence = static_cast<std::uint64_t*>(memory.Allocate(sizeof(std::uint64_t), 64));
        std::construct_at(fence, 0);
        frames = static_cast<DrawCommand*>(memory.Allocate(sizeof(DrawCommand) * Canvas::Capacity, alignof(DrawCommand)));

        for (std::size_t index = 0; index < Canvas::Capacity; ++index) std::construct_at(frames + index);

        descriptor = static_cast<std::uint32_t*>(memory.Allocate(16, 16));
        const auto frameAddress = reinterpret_cast<std::uintptr_t>(frames);
        descriptor[0] = static_cast<std::uint32_t>(frameAddress);
        descriptor[1] = static_cast<std::uint32_t>(frameAddress >> 32);
        descriptor[2] = static_cast<std::uint32_t>(sizeof(DrawCommand) * Canvas::Capacity);
        descriptor[3] = 0x31016fac;
        contexts = static_cast<ShaderRegister*>(memory.Allocate(Pipeline::ContextCapacity * sizeof(ShaderRegister), 256));
        const auto bytes = reinterpret_cast<std::byte*>(contexts + Pipeline::ContextCapacity) - reinterpret_cast<std::byte*>(commands);
        slot = {commands, fence, frames, descriptor, contexts, static_cast<std::size_t>(bytes), false};
    }

    vertex = createShader(Calculator3DShaders::InterfaceVertex.Header, Calculator3DShaders::InterfaceVertex.Code);
    fragment = createShader(Calculator3DShaders::InterfaceFragment.Header, Calculator3DShaders::InterfaceFragment.Code);
    pipeline = std::make_unique<Pipeline>(memory, width, height, vertex, fragment);
}

Renderer::~Renderer() noexcept(false) { for (auto& slot : slots) waitSlot(slot); }
void Renderer::waitSlot(DrawSlot& slot) {
    if (!slot.Pending) return;
    const auto start = sceKernelGetProcessTime();
    std::atomic_ref completion(*slot.Fence);

    while (completion.load(std::memory_order_acquire) != 1) {
        if (sceKernelGetProcessTime() - start > 5000000) throw Calculator3DError{"GPU rendering timed out"};
        Check(sceKernelUsleep_nid_postfix(100), "sceKernelUsleep");
    }

    slot.Pending = false;
}

void Renderer::selectSlot(std::size_t index) {
    auto& slot = slots.at(index);
    waitSlot(slot);
    commands = slot.Commands;
    fence = slot.Fence;
    frames = slot.Frames;
    descriptor = slot.Descriptor;
    contexts = slot.Contexts;
}

Shader* Renderer::createShader(std::span<const std::uint8_t> header, std::span<const std::uint8_t> code) {
    void* headerMemory = memory.Allocate(header.size(), 8);
    void* codeMemory = memory.Allocate(code.size(), 256);
    std::memcpy(headerMemory, header.data(), header.size());
    std::memcpy(codeMemory, code.data(), code.size());
    Shader* shader = nullptr;
    Check(sceAgcCreateShader(&shader, headerMemory, codeMemory), "sceAgcCreateShader");
    if (!shader || !shader->cx_registers || !shader->sh_registers) throw Calculator3DError{"Incomplete shader"};
    return shader;
}

void Renderer::Draw(Display& display, std::span<const DrawCommand> drawings) {
    if (drawings.empty() || drawings.size() > Canvas::Capacity) throw Calculator3DError{"Invalid draw command count"};
    selectSlot(slotIndex);
    std::copy(drawings.begin(), drawings.end(), frames);

    std::atomic_ref completion(*fence);
    completion.store(0, std::memory_order_release);
    CommandBuffer buffer{commands, commands + CommandWords, commands, commands + CommandWords, exhausted, nullptr, 0};
    sceAgcDcbContextStateOp(&buffer, 0);
    sceAgcDcbAcquireMem(&buffer, 1, 0, 0x4381, shaderMemoryPending ? memory.Data() : commands, shaderMemoryPending ? memory.Size() : slots[slotIndex].Bytes, 40);
    pipeline->Bind(buffer, display.Acquire().data(), {contexts, Pipeline::ContextCapacity});

    for (const auto* shader : {vertex, fragment}) sceAgcDcbSetShRegistersIndirect(&buffer, shader->sh_registers, shader->num_sh_registers);
    sceAgcDcbSetNumInstances(&buffer, 1);
    bindShader(buffer, 0x8c, Calculator3DShaders::InterfaceVertex.UserData, Calculator3DShaders::InterfaceVertex.UserDataCount, Calculator3DShaders::InterfaceVertex.DescriptorSlot, descriptor);
    bindShader(buffer, 0xc, Calculator3DShaders::InterfaceFragment.UserData, Calculator3DShaders::InterfaceFragment.UserDataCount, -1, nullptr);
    sceAgcDcbDrawIndexAuto(&buffer, static_cast<std::uint32_t>(drawings.size() * 6), 0);
    const auto flip = display.PrepareFlip();
    sceAgcDcbSetFlip(&buffer, flip.Handle, flip.BufferIndex, 1, flip.Argument);
    sceAgcCbReleaseMem(&buffer, 0x2d, 0x70f, 0, 0, reinterpret_cast<Label*>(fence), 2, 1, 0, 0, 3, 0);
    const Packet packet{commands, static_cast<std::uint32_t>(buffer.cursor_up - commands), 0, {}};
    std::atomic_thread_fence(std::memory_order_seq_cst);
    Check(sceAgcDriverSubmitDcb(&packet), "sceAgcDriverSubmitDcb");
    slots[slotIndex].Pending = true;
    shaderMemoryPending = false;
    slotIndex = (slotIndex + 1) % slots.size();
    display.CompleteFlip();
}

