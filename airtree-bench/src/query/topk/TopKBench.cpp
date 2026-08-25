// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/bench/query/QueryFixtureBase.hpp>
#include <airtree/query/topk/TopK.hpp>
#include <benchmark/benchmark.h>

namespace {

using airtree::bench::query::QueryFixtureBase;

// ---- 1DxT ----
class AirTreeQuery1DxT_TopK : public QueryFixtureBase {};

BENCHMARK_DEFINE_F(AirTreeQuery1DxT_TopK, TopK_k1)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  
  std::size_t last_result_bytes = 0;
  
  for (auto _ : state) {
    auto r = q.getTopK(1.0);
    
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  
  applyQueryId(state, QueryId::TopK_k1);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Param_k"] = 1;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxT_TopK, TopK_k1);

BENCHMARK_DEFINE_F(AirTreeQuery1DxT_TopK, TopK_k5)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  
  std::size_t last_result_bytes = 0;
  
  for (auto _ : state) {
    auto r = q.getTopK(5.0);
    
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  
  applyQueryId(state, QueryId::TopK_k5);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Param_k"] = 5;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxT_TopK, TopK_k5);

BENCHMARK_DEFINE_F(AirTreeQuery1DxT_TopK, TopK_k15)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  std::size_t last_result_bytes = 0;
  for (auto _ : state) {
    auto r = q.getTopK(15.0);
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  applyQueryId(state, QueryId::TopK_k15);
  applyResultSize(state, last_result_bytes);
  state.counters["Param_k"] = 15;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxT_TopK, TopK_k15);

// ---- 1DxF ----
class AirTreeQuery1DxF_TopK : public QueryFixtureBase {};

BENCHMARK_DEFINE_F(AirTreeQuery1DxF_TopK, TopK_k1)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  
  std::size_t last_result_bytes = 0;
  
  for (auto _ : state) {
    auto r = q.getTopK(1.0);
    
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  
  applyQueryId(state, QueryId::TopK_k1);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Param_k"] = 1;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxF_TopK, TopK_k1);

BENCHMARK_DEFINE_F(AirTreeQuery1DxF_TopK, TopK_k5)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  
  std::size_t last_result_bytes = 0;
  
  for (auto _ : state) {
    auto r = q.getTopK(5.0);
    
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  
  applyQueryId(state, QueryId::TopK_k5);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Param_k"] = 5;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxF_TopK, TopK_k5);

BENCHMARK_DEFINE_F(AirTreeQuery1DxF_TopK, TopK_k15)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  std::size_t last_result_bytes = 0;
  for (auto _ : state) {
    auto r = q.getTopK(15.0);
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  applyQueryId(state, QueryId::TopK_k15);
  applyResultSize(state, last_result_bytes);
  state.counters["Param_k"] = 15;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxF_TopK, TopK_k15);

// ---- 1DxP ----
class AirTreeQuery1DxP_TopK : public QueryFixtureBase {};

BENCHMARK_DEFINE_F(AirTreeQuery1DxP_TopK, TopK_k1)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  
  std::size_t last_result_bytes = 0;
  
  for (auto _ : state) {
    auto r = q.getTopK(1.0);
    
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  
  applyQueryId(state, QueryId::TopK_k1);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Param_k"] = 1;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxP_TopK, TopK_k1);

BENCHMARK_DEFINE_F(AirTreeQuery1DxP_TopK, TopK_k5)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  
  std::size_t last_result_bytes = 0;
  
  for (auto _ : state) {
    auto r = q.getTopK(5.0);
    
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  
  applyQueryId(state, QueryId::TopK_k5);
  applyResultSize(state, last_result_bytes);
  
  state.counters["Param_k"] = 5;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxP_TopK, TopK_k5);

BENCHMARK_DEFINE_F(AirTreeQuery1DxP_TopK, TopK_k15)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  airtree::query::topk::TopK q(buffer_);
  std::size_t last_result_bytes = 0;
  for (auto _ : state) {
    auto r = q.getTopK(15.0);
    last_result_bytes = r.size() * sizeof(typename decltype(r)::value_type);
    benchmark::DoNotOptimize(r.data());
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  applyQueryId(state, QueryId::TopK_k15);
  applyResultSize(state, last_result_bytes);
  state.counters["Param_k"] = 15;
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxP_TopK, TopK_k15);


// Construction only (builds the 1D bin table).
BENCHMARK_DEFINE_F(AirTreeQuery1DxT_TopK, TopK_ctor)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  for (auto _ : state) {
    airtree::query::topk::TopK q(buffer_);
    benchmark::DoNotOptimize(q);
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  applyQueryId(state, QueryId::TopK_ctor);
  applyResultSize(state, 0);
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxT_TopK, TopK_ctor);

BENCHMARK_DEFINE_F(AirTreeQuery1DxF_TopK, TopK_ctor)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  for (auto _ : state) {
    airtree::query::topk::TopK q(buffer_);
    benchmark::DoNotOptimize(q);
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  applyQueryId(state, QueryId::TopK_ctor);
  applyResultSize(state, 0);
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxF_TopK, TopK_ctor);

BENCHMARK_DEFINE_F(AirTreeQuery1DxP_TopK, TopK_ctor)(benchmark::State &state) {
  if (!hasBuffer()) {
    state.SkipWithError("empty .airtree buffer");
    return;
  }
  for (auto _ : state) {
    airtree::query::topk::TopK q(buffer_);
    benchmark::DoNotOptimize(q);
    benchmark::ClobberMemory();
  }
  applyCommonCounters(state);
  applyQueryId(state, QueryId::TopK_ctor);
  applyResultSize(state, 0);
}
BENCHMARK_REGISTER_F(AirTreeQuery1DxP_TopK, TopK_ctor);

} // namespace
