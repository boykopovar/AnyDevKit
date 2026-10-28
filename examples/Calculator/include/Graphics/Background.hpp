#ifndef ANYDEVKIT_CALCULATOR_GRAPHICS_BACKGROUND_HPP
#define ANYDEVKIT_CALCULATOR_GRAPHICS_BACKGROUND_HPP

#include "Graphics/ShaderAsset.hpp"
#include "SceShaders.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

class Background final {
public:
    Background(int width, int height);
    ~Background() noexcept(false);
    Background(const Background&) = delete;
    Background& operator=(const Background&) = delete;
    std::span<const std::uint32_t> Draw(float seconds);

private:
    void* allocate(std::size_t bytes, std::size_t alignment);
    Shader* createShader(const ShaderAsset& asset);
    void createTarget();
    int width;
    int height;
    std::byte* memory = nullptr;
    std::int64_t physicalAddress = 0;
    std::size_t used = 0;
    std::span<std::uint32_t> pixels;
    std::uint32_t* commands = nullptr;
    std::uint64_t* fence = nullptr;
    float* constants = nullptr;
    Shader* vertex = nullptr;
    Shader* fragment = nullptr;
    ShaderRegister* context = nullptr;
    std::uint32_t contextCount = 0;
    ShaderRegister* primitive = nullptr;
    bool pending = false;
};

#endif
