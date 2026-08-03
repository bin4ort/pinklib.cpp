#include "server.h"
#include "utils.h"
#include <httplib.h>
#include <sstream>
#include <brotli/encode.h>

namespace pinklib {

static std::string content_type_for(const std::string& path) {
    auto pos = path.rfind('.');
    if (pos == std::string::npos) return "text/html; charset=utf-8";
    std::string ext = path.substr(pos);
    if (ext == ".css")  return "text/css";
    if (ext == ".js")   return "text/javascript";
    if (ext == ".png")  return "image/png";
    if (ext == ".ico")  return "image/vnd.microsoft.icon";
    if (ext == ".svg")  return "image/svg+xml";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".json") return "application/json";
    if (ext == ".xml")  return "application/opensearchdescription+xml";
    if (ext == ".rss" || ext == ".atom") return "application/rss+xml";
    if (ext == ".txt")  return "text/plain";
    if (ext == ".mp4")  return "video/mp4";
    return "text/html; charset=utf-8";
}

void Server::at(const std::string& path, RequestHandler handler) {
    routes.push_back({"GET", path, handler});
}

void Server::get(const std::string& path, RequestHandler handler) {
    routes.push_back({"GET", path, handler});
}

void Server::post(const std::string& path, RequestHandler handler) {
    routes.push_back({"POST", path, handler});
}

static bool match_route(const std::string& pattern, const std::string& path,
                        std::unordered_map<std::string, std::string>& params) {
    params.clear();
    std::vector<std::string> pat_parts, path_parts;

    {
        std::stringstream ss(pattern);
        std::string part;
        while (std::getline(ss, part, '/')) {
            if (!part.empty()) pat_parts.push_back(part);
        }
    }
    {
        std::stringstream ss(path);
        std::string part;
        while (std::getline(ss, part, '/')) {
            if (!part.empty()) path_parts.push_back(part);
        }
    }

    // If pattern ends with *, allow extra parts
    bool star = !pattern.empty() && pattern.back() == '*';

    size_t i = 0, j = 0;
    while (i < pat_parts.size() && j < path_parts.size()) {
        if (pat_parts[i] == "*") return true;
        if (pat_parts[i].starts_with(":")) {
            params[pat_parts[i].substr(1)] = path_parts[j];
            i++; j++;
        } else if (pat_parts[i] == path_parts[j]) {
            i++; j++;
        } else {
            return false;
        }
    }

    if (i < pat_parts.size()) {
        for (size_t k = i; k < pat_parts.size(); k++) {
            if (!pat_parts[k].starts_with(":")) return false;
        }
    }

    return (star && i == pat_parts.size()) || (i == pat_parts.size() && j == path_parts.size());
}

static std::string compress_brotli(const std::string& body) {
    size_t input_size = body.size();
    size_t out_size = BrotliEncoderMaxCompressedSize(input_size);
    std::vector<uint8_t> out(out_size);
    if (BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY, BROTLI_DEFAULT_WINDOW,
                              BROTLI_DEFAULT_MODE, input_size,
                              reinterpret_cast<const uint8_t*>(body.data()),
                              &out_size, out.data()) == BROTLI_TRUE) {
        return std::string(reinterpret_cast<char*>(out.data()), out_size);
    }
    return "";
}

static std::string compress_gzip(const std::string& body) {
    z_stream zs;
    zs.zalloc = Z_NULL;
    zs.zfree = Z_NULL;
    zs.opaque = Z_NULL;

    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY) != Z_OK)
        return "";

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(body.data()));
    zs.avail_in = body.size();

    std::vector<uint8_t> out(body.size() + 1024);
    zs.next_out = out.data();
    zs.avail_out = out.size();

    int ret = deflate(&zs, Z_FINISH);
    deflateEnd(&zs);

    if (ret == Z_STREAM_END) {
        return std::string(reinterpret_cast<char*>(out.data()),
                          out.size() - zs.avail_out);
    }
    return "";
}

