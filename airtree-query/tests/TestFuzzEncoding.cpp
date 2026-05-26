// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/core/AirTreeCore_internal.hpp>
#include <airtree/core/common/InternalEncoding.hpp>
#include <airtree/core/common/Reconstruct.hpp>
#include <airtree/query/AirTreeQuery_internal.hpp>
#include <airtree/query/bin-boundary/BinBoundary.hpp>

#include <gtest/gtest.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <map>
#include <random>
#include <vector>

using namespace airtree::query::bin_boundary;

// Encoder wrappers
static unsigned int encode_1DxP(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  return createInternal20Bit(fp, true);
}
static unsigned int encode_1DxT(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  return createInternal13Bit(fp, true);
}
static unsigned int encode_1DxF(double val) {
  uint64_t fp;
  std::memcpy(&fp, &val, sizeof(val));
  return createInternal16Bit(fp, true);
}

// Pattern-based bin count consistency: re-encode each value, match by
// reConstruct label. Shares the encoder, so does not independently verify
// the encoding spec — but catches serialization/query bugs and count loss.
static void verifyBinAccuracy1D(const std::vector<char> &buffer,
                                const std::vector<double> &raw_data,
                                std::function<unsigned int(double)> encodeFn,
                                int bitLength) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary1DList>(*result.getBoundaries());

  std::map<double, uint32_t> hist_counts;
  for (const auto &bin : bins) {
    double recon =
        (bin.getUpperBound() <= 0) ? bin.getUpperBound() : bin.getLowerBound();
    hist_counts[recon] = bin.getCount();
  }

  std::map<double, int> actual_counts;
  for (double v : raw_data) {
    if (v == 0.0 || std::isnan(v) || std::isinf(v))
      continue;
    unsigned int pattern = encodeFn(v);
    double bin_label = reConstruct<double>(pattern, bitLength);
    actual_counts[bin_label]++;
  }

  for (const auto &[label, hist_count] : hist_counts) {
    int actual = actual_counts.count(label) ? actual_counts[label] : 0;
    EXPECT_EQ(actual, static_cast<int>(hist_count))
        << "Bin at recon=" << label << ": actual=" << actual
        << " hist=" << hist_count;
  }

  for (const auto &[label, count] : actual_counts) {
    EXPECT_TRUE(hist_counts.count(label) > 0)
        << "Value encoded to bin at recon=" << label
        << " which doesn't exist in histogram output";
  }
}

// Sanity checks: total counts, non-overlapping, no NaN/Inf.
static void verifyHistogramSanity1D(const std::vector<char> &buffer,
                                    const std::vector<double> &raw_data) {
  BinBoundary query(buffer);
  auto result = query.generateBinBoundaries();
  auto bins = std::get<BinBoundary1DList>(*result.getBoundaries());

  std::sort(bins.begin(), bins.end(),
            [](const BinBoundary1D &a, const BinBoundary1D &b) {
              return a.getLowerBound() < b.getLowerBound();
            });

  int non_special = 0;
  for (double v : raw_data) {
    if (v != 0.0 && !std::isnan(v) && !std::isinf(v))
      non_special++;
  }

  int total_hist = 0;
  for (const auto &bin : bins)
    total_hist += bin.getCount();
  EXPECT_EQ(total_hist, non_special)
      << "Total histogram count != non-special data count";

  for (size_t i = 1; i < bins.size(); i++) {
    EXPECT_GE(bins[i].getLowerBound(), bins[i - 1].getUpperBound())
        << "Bin " << i << " overlaps with previous";
  }

  for (size_t i = 0; i < bins.size(); i++) {
    EXPECT_FALSE(std::isnan(bins[i].getLowerBound()));
    EXPECT_FALSE(std::isnan(bins[i].getUpperBound()));
    EXPECT_FALSE(std::isinf(bins[i].getLowerBound()));
    EXPECT_GT(bins[i].getCount(), 0u);
  }
}

