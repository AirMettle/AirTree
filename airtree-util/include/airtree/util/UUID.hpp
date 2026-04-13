#ifndef AIRTREE_UTIL_INCLUDE_AIRTREE_UTIL_UUID_HPP
#define AIRTREE_UTIL_INCLUDE_AIRTREE_UTIL_UUID_HPP

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <string>

namespace airtree::util::uuid {

class AirTreeUUID {
public:
  static std::string generateUUID();
};

} // namespace airtree::util::uuid

#endif // AIRTREE_UTIL_INCLUDE_AIRTREE_UTIL_UUID_HPP
