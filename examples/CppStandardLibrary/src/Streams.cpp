#include "Checks.hpp"
#include <version>
#if !defined(_LIBCPP_VERSION) || _LIBCPP_HAS_LOCALIZATION
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <iomanip>
#include <string>
#endif

void CheckStreams() {
#if defined(_LIBCPP_VERSION) && !_LIBCPP_HAS_LOCALIZATION
    throw std::runtime_error("libc++ localization is disabled; required component unavailable");
#else
    std::cout << "libc++ standard output" << std::endl;
    std::wcout << L"libc++ wide output" << std::endl;
    Require(std::cout.good() && std::wcout.good(), "Standard output streams");
    std::stringstream stream;
    stream << std::hex << 255 << ' ' << std::quoted("two words");
    int value = 0; std::string text; stream >> std::hex >> value >> std::quoted(text);
    Require(value == 255 && text == "two words", "Stream formatting and parsing");
    std::wstringstream wide;
    wide << L"value " << 42;
    std::wstring label; int wideNumber = 0; wide >> label >> wideNumber;
    Require(label == L"value" && wideNumber == 42, "Wide stream conversion");
    stream.clear(); stream.str("invalid"); stream.exceptions(std::ios::failbit);
    ExpectException<std::ios_base::failure>([&] { stream >> value; });
#endif
}

void CheckFileStreams(const char* path) {
#if defined(_LIBCPP_VERSION) && !_LIBCPP_HAS_LOCALIZATION
    static_cast<void>(path);
    throw std::runtime_error("libc++ localization is disabled; required component unavailable");
#else
    if (path == nullptr) throw std::invalid_argument("File stream check requires --report <path>");
    const auto fileName = std::string(path) + ".fstream";
    {
        std::ifstream existing(fileName);
        Require(!existing.is_open(), "File stream test path already exists");
    }
    {
        std::ofstream output(fileName, std::ios::binary);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        output << "value " << 123 << '\n';
        output.close();
    }
    {
        std::ifstream input(fileName, std::ios::binary);
        input.exceptions(std::ios::badbit | std::ios::failbit);
        std::string label; int value = 0; input >> label >> value;
        Require(label == "value" && value == 123, "File stream roundtrip");
        input.seekg(0); Require(input.peek() == 'v', "File stream positioning");
        input.close();
    }
    Require(std::remove(fileName.c_str()) == 0, "File stream cleanup");
#endif
}
