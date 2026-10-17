#include "elf_image.hpp"
#include "debug_data.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

int main(int argc, char** argv) {
    assert(argc == 2);
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<unsigned char> bytes{std::istreambuf_iterator<char>(file), {}};
    const soraxhook::elf::Image outer(bytes);
    assert(outer.valid());
    constexpr std::string_view symbol = "_ZN3art15GetMethodShortyFixture";
    assert(!outer.Find(symbol));
    const auto compressed = outer.FindSection(".gnu_debugdata");
    assert(!compressed.empty());
    const auto decoded = soraxhook::DecodeDebugData(compressed);
    const soraxhook::elf::Image inner(decoded);
    assert(inner.valid());
    assert(inner.Find(symbol)->address == 0x123456);
    assert(inner.Find("_ZN3art15GetMethodShorty", true)->address == 0x123456);
    assert(!inner.Find("unrelated"));
    assert(!soraxhook::elf::Image({}).valid());
    for (size_t length = 0; length < sizeof(soraxhook::elf::Header); ++length)
        assert(!soraxhook::elf::Image(std::span(bytes).first(length)).valid());
    auto corrupted = std::vector<unsigned char>(compressed.begin(), compressed.end());
    corrupted[0] ^= 0xff;
    assert(soraxhook::DecodeDebugData(corrupted).empty());
    assert(soraxhook::DecodeDebugData(compressed.first(compressed.size() / 2)).empty());
    std::cout << "Compressed ELF symbol lookup tests passed\n";
}
