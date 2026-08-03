#include "client.h"
#include "config.h"
#include "oauth.h"
#include <curl/curl.h>
#include <mutex>
#include <sstream>
#include <cstring>

namespace pinklib {

// ---- curl-based HTTP for Reddit API (better TLS fingerprint) ----

static size_t curl_write_cb(void* data, size_t size, size_t nmemb, void* userp) {
    auto* buf = static_cast<std::string*>(userp);
    buf->append(static_cast<char*>(data), size * nmemb);
    return size * nmemb;
}

static std::string curl_get(const std::string& url, const std::string& bearer_token) {
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    std::string response;
    curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Linux; Android 14) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/125.0.6422.165 Mobile Safari/537.36");
    headers = curl_slist_append(headers, "Accept: application/json");
    if (!bearer_token.empty()) {
        headers = curl_slist_append(headers, ("Authorization: Bearer " + bearer_token).c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        throw std::runtime_error(std::string("Curl error: ") + curl_easy_strerror(res));
    }

    if (http_code >= 400) {
        throw std::runtime_error("Reddit API returned " + std::to_string(http_code) + " (forbidden)");
    }

    return response;
}

static std::string get_bearer_token() {
    std::lock_guard<std::mutex> lock(OAUTH_MUTEX);
    if (OAUTH_CLIENT) {
        auto it = OAUTH_CLIENT->headers_map.find("Authorization");
        if (it != OAUTH_CLIENT->headers_map.end()) {
            std::string val = it->second;
            if (val.starts_with("Bearer ")) return val.substr(7);
        }
    }
    return "";
}

json reddit_json(const std::string& path, bool quarantine) {
    std::string url = std::string(REDDIT_URL_BASE) + path;
    std::string token = get_bearer_token();

    std::string response = curl_get(url, token);

    try {
        return json::parse(response);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()) + " | first bytes: " +
            response.substr(0, 100));
    }
}

std::string canonical_path(const std::string& path, int tries) {
    if (tries == 0) return "";
    try {
        std::string url = std::string(REDDIT_URL_BASE) + path;
        // HEAD request via curl
        CURL* curl = curl_easy_init();
        if (!curl) return "";

        curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "User-Agent: Mozilla/5.0 (Linux; Android 14) Chrome/125.0.6422.165");
        std::string token = get_bearer_token();
        if (!token.empty()) headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);

        curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (http_code >= 200 && http_code < 300) return path;
    } catch (...) {}
    return "";
}

void init_client() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void rate_limit_check() {
    try { reddit_json("/r/reddit/hot.json?raw_json=1", true); } catch (...) {}
}

std::string proxy(const std::string& path, const std::string& format_str) {
    return "";
}

} // namespace pinklib
