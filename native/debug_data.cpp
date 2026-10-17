#include "debug_data.hpp"
#include <array>
#include <memory>
extern "C" {
#include <xz.h>
}
namespace soraxhook {
std::vector<unsigned char> DecodeDebugData(std::span<const unsigned char> input) {
    if (input.empty()) return {};
    static const bool initialized = [] { xz_crc32_init(); xz_crc64_init(); return true; }();
    (void)initialized;
    constexpr size_t limit = 64 * 1024 * 1024;
    std::unique_ptr<xz_dec, decltype(&xz_dec_end)> decoder(xz_dec_init(XZ_DYNALLOC, limit), xz_dec_end);
    if (!decoder) return {};
    std::array<unsigned char, 65536> chunk{};
    xz_buf buffer{};
    buffer.in = input.data();
    buffer.in_size = input.size();
    buffer.out = chunk.data();
    buffer.out_size = chunk.size();
    std::vector<unsigned char> output;
    while (true) {
        buffer.out_pos = 0;
        const size_t previous = buffer.in_pos;
        const auto result = xz_dec_run(decoder.get(), &buffer);
        if (buffer.out_pos > limit - output.size()) return {};
        output.insert(output.end(), chunk.begin(), chunk.begin() + buffer.out_pos);
        if (result == XZ_STREAM_END) return output;
        if (result != XZ_OK || (previous == buffer.in_pos && !buffer.out_pos)) return {};
    }
}
}
