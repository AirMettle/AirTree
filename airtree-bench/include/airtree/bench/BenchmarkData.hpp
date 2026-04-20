#ifndef AIRTREE_INCLUDE_BENCH_BENCHMARKDATA_HPP
#define AIRTREE_INCLUDE_BENCH_BENCHMARKDATA_HPP

#include <vector>
#include <airtree/core/AirTreeCore_internal.hpp>

namespace airtree::bench {

template <typename T> class BenchmarkData {
public:
  inline static std::vector<std::vector<T>> raw_data;
  inline static std::vector<FPHArray> fpharrays;
};

} // namespace airtree::bench

#endif // AIRTREE_INCLUDE_BENCH_BENCHMARKDATA_HPP
