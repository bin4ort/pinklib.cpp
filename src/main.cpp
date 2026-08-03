#include "config.h"
#include "utils.h"
#include "client.h"
#include "server.h"
#include "handlers.h"
#include "subreddit.h"
#include "oauth.h"
#include "static_embedded.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <functional>

static std::string serve_static(const std::string& url_path) {
    const auto& s = pinklib::embedded_static();
    // Clean path (start with /)
    std::string p = url_path;
    if (!p.empty() && p[0] != '/') p = "/" + p;
    auto it = s.find(p);
    if (it != s.end()) return it->second.content;
    // Fallback: try filesystem
    std::string fpath = "static" + p;
    std::ifstream file(fpath, std::ios::binary);
    if (!file.is_open()) return "";
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// Type alias for handlers
using Handler = std::function<std::string(
    const std::string&, const std::string&,
    const std::unordered_map<std::string, std::string>&,
    const std::string&, const std::string&,
    const std::unordered_map<std::string, std::string>&)>;

static std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// Wrapper to create handler from a simpler fn
static Handler simple_handler(std::function<std::string(
    const std::string&, const std::string&,
    const std::unordered_map<std::string, std::string>&,
    const std::unordered_map<std::string, std::string>&)> fn) {
    return [fn](const std::string&, const std::string& path,
                 const std::unordered_map<std::string, std::string>& params,
                 const std::string& query, const std::string&,
                 const std::unordered_map<std::string, std::string>& cookies) {
        return fn(path, query, params, cookies);
    };
}

int main(int argc, char* argv[]) {
    std::ifstream env_file(".env");
    if (env_file.is_open()) {
        std::string line;
        while (std::getline(env_file, line)) {
            auto eqpos = line.find('=');
            if (eqpos != std::string::npos && !line.starts_with("#")) {
                setenv(line.substr(0, eqpos).c_str(), line.substr(eqpos + 1).c_str(), 0);
            }
        }
    }

    pinklib::init_config();
    std::cout << "Initializing OAuth client..." << std::endl;
    pinklib::init_oauth();
    pinklib::init_client();

    std::string address = "127.0.0.1";
    std::string port = "8095";
    bool ipv4_only = false, ipv6_only = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-a" || arg == "--address") && i + 1 < argc) address = argv[++i];
        else if ((arg == "-p" || arg == "--port") && i + 1 < argc) port = argv[++i];
        else if (arg == "-4" || arg == "--ipv4-only") ipv4_only = true;
        else if (arg == "-6" || arg == "--ipv6-only") ipv6_only = true;
        else if (arg == "-h" || arg == "--help") {
            std::cout << "PinkLib v0.36.0 - Private front-end for Reddit\n";
            return 0;
        }
    }

    const char* ep = std::getenv("PORT");
    if (ep) port = ep;
    if (std::getenv("IPV4_ONLY")) ipv4_only = true;
    if (std::getenv("IPV6_ONLY")) ipv6_only = true;

    std::string listener = ipv4_only ? "127.0.0.1" : ipv6_only ? "::1" : address;
    std::cout << "Starting PinkLib..." << std::endl;

    pinklib::Server app;

    // Static routes - using lambdas with correct signature
    app.at("/style.css", [](const std::string&, const std::string&, const auto&, const auto&, const auto&, const auto&) {
        return read_file("static/style.css");
    });

    app.at("/favicon.ico", [](const std::string&, const std::string&, const auto&, const auto&, const auto&, const auto&) {
        return read_file("static/favicon.ico");
    });

    app.at("/robots.txt", [](const std::string&, const std::string&, const auto&, const auto&, const auto&, const auto&) {
        auto d = pinklib::get_setting("REDLIB_ROBOTS_DISABLE_INDEXING");
        return (d && *d == "on") ? "User-agent: *\nDisallow: /\n" : "User-agent: *\nDisallow: /u/\nDisallow: /user/\n";
    });

    // Core routes as simple handlers
    app.at("/*", [](const std::string& method, const std::string& req_path,
        const std::unordered_map<std::string, std::string>& params,
        const std::string& query, const std::string& body,
        const std::unordered_map<std::string, std::string>& cookies) -> std::string {

    // Dispatch based on path patterns
    if (req_path == "/" || req_path == "/best" || req_path == "/hot" || req_path == "/new" ||
        req_path == "/top" || req_path == "/rising" || req_path == "/controversial") {
        return pinklib::subreddit_community(req_path, query, params, cookies);
    }

    // r/ routes
    if (req_path.starts_with("/r/")) {
        std::string sub_path = req_path.substr(2);
        if (sub_path.ends_with(".rss")) {
            return pinklib::subreddit_rss(req_path, query, params, cookies);
        }
        if (sub_path.find("/comments/") != std::string::npos) {
            return pinklib::post_item(req_path, query, params, cookies);
        }
        if (sub_path.find("/duplicates/") != std::string::npos) {
            return pinklib::duplicates_item(req_path, query, params, cookies);
        }
        if (sub_path.find("/wiki") != std::string::npos || sub_path.find("/w/") != std::string::npos) {
            return pinklib::subreddit_wiki(req_path, query, params, cookies);
        }
        if (sub_path.find("/about/sidebar") != std::string::npos) {
            return pinklib::subreddit_sidebar(req_path, query, params, cookies);
        }
        if (sub_path.find("/search") != std::string::npos) {
            return pinklib::search_find(req_path, query, params, cookies);
        }
        return pinklib::subreddit_community(req_path, query, params, cookies);
    }

    // u/ redirects
    if (req_path.starts_with("/u/")) {
        std::string name = req_path.substr(3);
        return pinklib::redirect("/user/" + name);
    }

    // user/ routes
    if (req_path.starts_with("/user/")) {
        if (req_path.ends_with(".rss")) {
            return pinklib::user_rss(req_path, query, params, cookies);
        }
        if (req_path.find("/comments/") != std::string::npos) {
            return pinklib::post_item(req_path, query, params, cookies);
        }
        return pinklib::user_profile(req_path, query, params, cookies);
    }

    // comments routes
    if (req_path.starts_with("/comments/")) {
        return pinklib::post_item(req_path, query, params, cookies);
    }

    if (req_path.starts_with("/duplicates/")) {
        return pinklib::duplicates_item(req_path, query, params, cookies);
    }

    if (req_path == "/search" || req_path.starts_with("/search?")) {
        return pinklib::search_find(req_path, query, params, cookies);
    }

    if (req_path == "/settings") {
        if (method == "POST") {
            std::unordered_map<std::string, std::string> set_cookies;
            return pinklib::settings_set(req_path, query, body, params, cookies, set_cookies);
        }
        return pinklib::settings_get(req_path, query, params, cookies);
    }

    if (req_path == "/settings/restore") {
        std::unordered_map<std::string, std::string> set_cookies;
        return pinklib::settings_restore(req_path, query, params, cookies, set_cookies);
    }

    if (req_path == "/settings/update") {
        std::unordered_map<std::string, std::string> set_cookies;
        return pinklib::settings_update(req_path, query, params, cookies, set_cookies);
    }

    if (req_path == "/settings/encoded-restore" && method == "POST") {
        auto prefs = pinklib::Preferences::from_cookies(cookies);
        auto url = "/settings/restore/?" + prefs.to_urlencoded();
        return pinklib::redirect(url);
    }

    // Wiki
    if (req_path.starts_with("/wiki") || req_path.starts_with("/w/") || req_path == "/w") {
        return pinklib::subreddit_wiki(req_path, query, params, cookies);
    }

    if (req_path == "/info" || req_path.starts_with("/info.")) {
        return pinklib::instance_info_page(req_path, query, params, cookies);
    }

    if (req_path == "/about") {
        return pinklib::error_response("About pages aren't added yet",
            pinklib::Preferences::from_cookies(cookies), req_path);
    }

    // Proxy routes
    if (req_path.starts_with("/vid/")) return "";
    if (req_path.starts_with("/img/")) return "";
    if (req_path.starts_with("/thumb/")) return "";
    if (req_path.starts_with("/emoji/")) return "";
    if (req_path.starts_with("/preview/")) return "";
    if (req_path.starts_with("/style/")) return "";
    if (req_path.starts_with("/static/")) return "";
    if (req_path.starts_with("/emote/")) return "";
    if (req_path.starts_with("/hls/")) return "";

    // Serve static assets (embedded or from disk)
    std::string static_body = serve_static(req_path);
    if (!static_body.empty()) return static_body;

    // 404
    return pinklib::error_response("Nothing here",
        pinklib::Preferences::from_cookies(cookies), req_path);
    });

    app.listen(listener, std::stoi(port));
    return 0;
}
