/**
 * main_test.cpp
 * Simple test for AirTree Percentile API.
 *
 * Build:
 *   g++ -std=c++17 main_test.cpp \
 *       -I$AIRTREE/include \
 *       -I$DEPS/spdlog_ep/<hash>/include \
 *       -L$AIRTREE/lib \
 *       -lairtree-query -lairtree-core -lairtree-util \
 *       -lspdlog -lssl -lcrypto -lpthread -ldl -lrt -lm \
 *       -o main_test
 */

#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/query/percentile/Percentile.hpp>

// ── Helpers ───────────────────────────────────────────────────────────────────

// Simple pass/fail counters
static int g_passed = 0;
static int g_failed = 0;

void report(const std::string& name, bool ok, const std::string& detail = "") {
    if (ok) {
        std::cout << "  [PASS] " << name << "\n";
        ++g_passed;
    } else {
        std::cout << "  [FAIL] " << name;
        if (!detail.empty()) std::cout << " — " << detail;
        std::cout << "\n";
        ++g_failed;
    }
}

// Generate N floats from a simple deterministic distribution centred at 'mean'
std::vector<float> make_data(int n, float mean = 50.0f, float spread = 20.0f) {
    std::vector<float> v;
    v.reserve(n);
    // evenly spaced from (mean - spread) to (mean + spread)
    for (int i = 0; i < n; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(n - 1); // 0 to 1
        v.push_back((mean - spread) + t * 2.0f * spread);
    }
    return v;
}

// ── Tests ─────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "=== Percentile Tests ===\n\n";

    namespace api = airtree::core::api;

    // Build a 1D histogram from 1000 evenly-spaced floats in [30, 70]
    auto data   = make_data(1000, 50.0f, 20.0f);  // range: 30 to 70
    auto buf_xp = api::generate(data);

    api::AirTreeOptions opts_xf;
    opts_xf.type = api::ConfigType::XF;
    auto buf_xf = api::generate(data, opts_xf);

    api::AirTreeOptions opts_xt;
    opts_xt.type = api::ConfigType::XT;
    auto buf_xt = api::generate(data, opts_xt);

    // ── Test 1: p50 should be near the middle of [30, 70] = ~50 ──────────────
    {
        airtree::query::percentile::Percentile p(buf_xp);
        double v = p.getPercentile(50.0);
        bool ok = (v > 45.0 && v < 55.0);
        report("p50 XP is near 50", ok,
               "got " + std::to_string(v));
    }

    // ── Test 2: p50 with XF config ────────────────────────────────────────────
    {
        airtree::query::percentile::Percentile p(buf_xf);
        double v = p.getPercentile(50.0);
        bool ok = (v > 45.0 && v < 55.0);
        report("p50 XF is near 50", ok,
               "got " + std::to_string(v));
    }

    // ── Test 3: p50 with XT config ────────────────────────────────────────────
    {
        airtree::query::percentile::Percentile p(buf_xt);
        double v = p.getPercentile(50.0);
        bool ok = (v > 45.0 && v < 55.0);
        report("p50 XT is near 50", ok,
               "got " + std::to_string(v));
    }

    // ── Test 4: 1 <= p50 <= p99 (monotone) ─────────────────────────────────
    {
        airtree::query::percentile::Percentile p(buf_xp);
        double p1   = p.getPercentile(1.0);
        double p50  = p.getPercentile(50.0);
        double p99 = p.getPercentile(99.0);
        bool ok = (p1 <= p50 && p50 <= p99);
        report("p1 <= p50 <= p99 (monotone)", ok,
               "p1=" + std::to_string(p1) +
               " p50=" + std::to_string(p50) +
               " p99=" + std::to_string(p99));
    }

    // ── Test 5: p25 < p75 ────────────────────────────────────────────────────
    {
        airtree::query::percentile::Percentile p(buf_xp);
        double p25 = p.getPercentile(25.0);
        double p75 = p.getPercentile(75.0);
        bool ok = (p25 < p75);
        report("p25 < p75", ok,
               "p25=" + std::to_string(p25) +
               " p75=" + std::to_string(p75));
    }

    // ── Test 6: 1 is near the minimum of our data (~30) ─────────────────────
    {
        airtree::query::percentile::Percentile p(buf_xp);
        double v = p.getPercentile(1.0);
        bool ok = (v >= 28.0 && v <= 35.0);
        report("p1 is near data minimum (~30)", ok,
               "got " + std::to_string(v));
    }

    // ── Test 7: p99 is near the maximum of our data (~70) ───────────────────
    {
        airtree::query::percentile::Percentile p(buf_xp);
        double v = p.getPercentile(99.0);
        bool ok = (v >= 65.0 && v <= 72.0);
        report("p99 is near data maximum (~70)", ok,
               "got " + std::to_string(v));
    }

    // ── Test 8: throws on a 2D buffer ────────────────────────────────────────
    {
        auto d1   = make_data(500, 50.0f, 20.0f);
        auto d2   = make_data(500, 50.0f, 20.0f);
        auto buf2d = api::generate(d1, d2);
        bool threw = false;
        try {
            airtree::query::percentile::Percentile p(buf2d);
            (void)p.getPercentile(50.0);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        report("throws on 2D buffer", threw);
    }

    // ── Summary ───────────────────────────────────────────────────────────────
    std::cout << "\n--- Results: "
              << g_passed << " / " << (g_passed + g_failed)
              << " passed ---\n";

    return g_failed == 0 ? 0 : 1;
}
