#ifndef ANYDEVKIT_PUBLICSDK_APPLICATION_HPP
#define ANYDEVKIT_PUBLICSDK_APPLICATION_HPP
#include <span>
#include <string>
namespace AnyDevKit::PublicSdk {
class Application final {
public:
    static void Run(std::span<const std::string> arguments);
};
}
#endif
