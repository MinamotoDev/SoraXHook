#pragma once
#include <span>
#include <vector>
namespace soraxhook {
// Decode the mini ELF embedded in .gnu_debugdata. Empty on malformed/oversized input.
std::vector<unsigned char> DecodeDebugData(std::span<const unsigned char> input);
}
