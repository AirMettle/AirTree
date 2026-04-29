#include <airtree/core/utils/Utils.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>


std::vector<double> readBinaryFile(const std::string &filename) {
  std::vector<double> data;
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return data; // Return an empty vector in case of error
  }

  double value;
  while (file.read(reinterpret_cast<char *>(&value), sizeof(double))) {
    data.push_back(value);
  }

  file.close();
  return data;
}

std::vector<int32_t> readBinaryFileInt32(const std::string &filename) {
  std::vector<int32_t> data;
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    std::cerr << "Error opening file: " << filename << std::endl;
  }

  int32_t value;
  while (file.read(reinterpret_cast<char *>(&value), sizeof(int32_t))) {
    data.push_back(value);
  }

  file.close();
  return data;
}

// Function to hash a buffer using SHA-256 with EVP
std::string hashBuffer(const std::vector<char> &buffer) {
  EVP_MD_CTX *context = EVP_MD_CTX_new();
  if (context == nullptr) {
    throw std::runtime_error("Failed to create EVP_MD_CTX");
    // LOG_ERROR("Failed to create EVP_MD_CTX");
  }

  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int lengthOfHash = 0;

  if (!EVP_DigestInit_ex(context, EVP_sha256(), nullptr)
      || !EVP_DigestUpdate(context, buffer.data(), buffer.size())
      || !EVP_DigestFinal_ex(context, hash, &lengthOfHash)) {
    EVP_MD_CTX_free(context);
    throw std::runtime_error("Hash computation failed");
    // LOG_ERROR("Hash computation failed");
  }

  EVP_MD_CTX_free(context);

  std::ostringstream oss;
  for (unsigned int i = 0; i < lengthOfHash; ++i) {
    oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
  }
  return oss.str();
}

std::string readValueFromJson(const std::string &filename,
                              const std::string &key) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open JSON file.");
  }

  std::string line;
  std::string value;
  bool key_found = false;

  // Read file line by line
  while (std::getline(file, line)) {
    // Look for the key
    size_t key_pos = line.find("\"" + key + "\":");
    if (key_pos != std::string::npos) {
      // Extract the value
      size_t start = line.find("\"", key_pos + key.length() + 2) + 1;
      size_t end = line.find("\"", start);
      if (start != std::string::npos && end != std::string::npos) {
        value = line.substr(start, end - start);
        key_found = true;
        break;
      }
    }
  }

  file.close();

  if (!key_found) {
    throw std::runtime_error("Key not found in JSON file.");
  }

  return value;
}