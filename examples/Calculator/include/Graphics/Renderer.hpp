#ifndef ANYDEVKIT_CALCULATOR_GRAPHICS_RENDERER_HPP
#define ANYDEVKIT_CALCULATOR_GRAPHICS_RENDERER_HPP

#include "Graphics/ShaderAsset.hpp"
#include "Physics/Scene.hpp"
#include "Canvas.hpp"
#include "SceShaders.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

class Renderer final {
public:
    Renderer(int width, int height);
    ~Renderer() noexcept(false);
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void Draw(std::span<std::uint32_t> pixels, float seconds, const Canvas& canvas);

private:
    void* allocate(std::size_t bytes, std::size_t alignment);
    Shader* createShader(const ShaderAsset& asset);
    void createPipeline(ShaderRegister* context, std::uint32_t& contextCount, ShaderRegister* primitive, Shader* vertex, Shader* fragment);
    int width;
    int height;
    std::byte* memory = nullptr;
    std::int64_t physicalAddress = 0;
    std::size_t used = 0;
    std::uint32_t* commands = nullptr;
    std::uint64_t* fence = nullptr;
    float* constants = nullptr;
    Shader* vertex = nullptr;
    Shader* fragment = nullptr;
    ShaderRegister* context = nullptr;
    std::uint32_t contextCount = 0;
    ShaderRegister* primitive = nullptr;
    Shader* interfaceVertex = nullptr;
    Shader* interfaceFragment = nullptr;
    ShaderRegister* interfaceContext = nullptr;
    std::uint32_t interfaceContextCount = 0;
    ShaderRegister* interfacePrimitive = nullptr;
    DrawCommand* interfaceCommands = nullptr;
    bool pending = false;
    BackgroundPhysics::Scene physics;
};

#endif
