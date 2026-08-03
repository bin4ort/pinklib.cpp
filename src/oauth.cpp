#include "oauth.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <random>
#include <sstream>
#include <thread>
#include <chrono>
#include <iostream>

namespace pinklib {

std::shared_ptr<Oauth> OAUTH_CLIENT;
std::mutex OAUTH_MUTEX;
std::atomic<uint16_t> OAUTH_RATELIMIT_REMAINING{99};
std::atomic<bool> OAUTH_IS_ROLLING_OVER{false};

static size_t curl_write_cb(void* data, size_t size, size_t nmemb, void* userp) {
    auto* buf = static_cast<std::string*>(userp);
    buf->append(static_cast<char*>(data), size * nmemb);
    return size * nmemb;
}

static std::string gen_random_string(size_t length) {
    static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.resize(length);
    for (size_t i = 0; i < length; i++) {
        result[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    return result;
}

// Direct HTTP call to Reddit auth - does NOT use reddit_json() to avoid circular dep
// Redlib's OAuth client ID for Android app spoofing
static const char* DEFAULT_CLIENT_ID = "ohXpoqrZYub1kg";

static std::string get_client_auth() {
    // Encode "client_id:" as base64 for Basic auth header
    std::string client_id = DEFAULT_CLIENT_ID;
    const char* env = std::getenv("PINKLIB_CLIENT_ID");
    if (env) client_id = env;
    
    // Manually base64 encode "client_id:" 
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string input = client_id + ":";
    std::string result;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(b64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return "Basic " + result;
}

static std::string gen_android_user_agent() {
    // Random Android version and Reddit app version like the original redlib
    int android_ver = 12 + (rand() % 3); // 12-14
    static const char* app_versions[] = {
        "2024.22.1", "2024.25.0", "2024.30.0", "2024.35.0", "2024.40.0"
    };
    const char* ver = app_versions[rand() % 5];
    return "Reddit/" + std::string(ver) + "/Android " + std::to_string(android_ver);
}

static std::string fetch_oauth_token(const std::string& device_id) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    std::string ua = gen_android_user_agent();

    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("User-Agent: " + ua).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    headers = curl_slist_append(headers, ("Authorization: " + get_client_auth()).c_str());

    std::string body = "grant_type=https%3A%2F%2Foauth.reddit.com%2Fgrants%2Finstalled_client&device_id=" + device_id;

    curl_easy_setopt(curl, CURLOPT_URL, "https://www.reddit.com/api/v1/access_token");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body.size());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    if (res == CURLE_OK) curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "[OAUTH] curl error: " << curl_easy_strerror(res) << std::endl;
        return "";
    }
    if (http_code != 200) {
        std::cerr << "[OAUTH] HTTP " << http_code << ": " << response.substr(0, 200) << std::endl;
        return "";
    }
    return response;
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
        std::cerr << "[OAUTH] Fetching token..." << std::endl;
        std::string resp_body = fetch_oauth_token(device_id);
        std::cerr << "[OAUTH] Response length: " << resp_body.size() << std::endl;
        if (!resp_body.empty()) {
            auto json_resp = nlohmann::json::parse(resp_body);
            if (json_resp.contains("access_token")) {
                std::string token = json_resp["access_token"].get<std::string>();
                oauth.headers_map["Authorization"] = "Bearer " + token;
                oauth.expires_in = json_resp.value("expires_in", 3600ULL);
                std::cerr << "[OAUTH] Got token! expires_in=" << oauth.expires_in << std::endl;
                return oauth;
            }
            std::cerr << "[OAUTH] No access_token in response: " << resp_body.substr(0, 200) << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "[OAUTH] Exception: " << e.what() << std::endl;
    }

    std::cerr << "[OAUTH] Falling back to no-token mode" << std::endl;
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
