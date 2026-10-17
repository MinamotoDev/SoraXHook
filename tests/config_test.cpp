#include "config.hpp"
#include <cassert>
#include <iostream>
using namespace soraxhook;
int main() {
    assert(ParseConfig("").country == "us");
    assert(ParseConfig("# comment\n").valid);
    assert(ParseConfig("network_country_iso: jp\n").country == "jp");
    assert(ParseConfig("network_country_iso: US # test\r\n").country == "us");
    assert(ParseConfig("\xef\xbb\xbf---\nnetwork_country_iso: \"GB\" # hi\n").country == "gb");
    assert(ParseConfig("network_country_iso: 'de'\n").country == "de");
    for (auto text : {"network_country_iso: USA", "network_country_iso:",
          "network_country_iso: u1", "network_country_iso: us\nnetwork_country_iso: jp",
          "other: jp", "network_country_iso: us#bad", "network_country_iso: [us]",
          "network_country_iso: \"jp\" trailing", "network_country_iso: 'jp"}) {
        const auto result = ParseConfig(text);
        assert(!result.valid && result.country == "us");
    }
    assert(!ParseConfig(std::string(4097, 'x')).valid);
    assert(!ParseConfig(std::string("a\0b", 3)).valid);
    assert(IsTarget("com.twitter.android", "/data/user/0/com.twitter.android"));
    assert(IsTarget("com.twitter.android:service", "/data/user/10/com.twitter.android"));
    assert(!IsTarget("com.twitter.android.fake", "/data/user/0/com.twitter.android.fake"));
    assert(!IsTarget("com.twitter.android", "/data/user/0/another.app"));
    assert(!IsTarget("system_server", ""));
    std::cout << "Config and process scope tests passed\n";
}
