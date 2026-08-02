#pragma once

#include "utils.h"
#include <string>
#include <unordered_map>

namespace pinklib {

std::string subreddit_community(const std::string& path,
                                 const std::string& query_string,
                                 const std::unordered_map<std::string, std::string>& params,
                                 const std::unordered_map<std::string, std::string>& cookies);

std::string subreddit_wiki(const std::string& path,
                            const std::string& query_string,
                            const std::unordered_map<std::string, std::string>& params,
                            const std::unordered_map<std::string, std::string>& cookies);

std::string subreddit_sidebar(const std::string& path,
                               const std::string& query_string,
                               const std::unordered_map<std::string, std::string>& params,
                               const std::unordered_map<std::string, std::string>& cookies);

std::string subreddit_subscriptions_filters(const std::string& method,
                                             const std::string& path,
                                             const std::string& query_string,
                                             const std::unordered_map<std::string, std::string>& params,
                                             const std::unordered_map<std::string, std::string>& cookies,
                                             std::unordered_map<std::string, std::string>& set_cookies);

std::string subreddit_add_quarantine(const std::string& path,
                                      const std::string& query_string,
                                      const std::unordered_map<std::string, std::string>& params,
                                      std::unordered_map<std::string, std::string>& set_cookies);

bool can_access_quarantine(const std::unordered_map<std::string, std::string>& cookies,
                           const std::string& sub);

SubredditData fetch_subreddit(const std::string& sub, bool quarantined);

std::string subreddit_rss(const std::string& path,
                           const std::string& query_string,
                           const std::unordered_map<std::string, std::string>& params,
                           const std::unordered_map<std::string, std::string>& cookies);

} // namespace pinklib
