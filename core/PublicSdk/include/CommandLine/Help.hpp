#ifndef ANYDEVKIT_PUBLICSDK_HELP_HPP
#define ANYDEVKIT_PUBLICSDK_HELP_HPP
#include <string_view>
namespace AnyDevKit::PublicSdk {
class Help final {
public:
    static void Print(std::string_view command);
};
}
#endif
