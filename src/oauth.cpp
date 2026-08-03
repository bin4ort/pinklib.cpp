#include "oauth.h"
#include <httplib.h>
#include <nlohmann/json.hpp>
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
    std::vector<unsigned char> buf(length);
    RAND_bytes(buf.data(), length);
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[buf[i] % (sizeof(charset) - 1)];
    }
    return result;
}

// Direct HTTP call to Reddit auth - does NOT use reddit_json() to avoid circular dep
// OAuth client credentials - set via env vars PINKLIB_CLIENT_ID / PINKLIB_CLIENT_SECRET
// Or get them from https://www.reddit.com/prefs/apps (create an "installed app")
static std::string get_client_id() {
    const char* env = std::getenv("PINKLIB_CLIENT_ID");
    if (env) return env;
    env = std::getenv("REDLIB_CLIENT_ID");
    if (env) return env;
    return "M1hmQkpXbGlIdnFBQ25YcmZJWWxMdzo="; // fallback, likely rejected
}

static std::string fetch_oauth_token(const std::string& device_id) {
    std::string client_id = get_client_id();
    // Decode base64 to get client_id, then re-encode with secret if available
    // For installed apps, just the client_id is used as "client_id:" base64
    std::string auth_header = "Basic " + client_id;

    httplib::Client cli("https://www.reddit.com");
    cli.set_read_timeout(10);
    cli.set_write_timeout(10);
    cli.set_connection_timeout(10);
    cli.set_follow_location(true);

    httplib::Headers headers = {
        {"User-Agent", "Mozilla/5.0 (Linux; Android 14) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.6422.165 Mobile Safari/537.36"},
        {"Content-Type", "application/x-www-form-urlencoded"},
        {"Accept", "application/json"},
        {"Authorization", auth_header},
    };

    std::string body = "grant_type=https%3A%2F%2Foauth.reddit.com%2Fgrants%2Finstalled_client&device_id=" + device_id;

    auto res = cli.Post("/api/v1/access_token", headers, body, "application/x-www-form-urlencoded");
    if (res && res->status == 200) return res->body;
    if (res) {
        std::cerr << "[OAUTH] " << res->status << ": " << res->body.substr(0, 200) << std::endl;
    }
    return "";
}

void init_oauth() {
    // Create an initial OAuth client WITHOUT locking OAUTH_MUTEX during HTTP call
    auto client = Oauth::create();
    {
        std::lock_guard<std::mutex> lock(OAUTH_MUTEX);
        if (OAUTH_CLIENT) return; // already initialized by another thread
        OAUTH_CLIENT = std::make_shared<Oauth>(std::move(client));
    }
    std::thread([]() { token_daemon(); }).detach();
}

Oauth Oauth::create() {
    Oauth oauth;
    oauth.is_mobile_spoof = true;

    std::string uuid = gen_random_string(8) + "-" + gen_random_string(4) + "-" +
                       gen_random_string(4) + "-" + gen_random_string(4) + "-" + gen_random_string(12);
    std::string device_id = gen_random_string(20);

    oauth.headers_map = {
        {"User-Agent", "Mozilla/5.0 (Linux; Android 14) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.6422.165 Mobile Safari/537.36"},
        {"Content-Type", "application/json; charset=UTF-8"},
        {"Accept", "application/json"},
        {"X-Reddit-Device-Id", uuid},
        {"client-vendor-id", uuid}
    };

    // Fetch OAuth token via direct HTTP (no circular dependency)
    try {
        std::string resp_body = fetch_oauth_token(device_id);
        if (!resp_body.empty()) {
            auto json_resp = nlohmann::json::parse(resp_body);
            if (json_resp.contains("access_token")) {
                std::string token = json_resp["access_token"].get<std::string>();
                oauth.headers_map["Authorization"] = "Bearer " + token;
                oauth.expires_in = json_resp.value("expires_in", 3600ULL);
                return oauth;
            }
        }
    } catch (...) {}

    // Fallback: create without token (will serve generic responses)
    oauth.is_mobile_spoof = false;
    oauth.expires_in = 3600;
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
