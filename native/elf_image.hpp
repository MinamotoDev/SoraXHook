#pragma once
#include <elf.h>
#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string_view>

namespace soraxhook::elf {
using Bytes = std::span<const unsigned char>;
#if INTPTR_MAX == INT64_MAX
using Header = Elf64_Ehdr;
using Section = Elf64_Shdr;
using Symbol = Elf64_Sym;
inline constexpr int kClass = ELFCLASS64;
#else
using Header = Elf32_Ehdr;
using Section = Elf32_Shdr;
using Symbol = Elf32_Sym;
inline constexpr int kClass = ELFCLASS32;
#endif
inline bool Range(size_t offset, size_t length, size_t total) {
    return offset <= total && length <= total - offset;
}
template<class T> std::optional<T> Read(Bytes bytes, size_t offset) {
    if (!Range(offset, sizeof(T), bytes.size())) return {};
    T value;
    std::memcpy(&value, bytes.data() + offset, sizeof(value));
    return value;
}
inline std::optional<std::string_view> String(Bytes bytes, size_t offset) {
    if (offset >= bytes.size()) return {};
    const char* start = reinterpret_cast<const char*>(bytes.data() + offset);
    const char* end = static_cast<const char*>(std::memchr(start, 0, bytes.size() - offset));
    if (!end) return {};
    return std::string_view(start, end - start);
}
class Image {
public:
    explicit Image(Bytes bytes) : bytes_(bytes) {
        const auto header = Read<Header>(bytes, 0);
        if (!header || std::memcmp(header->e_ident, ELFMAG, SELFMAG) ||
            header->e_ident[EI_CLASS] != kClass || header->e_ident[EI_DATA] != ELFDATA2LSB ||
            header->e_shentsize != sizeof(Section) || !header->e_shnum ||
            !Range(header->e_shoff, size_t(header->e_shnum) * sizeof(Section), bytes.size())) return;
        header_ = header;
    }
    bool valid() const { return header_.has_value(); }
    std::optional<Section> GetSection(size_t index) const {
        if (!header_ || index >= header_->e_shnum) return {};
        return Read<Section>(bytes_, header_->e_shoff + index * sizeof(Section));
    }
    Bytes SectionBytes(const Section& section) const {
        if (!Range(section.sh_offset, section.sh_size, bytes_.size())) return {};
        return bytes_.subspan(section.sh_offset, section.sh_size);
    }
    Bytes FindSection(std::string_view name) const {
        if (!header_) return {};
        const auto names = GetSection(header_->e_shstrndx);
        if (!names) return {};
        const auto strings = SectionBytes(*names);
        for (size_t i = 0; i < header_->e_shnum; ++i) {
            const auto section = GetSection(i);
            const auto candidate = section ? String(strings, section->sh_name) : std::nullopt;
            if (candidate && *candidate == name) return SectionBytes(*section);
        }
        return {};
    }
    struct Value { uintptr_t address; bool absolute; };
    std::optional<Value> Find(std::string_view name, bool prefix = false) const {
        if (!header_) return {};
        for (size_t i = 0; i < header_->e_shnum; ++i) {
            const auto section = GetSection(i);
            if (!section || (section->sh_type != SHT_SYMTAB && section->sh_type != SHT_DYNSYM) ||
                section->sh_entsize != sizeof(Symbol)) continue;
            const auto symbols = SectionBytes(*section);
            const auto string_section = GetSection(section->sh_link);
            if (!string_section) continue;
            const auto strings = SectionBytes(*string_section);
            for (size_t offset = 0; offset + sizeof(Symbol) <= symbols.size(); offset += sizeof(Symbol)) {
                const auto symbol = Read<Symbol>(symbols, offset);
                if (symbol->st_shndx == SHN_UNDEF || !symbol->st_value) continue;
                const auto candidate = String(strings, symbol->st_name);
                if (candidate && (prefix ? candidate->starts_with(name) : *candidate == name))
                    return Value{static_cast<uintptr_t>(symbol->st_value), symbol->st_shndx == SHN_ABS};
            }
        }
        return {};
    }
private:
    Bytes bytes_;
    std::optional<Header> header_;
};
}
