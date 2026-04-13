#include <cstdint>
#ifndef FEATURE_FLAGS_H

const uint64_t threshold_1D = 64 * 1024 * 1024; // 64 MB in-memory limit
const bool enable_threshold_1D =
    true; // set this flag false to disable the threshold

const uint64_t threshold_4D = 128 * 1024 * 1024; // 128 MB in-memory limit
const bool enable_threshold_4D =
    true; // set this flag false to disable the threshold

const uint64_t threshold_3D = 128 * 1024 * 1024; // 128 MB in-memory limit
const bool enable_threshold_3D =
    true; // set this flag false to disable the threshold

const uint64_t threshold_2D = 128 * 1024 * 1024; // 128 MB in-memory limit
const bool enable_threshold_2D =
    true; // set this flag false to disable the threshold
#endif    // FEATURE_FLAGS_H
