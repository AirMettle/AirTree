#include <airtree/util/UUID.hpp>

using namespace airtree::util::uuid;

std::string AirTreeUUID::generateUUID() {
  boost::uuids::uuid uuid = boost::uuids::random_generator()();
  std::string uuid_str = to_string(uuid);
  return uuid_str;
}