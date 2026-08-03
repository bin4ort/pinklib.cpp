#include "client.h"
#include "config.h"
#include "oauth.h"
#include <httplib.h>
#include <thread>
#include <chrono>
#include <sstream>

namespace pinklib {

using namespace std::string_literals;

// Use cpp-httplib for HTTP

static httplib::Client* reddit_client() {
    static thread_local auto cli = []() -> std::unique_ptr<httplib::Client> {
        auto c = std::make_unique<httplib::Client>("https://oauth.reddit.com");
        c->set_follow_location(false);
        c->set_read_timeout(10);
        c->set_write_timeout(10);
        c->set_connection_timeout(5);
        return c;
    }();
    return cli.get();
}

static httplib::Client* www_client() {
    static thread_local auto cli = std::make_unique<httplib::Client>("https://www.reddit.com");
    return cli.get();
}

static std::pair<httplib::Client*, std::string> resolve_client(const std::string& path) {
    // Some paths go to www.reddit.com instead of oauth
    if (path.find("/api/v1/access_token") != std::string::npos) {
        return {www_client(), path};
    }
    return {reddit_client(), path};
}

static httplib::Headers build_headers(bool quarantine, const std::string& path) {
    httplib::Headers headers;

    {
        std::lock_guard<std::mutex> lock(OAUTH_MUTEX);
        if (OAUTH_CLIENT) {
            for (const auto& [k, v] : OAUTH_CLIENT->headers_map) {
                headers.emplace(k, v);
            }
        }
    }

    // Shuffle headers for anti-detection
    // (simplified - real version would randomize order)

    if (quarantine) {
        headers.emplace("Cookie", "_options=%7B%22pref_quarantine_optin%22%3A%20true%2C%20%22pref_gated_sr_optin%22%3A%20true%7D");
    }

    return headers;
}

json reddit_json(const std::string& path, bool quarantine) {
    // Rate limit check
    uint16_t current_rate = OAUTH_RATELIMIT_REMAINING.load();
    if (current_rate < 10 && !OAUTH_IS_ROLLING_OVER.load()) {
        std::thread([]() { force_refresh_token(); }).detach();
    }
    OAUTH_RATELIMIT_REMAINING.fetch_sub(1);

    auto [client, resolved_path] = resolve_client(path);
    auto headers = build_headers(quarantine, resolved_path);

    auto result = client->Get(resolved_path, headers);

    if (!result) {
        throw std::runtime_error("Failed to fetch from Reddit: HTTP error " +
            std::to_string(static_cast<int>(result.error())));
    }

    if (result->status >= 300 && result->status < 400) {
        auto loc_it = result->headers.find("Location");
        if (loc_it != result->headers.end()) {
            std::string new_path = loc_it->second;
            // Strip Reddit URL bases
            for (const auto& base : {REDDIT_URL_BASE, ALTERNATIVE_REDDIT_URL_BASE}) {
                if (new_path.starts_with(base)) {
                    new_path = new_path.substr(std::string(base).size());
                    break;
                }
            }
            // Add raw_json
            std::string sep = new_path.find('?') != std::string::npos ? "&" : "?";
            return reddit_json(new_path + sep + "raw_json=1", quarantine);
        }
    }

    if (result->status == 429) {
        throw std::runtime_error("Too many requests");
    }

    // Parse JSON
    try {
        json j = json::parse(result->body);

        // Check for Reddit errors
        if (j.contains("error") && j["error"].is_number()) {
            std::string reason = j.value("reason", "");
            std::string msg = j.value("message", "");
            if (msg == "Unauthorized") {
                force_refresh_token();
                throw std::runtime_error("OAuth token expired. Please refresh.");
            }
            if (reason == "quarantined") throw std::runtime_error("quarantined");
            if (reason == "gated") throw std::runtime_error("gated");
            if (reason == "private") throw std::runtime_error("private");
            if (reason == "banned") throw std::runtime_error("banned");
            throw std::runtime_error("Reddit error: " + msg);
        }

        // Check for user suspension
        if (j.contains("data") && j["data"].is_object()) {
            if (j["data"].value("is_suspended", false))
                throw std::runtime_error("suspended");
        }

        return j;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse JSON: " + std::string(e.what()));
    }
}

std::string canonical_path(const std::string& path, int tries) {
    if (tries == 0) return "";

    try {
        auto [client, resolved] = resolve_client(path);
        auto headers = build_headers(false, resolved);

        auto result = client->Head(resolved, headers);

        if (!result) return "";

        int status = result->status;

        if (status >= 200 && status < 300) return path;
        if (status == 301) {
            auto loc = result->headers.find("Location");
            if (loc != result->headers.end()) {
                std::string new_path = loc->second;
                // Strip .json and query params
                size_t json_pos = new_path.find(".json");
                if (json_pos != std::string::npos) new_path = new_path.substr(0, json_pos);
                size_t qpos = new_path.find('?');
                if (qpos != std::string::npos) new_path = new_path.substr(0, qpos);
                // Strip Reddit domain
                for (const auto& base : {REDDIT_URL_BASE, ALTERNATIVE_REDDIT_URL_BASE}) {
                    if (new_path.starts_with(base)) {
                        new_path = new_path.substr(std::string(base).size());
                        break;
                    }
                }
                return canonical_path(new_path, tries - 1);
            }
        }
        if (status == 429) throw std::runtime_error("Too many requests");
    } catch (...) {
        return "";
    }

    return "";
}

void init_client() {
    init_oauth();
}

void rate_limit_check() {
    try {
        reddit_json("/r/reddit/hot.json?raw_json=1", true);
    } catch (...) {}
}

std::string proxy(const std::string& path, const std::string& format_str) {
    std::string url = format_str;
    // Simplified - in real version, parse params from path
    try {
        std::string host;
        // Determine the target from the URL pattern
        if (url.find("v.redd.it") != std::string::npos) {
            host = "https://v.redd.it";
        } else if (url.find("i.redd.it") != std::string::npos) {
            host = "https://i.redd.it";
        } else if (url.find("thumbs.redditmedia.com") != std::string::npos) {
            host = "https://a.thumbs.redditmedia.com";
        } else if (url.find("emoji.redditmedia.com") != std::string::npos) {
            host = "https://emoji.redditmedia.com";
        } else if (url.find("styles.redditmedia.com") != std::string::npos) {
            host = "https://styles.redditmedia.com";
        } else if (url.find("redditstatic.com") != std::string::npos) {
            host = "https://www.redditstatic.com";
        } else {
            host = "https://i.redd.it";
        }

        httplib::Client cli(host);

        httplib::Headers headers;
        {
            std::lock_guard<std::mutex> lock(OAUTH_MUTEX);
            if (OAUTH_CLIENT) {
                headers.emplace("User-Agent", OAUTH_CLIENT->user_agent());
            }
        }
        headers.emplace("Accept", "*/*");

        auto result = cli.Get(url, headers);
        if (result) return result->body;
        return "";
    } catch (...) {
        return "";
    }
}

} // namespace pinklib
