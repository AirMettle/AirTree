// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/query/bounding-box/BoundingBox.hpp>
#include <airtree/query/minmax/MinMax.hpp>
#include <benchmark/benchmark.h>
#include <cmath>

namespace {

using airtree::bench::query::QueryFixtureBase;
using airtree::query::bounding_box::BoundingBox;
using airtree::query::bounding_box::BoundingBoxCoordinate2D;
using airtree::query::bounding_box::BoundingBoxCoordinate3D;

// Mid-50% box on each 1D marginal via MinMax is not available multi-D.
// Use a fixed relative box that exercises getCounts; values are wide enough
// to cover typical float data while remaining valid finite bounds.
BoundingBoxCoordinate2D mid50Box2D() {
  // Mid-50% of a nominal [0, 100] span per axis → [25, 75]
  return BoundingBoxCoordinate2D(25.0, 75.0, 25.0, 75.0);
}

BoundingBoxCoordinate3D mid50Box3D() {
  return BoundingBoxCoordinate3D(25.0, 75.0, 25.0, 75.0, 25.0, 75.0);
}

class AirTreeQuery2DxP_BoundingBox : public QueryFixtureBase {};

BENCHMARK_DEFINE_F(AirTreeQuery2DxP_BoundingBox, BoundingBox_mid50)
(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  BoundingBox q(buffer_);
  const auto box = mid50Box2D();
  
  std::size_t last_result_bytes = 0;
 
  for (auto _ : state) {
    auto r = q.getCounts(box);
    
    last_result_bytes = sizeof(decltype(r));
    
    benchmark::DoNotOptimize(r);
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  //===== start new code ====//
  applyQueryId(state, QueryId::BoundingBox_mid50);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Dims"] = 2;
}
BENCHMARK_REGISTER_F(AirTreeQuery2DxP_BoundingBox, BoundingBox_mid50);

class AirTreeQuery3DxP_BoundingBox : public QueryFixtureBase {};

BENCHMARK_DEFINE_F(AirTreeQuery3DxP_BoundingBox, BoundingBox_mid50)
(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  BoundingBox q(buffer_);
  const auto box = mid50Box3D();
  
  std::size_t last_result_bytes = 0;
  
  for (auto _ : state) {
    auto r = q.getCounts(box);
    
    last_result_bytes = sizeof(decltype(r));
   
    benchmark::DoNotOptimize(r);
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  
  applyQueryId(state, QueryId::BoundingBox_mid50);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Dims"] = 3;
}
BENCHMARK_REGISTER_F(AirTreeQuery3DxP_BoundingBox, BoundingBox_mid50);

} // namespace