TEST(FuzzEncoding, RandomDoubles_1DxP) {
  std::mt19937_64 rng(42); // fixed seed for reproducibility
  std::vector<double> data;

  // 50K with random valid exponents (uniform across the exponent range)
  for (int i = 0; i < 50000; i++) {
    uint64_t bits = rng();
    // Clear exponent, set a random valid exponent (1-2046, avoids 0=subnormal
    // and 2047=NaN/Inf)
    bits &= ~(0x7FFull << 52);
    bits |= (static_cast<uint64_t>((rng() % 2046) + 1) << 52);
    double v;
    std::memcpy(&v, &bits, sizeof(v));
    if (!std::isnan(v) && !std::isinf(v) && v != 0.0)
      data.push_back(v);
  }

  // 50K concentrated near powers of 2 (the saturation zone)
  std::uniform_real_distribution<double> eps_dist(1e-8, 0.02);
  std::uniform_int_distribution<int> pow_dist(-20, 20);
  for (int i = 0; i < 50000; i++) {
    int p = pow_dist(rng);
    double base = std::pow(2.0, p);
    double eps = eps_dist(rng);
    double v = base * (1.0 + eps);
    if (rng() % 2)
      v = -v;
    data.push_back(v);
  }

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  auto buffer = generate_1DxP(array);
  verifyHistogramSanity1D(buffer, data);
  verifyBinAccuracy1D(buffer, data, encode_1DxP, 20);
}

TEST(FuzzEncoding, RandomDoubles_1DxT) {
  std::mt19937_64 rng(123);
  std::vector<double> data;

  std::uniform_real_distribution<double> eps_dist(1e-8, 0.02);
  std::uniform_int_distribution<int> pow_dist(-20, 20);
  for (int i = 0; i < 50000; i++) {
    int p = pow_dist(rng);
    double base = std::pow(2.0, p);
    double eps = eps_dist(rng);
    double v = base * (1.0 + eps);
    if (rng() % 2)
      v = -v;
    data.push_back(v);
  }

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  auto buffer = generate_1DxT(array);
  verifyHistogramSanity1D(buffer, data);
  verifyBinAccuracy1D(buffer, data, encode_1DxT, 13);
}

TEST(FuzzEncoding, RandomDoubles_1DxF) {
  std::mt19937_64 rng(456);
  std::vector<double> data;

  std::uniform_real_distribution<double> eps_dist(1e-8, 0.02);
  std::uniform_int_distribution<int> pow_dist(-20, 20);
  for (int i = 0; i < 50000; i++) {
    int p = pow_dist(rng);
    double base = std::pow(2.0, p);
    double eps = eps_dist(rng);
    double v = base * (1.0 + eps);
    if (rng() % 2)
      v = -v;
    data.push_back(v);
  }

  FPHArray array = buildFPHArray(data.data(), static_cast<int>(data.size()));
  auto buffer = generate_1DxF(array);
  verifyHistogramSanity1D(buffer, data);
  verifyBinAccuracy1D(buffer, data, encode_1DxF, 16);
}

TEST(FuzzEncoding, RandomFloats_1DxP) {
  std::mt19937 rng(789);
  std::vector<float> float_data;

  // Random floats with valid exponents (positive and negative)
  for (int i = 0; i < 50000; i++) {
    uint32_t bits = rng();
    bits &= ~(0xFFu << 23);
    bits |= (((rng() % 254) + 1) << 23); // exponent 1-254
    float v;
    std::memcpy(&v, &bits, sizeof(v));
    if (!std::isnan(v) && !std::isinf(v) && v != 0.0f)
      float_data.push_back(v);
  }

  // Floats near powers of 2 (positive and negative)
  std::uniform_real_distribution<float> eps_dist(1e-7f, 0.02f);
  std::uniform_int_distribution<int> pow_dist(-10, 10);
  for (int i = 0; i < 50000; i++) {
    int p = pow_dist(rng);
    float base = std::pow(2.0f, p);
    float eps = eps_dist(rng);
    float v = base * (1.0f + eps);
    if (rng() % 2)
      v = -v;
    float_data.push_back(v);
  }

  FPHArray array =
      buildFPHArray(float_data.data(), static_cast<int>(float_data.size()));
  auto buffer = generate_1DxP(array);

  std::vector<double> as_double(float_data.begin(), float_data.end());
  verifyHistogramSanity1D(buffer, as_double);
  verifyBinAccuracy1D(buffer, as_double, encode_1DxP, 20);
}
