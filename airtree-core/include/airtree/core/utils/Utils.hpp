#ifndef FLOATINGPOINTHISTOGRAM_UTILS_H
#define FLOATINGPOINTHISTOGRAM_UTILS_H

#include <vector>
#include <openssl/evp.h>
#include <openssl/types.h>
#include <string>

std::vector<double> readBinaryFile(const std::string &filename);
std::vector<int32_t> readBinaryFileInt32(const std::string &filename);
std::string hashBuffer(const std::vector<char> &buffer);
std::string readValueFromJson(const std::string &filename,
                              const std::string &key);


#endif // FLOATINGPOINTHISTOGRAM_UTILS_H
