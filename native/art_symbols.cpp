#include "art_symbols.hpp"
#include "elf_image.hpp"
#include "debug_data.hpp"
#include <fcntl.h>
#include <link.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace soraxhook {
namespace {
struct LoadedArt { std::string path; uintptr_t base = 0; };
int FindArt(dl_phdr_info* info, size_t, void* data) {
    std::string_view name(info->dlpi_name ? info->dlpi_name : "");
    if (!name.ends_with("/libart.so")) return 0;
    auto& result = *static_cast<LoadedArt*>(data);
    result.path = name;
    result.base = info->dlpi_addr;
    return 1;
}
}
ArtSymbols::ArtSymbols() {
    LoadedArt loaded;
    dl_iterate_phdr(FindArt, &loaded);
    if (loaded.path.empty()) return;
    const int fd = open(loaded.path.c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) return;
    struct stat st{};
    if (fstat(fd, &st) == 0 && st.st_size >= static_cast<off_t>(sizeof(elf::Header))) {
        size_ = static_cast<size_t>(st.st_size);
        void* p = mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd, 0);
        if (p != MAP_FAILED) { mapping_ = p; base_ = loaded.base; }
    }
    close(fd);
    if (!mapping_) return;
    const elf::Image image({static_cast<const unsigned char*>(mapping_), size_});
    if (!image.valid()) {
        munmap(mapping_, size_);
        mapping_ = nullptr;
        return;
    }
    debug_data_ = DecodeDebugData(image.FindSection(".gnu_debugdata"));
    if (!debug_data_.empty() && !elf::Image(debug_data_).valid()) debug_data_.clear();
}
ArtSymbols::~ArtSymbols() { if (mapping_) munmap(mapping_, size_); }
void* ArtSymbols::Find(std::string_view name, bool prefix) const {
    if (!mapping_) return nullptr;
    auto symbol = elf::Image({static_cast<const unsigned char*>(mapping_), size_}).Find(name, prefix);
    if (!symbol && !debug_data_.empty()) symbol = elf::Image(debug_data_).Find(name, prefix);
    if (!symbol) return nullptr;
    return reinterpret_cast<void*>((symbol->absolute ? 0 : base_) + symbol->address);
}
}
