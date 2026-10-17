#pragma once
#include <string>
#include <string_view>

namespace soraxhook {
inline constexpr char kConfigPath[] = "/sdcard/Documents/MinamotoDev/SoraXHook/config.yaml";
inline constexpr size_t kMaxConfigBytes = 4096;
struct Config {
    std::string country = "us";
    bool valid = true;
};
inline std::string_view Trim(std::string_view s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == s.npos) return {};
    return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
// Deliberately supports only this module's flat YAML schema, not general YAML.
// Reject unsupported syntax instead of silently accepting a partial document.
inline Config ParseConfig(std::string_view input) {
    const auto bad = [] { return Config{"us", false}; };
    if (input.size() > kMaxConfigBytes || input.find('\0') != input.npos) return bad();
    if (input.starts_with("\xef\xbb\xbf")) input.remove_prefix(3);
    Config result;
    bool found = false;
    while (!input.empty()) {
        const auto end = input.find('\n');
        auto line = Trim(input.substr(0, end));
        input = end == input.npos ? std::string_view{} : input.substr(end + 1);
        if (line.empty() || line.starts_with('#')) continue;
        if (line == "---" && !found) continue;
        const auto colon = line.find(':');
        if (colon == line.npos || Trim(line.substr(0, colon)) != "network_country_iso" || found)
            return bad();
        auto value = Trim(line.substr(colon + 1));
        if (value.empty()) return bad();
        if (value.front() == '\'' || value.front() == '"') {
            const auto close = value.find(value.front(), 1);
            if (close == value.npos) return bad();
            const auto tail = Trim(value.substr(close + 1));
            if (!tail.empty() && !tail.starts_with('#')) return bad();
            value = value.substr(1, close - 1);
        } else {
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '#' && i && (value[i - 1] == ' ' || value[i - 1] == '\t')) {
                    value = Trim(value.substr(0, i));
                    break;
                }
            }
        }
        if (value.size() != 2) return bad();
        result.country.clear();
        for (char c : value) {
            if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
            if (c < 'a' || c > 'z') return bad();
            result.country += c;
        }
        found = true;
    }
    return result;
}
inline bool IsTarget(std::string_view process, std::string_view data_dir) {
    constexpr std::string_view package = "com.twitter.android";
    const bool name = process == package || process.starts_with("com.twitter.android:");
    return name && data_dir.ends_with("/com.twitter.android");
}
}
