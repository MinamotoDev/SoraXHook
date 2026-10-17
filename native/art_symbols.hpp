#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace soraxhook {
// Resolve both exported and internal symbols from the loaded ART's ELF file.
class ArtSymbols {
public:
    ArtSymbols();
    ~ArtSymbols();
    ArtSymbols(const ArtSymbols&) = delete;
    ArtSymbols& operator=(const ArtSymbols&) = delete;
    bool valid() const { return mapping_ != nullptr; }
    bool has_debug_data() const { return !debug_data_.empty(); }
    void* Find(std::string_view name, bool prefix = false) const;
private:
    void* mapping_ = nullptr;
    size_t size_ = 0;
    uintptr_t base_ = 0;
    std::vector<unsigned char> debug_data_;
};
}
