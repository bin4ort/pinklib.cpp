#pragma once

#include "utils.h"
#include <string>

namespace pinklib {

json reddit_json(const std::string& path, bool quarantine);
std::string reddit_request(const std::string& method, const std::string& path,
                           bool redirect, bool quarantine);
std::string canonical_path(const std::string& path, int tries);

void init_client();
void rate_limit_check();
std::string proxy(const std::string& path, const std::string& format);

} // namespace pinklib
