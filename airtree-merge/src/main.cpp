// Required Notice: Copyright AirMettle, Inc. 2026 (https://airmettle.com/)

#include <airtree/merge/AirTreeMerge.hpp>
#include <airtree/merge/Logger.hpp>
#include <fstream>
#include <vector>

#ifdef __linux__
#include <unistd.h>
#include <sys/resource.h>
#endif

using namespace airtree::merge;

long getPeakMemoryUsage() {
#ifdef __linux__
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);
  return usage.ru_maxrss;
#else
  return -1;
#endif
}

long monitorMemoryUsage() {
#ifdef __linux__
  long maxMemory = 0;
  while (true) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    if (usage.ru_maxrss > maxMemory) {
      maxMemory = usage.ru_maxrss;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return maxMemory;
  }
#else
  return -1;
#endif
}

int main(int argc, char *argv[]) {
  // Usage: ./ResultMerge <input_file1> <input_file2> <output_file>
  if (argc != 4) {
    SPDLOG_LOGGER_ERROR(logger(),
                        "Usage: {} <input_file1> <input_file2> <output_file>",
                        argv[0]);
    return EXIT_FAILURE;
  }

  std::string inputFile1 = argv[1];
  std::string inputFile2 = argv[2];
  std::string outputFile = argv[3];
  auto startTime = std::chrono::high_resolution_clock::now();
  std::thread memoryThread([&] { monitorMemoryUsage(); });
  try {
    SPDLOG_LOGGER_INFO(logger(), "Opening first input file: {}", inputFile1);
    std::ifstream inFile1(inputFile1, std::ios::binary);
    if (!inFile1)
      throw std::runtime_error("Failed to open input file: " + inputFile1);
    std::vector<char> buffer1((std::istreambuf_iterator<char>(inFile1)),
                              std::istreambuf_iterator<char>());
    inFile1.close();
    SPDLOG_LOGGER_INFO(
        logger(), "First input file read, size: {}", buffer1.size());

    SPDLOG_LOGGER_INFO(logger(), "Opening second input file: {}", inputFile2);
    std::ifstream inFile2(inputFile2, std::ios::binary);
    if (!inFile2)
      throw std::runtime_error("Failed to open input file: " + inputFile2);
    std::vector<char> buffer2((std::istreambuf_iterator<char>(inFile2)),
                              std::istreambuf_iterator<char>());
    inFile2.close();
    SPDLOG_LOGGER_INFO(
        logger(), "Second input file read, size: {}", buffer2.size());

    SPDLOG_LOGGER_INFO(logger(), "File 1 Size: {}", buffer1.size());
    SPDLOG_LOGGER_INFO(logger(), "File 2 Size: {}", buffer2.size());
    SPDLOG_LOGGER_INFO(
        logger(), "Total Size: {}", buffer1.size() + buffer2.size());

    // Perform the Merge
    mergeAirTree(buffer1, buffer2, outputFile);
  } catch (const std::exception &ex) {
    SPDLOG_LOGGER_ERROR(logger(), "Error: {}", ex.what());
    return EXIT_FAILURE;
  }
  auto endTime = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime)
          .count();

  long peakMemory = getPeakMemoryUsage();
  memoryThread.detach();

  SPDLOG_LOGGER_INFO(logger(), "Execution Time: {} ms", duration);
  SPDLOG_LOGGER_INFO(logger(), "Peak Memory Usage: {} KB", peakMemory);
  SPDLOG_LOGGER_INFO(logger(), "Program finished successfully");
  return EXIT_SUCCESS;
}
