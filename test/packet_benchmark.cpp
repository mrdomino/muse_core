// Copyright 2015 Steven Dee.

// Packet parser benchmarks.
// This is kinda hack-and-slash just to get some useful numbers right away.

#include <cassert>
#include <cmath>

#include <memory>
#include <string>
#include <vector>

#include <hammer/hammer.h>
#include <hammer/glue.h>
#include <muse_core/muse_core.h>

#include "packet_builders.h"

using std::unique_ptr;
using std::vector;


#define PAC_MIN IX_PAC_SYNC
#define PAC_MAX IX_PAC_DRLREF
#define PAC_N (PAC_MAX - PAC_MIN + 1)

extern HParser *g_ix_packet;

// TODO(soon): separate benchmark main from individual benchmarks
int main() {
    srand(0);   // We're going for arbitrary, not random, here.
    auto rand_eeg_packet = [] {
        return eeg_packet(rand() % 1024, rand() % 1024, rand() % 1024,
                          rand() % 1024);
    };
    auto rand_dropped_eeg_packet = [] {
        return eeg_packet(rand(), rand() % 1024, rand() % 1024, rand() % 1024,
                          rand() % 1024);
    };
    auto rand_acc_packet = [] {
        return acc_packet(rand() % 1024, rand() % 1024, rand() % 1024);
    };
    auto rand_dropped_acc_packet = [] {
        return acc_packet(rand(), rand() % 1024, rand() % 1024, rand() % 1024);
    };
    auto rand_drlref_packet = [] {
        return drlref_packet(rand() % 1024, rand() % 1024);
    };
    auto rand_battery_packet = [] {
        return battery_packet(rand(), rand(), rand(), rand() - RAND_MAX / 2);
    };
    auto rand_error_packet = [] {
        return error_packet(rand());
    };

    auto all_types = std::vector<parse_input>();
    auto names = std::vector<std::string>();
    for (auto i = 0u; i < PAC_N; ++i) {
        auto input = parse_input();
        auto name = std::string();
        switch (static_cast<ix_pac_type>(i) + PAC_MIN) {
        case IX_PAC_SYNC: name = "SYNC"; input = sync_packet(); break;
        case IX_PAC_ERROR: name = "ERROR"; input = rand_error_packet(); break;
        case IX_PAC_EEG: name = "EEG4"; input = rand_eeg_packet(); break;
        case IX_PAC_BATTERY: name = "BATTERY"; input = rand_battery_packet(); break;
        case IX_PAC_ACCELEROMETER: name = "ACC"; input = rand_acc_packet(); break;
        case IX_PAC_DRLREF: name = "DRL_REF"; input = rand_drlref_packet(); break;
        default: assert(false);
        }
        all_types.push_back(input);
        names.push_back(name);
    }

    auto extras = std::vector<parse_input>{rand_dropped_eeg_packet(), rand_dropped_acc_packet()};
    names.push_back("EEG4_D");
    names.push_back("ACC_D");

    for (auto i = 0u; i < names.size(); ++i) {
        printf("%d: %s\n", i, names[i].c_str());
    }

    auto inputs = all_types;
    inputs.insert(inputs.end(), extras.cbegin(), extras.cend());

    auto tests = std::vector<HParserTestcase>();
    tests.reserve(inputs.size() + 1);
    for (auto i = 0u; i < inputs.size(); ++i) {
        tests.push_back({(unsigned char*)inputs[i].data(), inputs[i].size(), const_cast<char*>("<u>")});
    }
    tests.push_back({ NULL, 0, NULL });
    auto results = h_benchmark(g_ix_packet, tests.data());
    h_benchmark_report(stdout, results);
    return 0;
}
