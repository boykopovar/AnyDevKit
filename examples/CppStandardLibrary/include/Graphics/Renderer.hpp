#ifndef ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_RENDERER_HPP
#define ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_RENDERER_HPP
#include "Graphics/Memory.hpp"
#include "Display.hpp"
#include "Canvas.hpp"
#include "Graphics/Pipeline.hpp"
#include <array>
#include <memory>
#include <span>

class Renderer final {
public:
    Renderer(int width, int height);
    ~Renderer() noexcept(false);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void Draw(Display& display, std::span<const DrawCommand> drawings);

private:
    struct DrawSlot {
        std::uint32_t* Commands = nullptr;
        std::uint64_t* Fence = nullptr;
        DrawCommand* Frames = nullptr;
        std::uint32_t* Descriptor = nullptr;
        ShaderRegister* Contexts = nullptr;
        std::size_t Bytes = 0;
        bool Pending = false;
    };
    std::array<DrawSlot, 3> slots{};
    std::size_t slotIndex = 0;
    bool shaderMemoryPending = true;
    void selectSlot(std::size_t index);
    void waitSlot(DrawSlot& slot);
    Shader* createShader(std::span<const std::uint8_t> header, std::span<const std::uint8_t> code);
    Memory memory;
    std::unique_ptr<Pipeline> pipeline;
    Shader* vertex;
    Shader* fragment;
    std::uint32_t* commands;
    std::uint64_t* fence;
    DrawCommand* frames;
    std::uint32_t* descriptor;
    ShaderRegister* contexts;
};
#endif

