#ifndef ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_PIPELINE_HPP
#define ANYDEVKIT_CPP_STANDARD_LIBRARY_GRAPHICS_PIPELINE_HPP
#include "Graphics/Memory.hpp"
#include "SceShaders.hpp"
#include <span>

struct CommandBuffer;
class Pipeline final {
public:
    static constexpr std::size_t ContextCapacity = 256;
    Pipeline(Memory& memory, int width, int height, Shader* vertex, Shader* fragment);
    void Bind(CommandBuffer& buffer, void* target, std::span<ShaderRegister> storage);

private:
    ShaderRegister* context;
    ShaderRegister* primitive;
    std::uint32_t contextCount = 0;
};
#endif