void Server::listen(const std::string& address, int port) {
    httplib::Server svr;

    for (const auto& route : routes) {
        std::string pattern = route.path_pattern;

        if (route.method == "GET") {
            svr.Get(pattern, [this, route, pattern](const httplib::Request& req, httplib::Response& res) {
                std::unordered_map<std::string, std::string> params;
                match_route(pattern, req.path, params);

                std::string query = !req.params.empty() ?
                    req.target.substr(req.target.find('?') + 1) : "";

                auto cookies = parse_cookies(
                    req.has_header("Cookie") ? req.get_header_value("Cookie") : "");

                std::string body;
                try {
                    body = route.handler("GET", req.path, params, query, req.body, cookies);
                } catch (const std::exception& e) {
                    res.status = 500;
                    res.set_content(e.what(), "text/plain");
                    return;
                }

                std::string content_type = content_type_for(req.path);
                res.status = 200;
                res.set_content(body, content_type.c_str());
            });
        } else if (route.method == "POST") {
            svr.Post(pattern, [this, route, pattern](const httplib::Request& req, httplib::Response& res) {
                std::unordered_map<std::string, std::string> params;
                match_route(pattern, req.path, params);

                auto cookies = parse_cookies(
                    req.has_header("Cookie") ? req.get_header_value("Cookie") : "");

                std::string body;
                try {
                    body = route.handler("POST", req.path, params,
                        req.target.substr(req.target.find('?') + 1),
                        req.body, cookies);
                } catch (const std::exception& e) {
                    res.status = 500;
                    res.set_content(e.what(), "text/plain");
                    return;
                }

                res.status = 200;
                res.set_content(body, "text/html; charset=utf-8");
            });
        }
    }

    // Handle unmatched routes using the last registered (catch-all) route
    svr.set_mount_point("/", "");  // serve static

    std::cout << "Running PinkLib v0.36.0 on " << address << ":" << port << "!" << std::endl;
    svr.listen(address.c_str(), port);
}

std::string redirect(const std::string& path) {
    return "<html><head><meta http-equiv=\"refresh\" content=\"0;url=" + path +
           "\"></head><body>Redirecting to <a href=\"" + path + "\">" + path +
           "</a>...</body></html>";
}

std::string error_response(const std::string& msg, const Preferences& prefs, const std::string& url) {
    std::stringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "<title>Error - PinkLib</title>\n";
    html << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n";
    html << "</head>\n<body>\n";
    html << "<nav><div id=\"logo\">";
    html << "<a id=\"pinklib\" href=\"/\"><span id=\"pink\">pink</span><span id=\"lib\">lib.</span></a>";
    html << "</div></nav>\n";
    html << "<main><div id=\"error\">\n";
    html << "<h1>Error</h1>\n";
    html << "<p>" << msg << "</p>\n";
    html << "<p><a href=\"/\">Head back home?</a></p>\n";
    html << "</div></main>\n";
    html << "</body>\n</html>";
    return html.str();
}

std::string info_response(const std::string& msg, const Preferences& prefs, const std::string& url) {
    std::stringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "<title>Info - PinkLib</title>\n";
    html << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n";
    html << "</head>\n<body>\n";
    html << "<nav><div id=\"logo\">";
    html << "<a id=\"pinklib\" href=\"/\"><span id=\"pink\">pink</span><span id=\"lib\">lib.</span></a>";
    html << "</div>";
    html << "<div id=\"links\"><a href=\"/settings\">settings</a></div></nav>\n";
    html << "<main><div id=\"info\">\n";
    html << "<h1>" << msg << "</h1>\n";
    html << "<p>PinkLib v0.36.0</p>\n";
    html << "<p><a href=\"/\">Home</a></p>\n";
    html << "</div></main>\n";
    html << "</body>\n</html>";
    return html.str();
}

std::string nsfw_landing_response(const std::string& res, ResourceType res_type,
                                   const Preferences& prefs, const std::string& url) {
    std::stringstream html;
    html << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<title>NSFW - PinkLib</title>\n";
    html << "<link rel=\"stylesheet\" type=\"text/css\" href=\"/style.css\">\n";
    html << "</head>\n<body>\n";
    html << "<nav><div id=\"logo\">";
    html << "<a id=\"pinklib\" href=\"/\"><span id=\"pink\">pink</span><span id=\"lib\">lib.</span></a>";
    html << "</div></nav>\n";
    html << "<main><div id=\"nsfw\">\n";
    html << "<h1>NSFW Content</h1>\n";
    html << "<p>This content is marked NSFW. ";
    html << "Enable \"Show NSFW posts\" in <a href=\"/settings\">settings</a> to view.</p>\n";
    html << "<p><a href=\"/\">Home</a></p>\n";
    html << "</div></main>\n";
    html << "</body>\n</html>";
    return html.str();
}

} // namespace pinklib
