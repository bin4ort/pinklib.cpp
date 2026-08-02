#pragma once

#include "utils.h"
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

namespace pinklib {

using RequestHandler = std::function<std::string(
    const std::string& method,
    const std::string& path,
    const std::unordered_map<std::string, std::string>& params,
    const std::string& query_string,
    const std::string& body,
    const std::unordered_map<std::string, std::string>& cookies)>;

struct Route {
    std::string method;
    std::string path_pattern;
    RequestHandler handler;
};

struct Server {
    std::unordered_map<std::string, std::string> default_headers;
    std::vector<Route> routes;

    void at(const std::string& path, RequestHandler handler);
    void get(const std::string& path, RequestHandler handler);
    void post(const std::string& path, RequestHandler handler);
    void listen(const std::string& address, int port);
};

std::string redirect(const std::string& path);
std::string error_response(const std::string& msg, const Preferences& prefs, const std::string& url);
std::string info_response(const std::string& msg, const Preferences& prefs, const std::string& url);
std::string nsfw_landing_response(const std::string& res, ResourceType res_type,
                                   const Preferences& prefs, const std::string& url);

} // namespace pinklib
