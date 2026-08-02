#include "oauth.h"
#include "client.h"
#include <random>
#include <sstream>
#include <thread>
#include <chrono>
#include <openssl/rand.h>

namespace pinklib {

std::shared_ptr<Oauth> OAUTH_CLIENT;
std::mutex OAUTH_MUTEX;
std::atomic<uint16_t> OAUTH_RATELIMIT_REMAINING{99};
std::atomic<bool> OAUTH_IS_ROLLING_OVER{false};

static std::string gen_random_string(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.resize(length);
    unsigned char buf[length];
    RAND_bytes(buf, length);
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[buf[i] % (sizeof(charset) - 1)];
    }
    return result;
}

void init_oauth() {
    std::lock_guard<std::mutex> lock(OAUTH_MUTEX);
    if (OAUTH_CLIENT) return;
    OAUTH_CLIENT = std::make_shared<Oauth>(Oauth::create());
    // Start token daemon in background
    std::thread([]() { token_daemon(); }).detach();
}

Oauth Oauth::create() {
    Oauth oauth;
    oauth.is_mobile_spoof = true;

    // Generate device-like credentials
    std::string uuid = gen_random_string(8) + "-" + gen_random_string(4) + "-" +
                       gen_random_string(4) + "-" + gen_random_string(4) + "-" + gen_random_string(12);
    std::string device_id = gen_random_string(20);

    // Build headers for Reddit API
    oauth.headers_map = {
        {"User-Agent", "PinkLib/1.0"},
        {"Content-Type", "application/json; charset=UTF-8"},
        {"X-Reddit-Device-Id", uuid},
        {"client-vendor-id", uuid}
    };

    // Try to authenticate with Reddit
    try {
        std::string auth_body = "grant_type=https%3A%2F%2Foauth.reddit.com%2Fgrants%2Finstalled_client&device_id=" + device_id;
        auto json_resp = reddit_json("/api/v1/access_token", false);
        if (json_resp.contains("access_token")) {
            std::string token = json_resp["access_token"].get<std::string>();
            oauth.headers_map["Authorization"] = "Bearer " + token;
            oauth.expires_in = json_resp.value("expires_in", 3600ULL);
        }
    } catch (...) {
        // Fall back to generic web auth
        oauth.is_mobile_spoof = false;
    }

    return oauth;
}

std::string Oauth::user_agent() const {
    auto it = headers_map.find("User-Agent");
    return it != headers_map.end() ? it->second : "PinkLib/1.0";
}

void token_daemon() {
    while (true) {
        uint64_t wait_time = 3600;
        {
            std::lock_guard<std::mutex> lock(OAUTH_MUTEX);
            if (OAUTH_CLIENT) wait_time = OAUTH_CLIENT->expires_in;
        }
        if (wait_time > 120) wait_time -= 120;
        std::this_thread::sleep_for(std::chrono::seconds(wait_time));
        force_refresh_token();
    }
}

void force_refresh_token() {
    bool expected = false;
    if (!OAUTH_IS_ROLLING_OVER.compare_exchange_strong(expected, true)) return;

    try {
        auto new_client = std::make_shared<Oauth>(Oauth::create());
        std::lock_guard<std::mutex> lock(OAUTH_MUTEX);
        OAUTH_CLIENT = new_client;
        OAUTH_RATELIMIT_REMAINING = 99;
    } catch (...) {}

    OAUTH_IS_ROLLING_OVER = false;
}

} // namespace pinklib
