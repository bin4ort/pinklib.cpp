#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>

namespace pinklib {

// OAuth response
struct OauthResponse {
    std::string token;
    uint64_t expires_in = 0;
    std::unordered_map<std::string, std::string> additional_headers;
};

// OAuth client
struct Oauth {
    std::unordered_map<std::string, std::string> headers_map;
    uint64_t expires_in = 0;
    // backend is implicitly selected
    bool is_mobile_spoof = true;

    static Oauth create();
    std::string user_agent() const;
};

extern std::shared_ptr<Oauth> OAUTH_CLIENT;
extern std::mutex OAUTH_MUTEX;
extern std::atomic<uint16_t> OAUTH_RATELIMIT_REMAINING;
extern std::atomic<bool> OAUTH_IS_ROLLING_OVER;

void init_oauth();
void token_daemon();
void force_refresh_token();

} // namespace pinklib
