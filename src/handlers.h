#pragma once
#include "utils.h"
#include <string>
#include <unordered_map>

namespace pinklib {

std::string post_item(const std::string& path,
                       const std::string& query_string,
                       const std::unordered_map<std::string, std::string>& params,
                       const std::unordered_map<std::string, std::string>& cookies);

std::string user_profile(const std::string& path,
                          const std::string& query_string,
                          const std::unordered_map<std::string, std::string>& params,
                          const std::unordered_map<std::string, std::string>& cookies);

std::string user_rss(const std::string& path,
                      const std::string& query_string,
                      const std::unordered_map<std::string, std::string>& params,
                      const std::unordered_map<std::string, std::string>& cookies);

std::string search_find(const std::string& path,
                         const std::string& query_string,
                         const std::unordered_map<std::string, std::string>& params,
                         const std::unordered_map<std::string, std::string>& cookies);

std::string settings_get(const std::string& path,
                          const std::string& query_string,
                          const std::unordered_map<std::string, std::string>& params,
                          const std::unordered_map<std::string, std::string>& cookies);

std::string settings_set(const std::string& path,
                          const std::string& query_string,
                          const std::string& body,
                          const std::unordered_map<std::string, std::string>& params,
                          const std::unordered_map<std::string, std::string>& cookies,
                          std::unordered_map<std::string, std::string>& set_cookies);

std::string settings_restore(const std::string& path,
                              const std::string& query_string,
                              const std::unordered_map<std::string, std::string>& params,
                              const std::unordered_map<std::string, std::string>& cookies,
                              std::unordered_map<std::string, std::string>& set_cookies);

std::string settings_update(const std::string& path,
                             const std::string& query_string,
                             const std::unordered_map<std::string, std::string>& params,
                             const std::unordered_map<std::string, std::string>& cookies,
                             std::unordered_map<std::string, std::string>& set_cookies);

std::string duplicates_item(const std::string& path,
                             const std::string& query_string,
                             const std::unordered_map<std::string, std::string>& params,
                             const std::unordered_map<std::string, std::string>& cookies);

std::string instance_info_page(const std::string& path,
                                const std::string& query_string,
                                const std::unordered_map<std::string, std::string>& params,
                                const std::unordered_map<std::string, std::string>& cookies);

} // namespace pinklib
