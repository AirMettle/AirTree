// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/query/grid/GridQuery.hpp>
#include <benchmark/benchmark.h>
#include <cmath>
#include <limits>
#include <vector>

namespace {

using airtree::bench::query::QueryFixtureBase;
using airtree::query::grid::GridAxisSpec;
using airtree::query::grid::GridQuery;
using airtree::query::grid::GridScaling;

std::vector<GridAxisSpec> makeFullExtentAxes(int dims, uint32_t steps) {
  const double neg_inf = -std::numeric_limits<double>::infinity();
  const double pos_inf = std::numeric_limits<double>::infinity();
  std::vector<GridAxisSpec> axes;
  axes.reserve(static_cast<std::size_t>(dims));
  for (int i = 0; i < dims; ++i) {
    axes.push_back(GridAxisSpec{neg_inf, pos_inf, steps, GridScaling::Linear});
  }
  return axes;
}


#define DEFINE_GRID_SUITE(SchemaClass, Dims)                                   \
  class SchemaClass : public QueryFixtureBase {};                              \
                                                                               \
  BENCHMARK_DEFINE_F(SchemaClass, Grid_steps8)                                 \
  (benchmark::State & state) {                                                 \
    if (!hasBuffer()) {                                                        \
      state.SkipWithError("empty .airtree buffer");                            \
      return;                                                                  \
    }                                                                          \
    GridQuery q(buffer_);                                                      \
    auto axes = makeFullExtentAxes(Dims, 8);                                   \
    std::size_t last_result_bytes = 0;                                         \
    for (auto _ : state) {                                                     \
      auto r = q.getGrid(axes);                                                \
      last_result_bytes =                                                      \
          r.counts.size() *                                                    \
          sizeof(typename decltype(r.counts)::value_type);                     \
      benchmark::DoNotOptimize(r.counts.data());                               \
      benchmark::ClobberMemory();                                              \
    }                                                                          \
    applyCommonCounters(state);                                                \
    applyQueryId(state, QueryId::Grid_steps8);                                 \
    applyResultSize(state, last_result_bytes);                                 \
    state.counters["Param_steps"] = 8;                                         \
    state.counters["Dims"] = Dims;                                             \
  }                                                                            \
  BENCHMARK_REGISTER_F(SchemaClass, Grid_steps8);


DEFINE_GRID_SUITE(AirTreeQuery2DxP_Grid, 2)
DEFINE_GRID_SUITE(AirTreeQuery3DxP_Grid, 3)
DEFINE_GRID_SUITE(AirTreeQuery4DxP_Grid, 4)

#undef DEFINE_GRID_SUITE

} // namespace
