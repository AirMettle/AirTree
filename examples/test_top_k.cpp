#include <airtree/core/api/AirTreeGenerator.hpp>
#include <airtree/query/topk/TopK.hpp>

#include <iostream>
#include <vector>
#include <iomanip>

int main() {
    std::cout << "=== AirTree Top-K Query Test ===\n\n";

    // ===================================================================
    // 1. Create test data (1D)
    // ===================================================================
    std::vector<double> data = {
        10.5, 12.0, 11.8, 13.2, 9.5, 14.1, 12.7,
        25.0, 27.5, 26.3, 24.8,               // high values (tail)
        8.0, 7.5, 6.8, 9.2,
        30.1, 29.5, 28.8,                     // more high outliers
        15.0, 16.2, 14.8
    };

    std::cout << "Data size: " << data.size() << " values\n";
    std::cout << "Range: [" << *std::min_element(data.begin(), data.end())
              << ", " << *std::max_element(data.begin(), data.end()) << "]\n\n";

    // ===================================================================
    // 2. Generate 1D histogram (using default XP configuration)
    // ===================================================================
    airtree::core::api::AirTreeOptions opts;
    opts.dimensions = 1;
    opts.type = airtree::core::api::ConfigType::XP;

    std::vector<char> histogram = airtree::core::api::generate(data, opts);

    std::cout << "Histogram generated: " << histogram.size() << " bytes\n\n";

    // ===================================================================
    // 3. Test Top-K queries
    // ===================================================================
    airtree::query::topk::TopK tk(histogram);

    std::vector<double> k_values = {5.0, 10.0, 25.0};

    for (double k : k_values) {
        std::cout << "=== Top " << k << "% by mass ===\n";
        
        auto results = tk.getTopK(k);

        if (results.empty()) {
            std::cout << "  No results returned.\n";
        } else {
            std::cout << std::left
                      << std::setw(15) << "Lower"
                      << std::setw(15) << "Upper"
                      << std::setw(12) << "Count"
                      << "Percentage\n";
            std::cout << std::string(55, '-') << "\n";

            for (const auto& r : results) {
                std::cout << std::left
                          << std::setw(15) << r.getLowerBound()
                          << std::setw(15) << r.getUpperBound()
                          << std::setw(12) << r.getCount()
                          << std::fixed << std::setprecision(2)
                          << (static_cast<double>(r.getCount()) * 100.0 / data.size()) << "%\n";
            }
        }
        std::cout << "\n";
    }

    std::cout << "Test completed successfully.\n";
    return 0;
}
