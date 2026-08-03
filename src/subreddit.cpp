#include "subreddit.h"
#include "client.h"
#include "config.h"
#include "server.h"
#include <regex>
#include <sstream>
#include <iostream>

namespace pinklib {

static std::string post_sort(const std::unordered_map<std::string, std::string>& cookies,
                              const std::string& override_val);

static std::string quarantine_wall(const std::string& sub, const std::string& restriction,
                                    const std::string& url, const Preferences& prefs) {
    json data;
    data["title"] = "r/" + sub + " is " + restriction;
    data["sub"] = sub;
    data["msg"] = "Please click the button below to continue to this subreddit.";
    data["url"] = url;
    data["prefs"] = prefs.to_json();
    return render_template("wall.html", data);
}

bool can_access_quarantine(const std::unordered_map<std::string, std::string>& cookies,
                           const std::string& sub) {
    std::string key = "allow_quaran_";
    for (char c : sub) key += std::tolower(c);
    auto it = cookies.find(key);
    return it != cookies.end() && it->second == "true";
}

SubredditData fetch_subreddit(const std::string& sub, bool quarantined) {
    SubredditData sd;
    try {
        std::string path = "/r/" + sub + "/about.json?raw_json=1";
        auto res = reddit_json(path, quarantined);

        sd.name = val(res, "display_name");
        sd.title = val(res, "title");
        sd.description = val(res, "public_description");
        sd.info = rewrite_urls(val(res, "description_html"));
        int64_t members = res["data"].value("subscribers", 0ULL);
        int64_t active = res["data"].value("accounts_active", 0ULL);
        sd.members = format_num(members);
        sd.active = format_num(active);
        std::string community_icon = res["data"].value("community_icon", "");
        sd.icon = !community_icon.empty() ? format_url(community_icon) : format_url(val(res, "icon_img"));
        sd.wiki = res["data"].value("wiki_enabled", false);
        sd.nsfw = res["data"].value("over18", false);
    } catch (...) {}
    return sd;
}

std::string catch_random_sub(const std::string& sub, const std::unordered_map<std::string, std::string>& cookies) {
    if (sub == "random" || sub == "randnsfw") {
        try {
            std::string path = "/r/" + sub + "/about.json?raw_json=1";
            auto res = reddit_json(path, false);
            std::string display = res["data"].value("display_name", "");
            if (!display.empty()) return "/r/" + display;
        } catch (...) {}
    }
    return "";
}

static std::string build_template(const std::string& template_name, const json& data) {
    return render_template(template_name, data);
}

// ---- Community handler ----
std::string subreddit_community(const std::string& path,
                                 const std::string& query_string,
                                 const std::unordered_map<std::string, std::string>& params,
                                 const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);

    std::string sub_name;
    auto sit = params.find("sub");
    auto idit = params.find("id");
    auto sortit = params.find("sort");

    if (sit != params.end()) {
        sub_name = sit->second;
    } else if (idit != params.end()) {
        sub_name = idit->second;
    } else {
        // Front page
        std::string front_page = prefs.front_page;
        std::string subscribed = prefs.subscriptions.empty() ? "" : prefs.subscriptions[0];
        if (front_page == "default" || front_page.empty()) {
            sub_name = subscribed.empty() ? "popular" : subscribed;
        } else {
            sub_name = front_page;
        }
    }

    bool quarantined = can_access_quarantine(cookies, sub_name) || path == "/";

    // Handle random
    std::string random_redirect = catch_random_sub(sub_name, cookies);
    if (!random_redirect.empty()) return redirect(random_redirect);

    // Redirect u_ to user
    if (sub_name.starts_with("u_")) {
        return redirect("/user/" + sub_name.substr(2));
    }

    // Fetch subreddit data
    SubredditData sub;
    bool is_multireddit = sub_name.find('+') != std::string::npos ||
                           sub_name == "popular" || sub_name == "all";
    if (!is_multireddit) {
        sub = fetch_subreddit(sub_name, quarantined);
    } else {
        sub.name = sub_name;
    }

    // NSFW check
    std::string req_url = path;
    if (!query_string.empty()) req_url += "?" + query_string;

    if (sub.nsfw && should_be_nsfw_gated(prefs.show_nsfw)) {
        return nsfw_landing_response(sub_name, ResourceType::Subreddit, prefs, req_url);
    }

    // Build path for Reddit API
    std::string sort = post_sort(cookies, sortit != params.end() ? sortit->second : "");
    std::string api_path = "/r/" + sub_name + "/" + sort + ".json?" + query_string + "&raw_json=1";

    // Filter check
    auto filters_set = std::unordered_set<std::string>(prefs.filters.begin(), prefs.filters.end());
    if (filters_set.count(sub_name) > 0) {
        json data;
        data["sub"] = json::object();
        data["sub"]["name"] = sub_name;
        data["posts"] = json::array();
        data["sort"] = json::array({sort, ""});
        data["ends"] = json::array({"", ""});
        data["prefs"] = prefs.to_json();
        data["url"] = req_url;
        data["redirect_url"] = "";
        data["is_filtered"] = true;
        data["all_posts_filtered"] = false;
        data["all_posts_hidden_nsfw"] = false;
        data["no_posts"] = false;
        return build_template("subreddit.html", data);
    }

    // Fetch posts
    try {
        auto res = reddit_json(api_path, quarantined);
        auto& data_node = res["data"];
        auto children = data_node["children"];
        std::cerr << "[DEBUG] children is_array: " << children.is_array() << " size: " << children.size() << std::endl;

        std::vector<Post> posts;
        int idx = 0;
        for (const auto& child : children) {
            try {
                posts.push_back(parse_post(child));
            } catch (const std::exception& e) {
                std::cerr << "[DEBUG] parse_post failed for child " << idx << ": " << e.what() << std::endl;
                throw;
            }
            idx++;
        }
        std::cerr << "[DEBUG] posts size: " << posts.size() << std::endl;

        std::string after = res["data"].value("after", "");

        uint64_t num_filtered = 0;
        bool all_filtered = false;
        filter_posts(posts, filters_set, num_filtered, all_filtered);

        bool no_posts = posts.empty();
        bool all_nsfw = !no_posts;
        for (const auto& p : posts) {
            if (!p.flags.nsfw) { all_nsfw = false; break; }
        }
        all_nsfw = all_nsfw && prefs.show_nsfw != "on";

        json data;
        data["sub"] = json::object();
        data["sub"]["name"] = sub.name;
        data["sub"]["title"] = sub.title;
        data["sub"]["description"] = sub.description;
        data["sub"]["info"] = sub.info;
        data["sub"]["icon"] = sub.icon;
        data["sub"]["members"] = json::array({sub.members.first, sub.members.second});
        data["sub"]["active"] = json::array({sub.active.first, sub.active.second});
        data["sub"]["wiki"] = sub.wiki;
        data["sub"]["nsfw"] = sub.nsfw;

        json posts_arr = json::array();
        for (const auto& p : posts) {
            json pj;
            pj["id"] = p.id;
            pj["title"] = p.title;
            pj["community"] = p.community;
            pj["body"] = p.body;
            pj["author"]["name"] = p.author.name;
            pj["permalink"] = p.permalink;
            pj["post_type"] = p.post_type;
            pj["score"] = json::array({p.score.first, p.score.second});
            pj["comments"] = json::array({p.comments.first, p.comments.second});
            pj["rel_time"] = p.rel_time;
            pj["created"] = p.created;
            pj["flags"]["nsfw"] = p.flags.nsfw;
            pj["flags"]["spoiler"] = p.flags.spoiler;
            pj["flags"]["stickied"] = p.flags.stickied;
            pj["flair"]["text"] = p.flair.text;
            pj["flair"]["background_color"] = p.flair.background_color;
            pj["flair"]["foreground_color"] = p.flair.foreground_color;
            pj["domain"] = p.domain;
            pj["media"]["url"] = p.media.url;
            pj["media"]["alt_url"] = p.media.alt_url;
            pj["media"]["width"] = p.media.width;
            pj["media"]["height"] = p.media.height;
            pj["media"]["poster"] = p.media.poster;
            pj["thumbnail"]["url"] = p.thumbnail.url;
            pj["thumbnail"]["width"] = p.thumbnail.width;
            pj["thumbnail"]["height"] = p.thumbnail.height;
            pj["gallery"] = json::array();
            for (const auto& g : p.gallery) {
                json gj;
                gj["url"] = g.url;
                gj["width"] = g.width;
                gj["height"] = g.height;
                gj["caption"] = g.caption;
                pj["gallery"].push_back(gj);
            }
            pj["out_url"] = p.out_url.value_or("");
            pj["nsfw"] = p.nsfw;
            posts_arr.push_back(pj);
        }

        data["posts"] = posts_arr;
        data["sort"] = json::array({sort, ""});
        data["ends"] = json::array({"", after});
        data["prefs"] = prefs.to_json();
        data["url"] = req_url;
        data["redirect_url"] = "";
        data["is_filtered"] = false;
        data["all_posts_filtered"] = all_filtered;
        data["all_posts_hidden_nsfw"] = all_nsfw;
        data["no_posts"] = no_posts;

        std::cerr << "[DEBUG] About to render template" << std::endl;
        auto result = build_template("subreddit.html", data);
        std::cerr << "[DEBUG] Render complete, length: " << result.size() << std::endl;
        return result;
    } catch (const std::exception& e) {
        std::cerr << "[DEBUG] Exception in community handler: " << e.what() << std::endl;
        std::string msg = e.what();
        if (msg == "quarantined" || msg == "gated") {
            return quarantine_wall(sub_name, msg, req_url, prefs);
        }
        if (msg == "private") return error_response("r/" + sub_name + " is a private community", prefs, req_url);
        if (msg == "banned") return error_response("r/" + sub_name + " has been banned from Reddit", prefs, req_url);
        return error_response(msg, prefs, req_url);
    }
}

// ---- Wiki handler ----
std::string subreddit_wiki(const std::string& path,
                            const std::string& query_string,
                            const std::unordered_map<std::string, std::string>& params,
                            const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);
    std::string sub;
    auto sit = params.find("sub");
    if (sit != params.end()) sub = sit->second;
    else sub = "reddit.com";

    bool quarantined = can_access_quarantine(cookies, sub);

    std::string page = "index";
    auto pit = params.find("page");
    if (pit != params.end()) page = pit->second;

    std::string req_url = path;
    if (!query_string.empty()) req_url += "?" + query_string;

    try {
        std::string api_path = "/r/" + sub + "/wiki/" + page + ".json?raw_json=1";
        auto res = reddit_json(api_path, quarantined);

        json data;
        data["sub"] = sub;
        data["wiki"] = rewrite_urls(res["data"].value("content_html", "<h3>Wiki not found</h3>"));
        data["page"] = page;
        data["prefs"] = prefs.to_json();
        data["url"] = req_url;
        return build_template("wiki.html", data);
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg == "quarantined" || msg == "gated") {
            return quarantine_wall(sub, msg, req_url, prefs);
        }
        return error_response(msg, prefs, req_url);
    }
}

// ---- Sidebar handler ----
std::string subreddit_sidebar(const std::string& path,
                               const std::string& query_string,
                               const std::unordered_map<std::string, std::string>& params,
                               const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);
    std::string sub;
    auto sit = params.find("sub");
    if (sit != params.end()) sub = sit->second;
    else sub = "reddit.com";

    bool quarantined = can_access_quarantine(cookies, sub);

    std::string req_url = path;
    if (!query_string.empty()) req_url += "?" + query_string;

    try {
        std::string api_path = "/r/" + sub + "/about.json?raw_json=1";
        auto res = reddit_json(api_path, quarantined);

        json data;
        data["wiki"] = rewrite_urls(val(res, "description_html"));
        data["sub"] = sub;
        data["page"] = "Sidebar";
        data["prefs"] = prefs.to_json();
        data["url"] = req_url;
        return build_template("wiki.html", data);
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg == "quarantined" || msg == "gated") {
            return quarantine_wall(sub, msg, req_url, prefs);
        }
        return error_response(msg, prefs, req_url);
    }
}

// ---- Subscriptions/Filters handler ----
std::string subreddit_subscriptions_filters(const std::string& method,
                                             const std::string& path,
                                             const std::string& query_string,
                                             const std::unordered_map<std::string, std::string>& params,
                                             const std::unordered_map<std::string, std::string>& cookies,
                                             std::unordered_map<std::string, std::string>& set_cookies) {
    std::string sub;
    auto sit = params.find("sub");
    if (sit != params.end()) sub = sit->second;

    Preferences prefs = Preferences::from_cookies(cookies);

    // Determine action
    bool is_subscribe = path.find("/subscribe") != std::string::npos;
    bool is_unsubscribe = path.find("/unsubscribe") != std::string::npos;
    bool is_filter = path.find("/filter") != std::string::npos;
    bool is_unfilter = path.find("/unfilter") != std::string::npos;

    if (is_subscribe || is_unsubscribe) {
        auto& sub_list = prefs.subscriptions;
        if (is_subscribe) {
            if (std::find(sub_list.begin(), sub_list.end(), sub) == sub_list.end())
                sub_list.push_back(sub);
        } else {
            sub_list.erase(std::remove(sub_list.begin(), sub_list.end(), sub), sub_list.end());
        }
        // Store in cookie
        std::string combined;
        for (size_t i = 0; i < sub_list.size(); i++) {
            if (i > 0) combined += "+";
            combined += sub_list[i];
        }
        set_cookies["subscriptions"] = combined;
    }

    if (is_filter || is_unfilter) {
        auto& filter_list = prefs.filters;
        if (is_filter) {
            if (std::find(filter_list.begin(), filter_list.end(), sub) == filter_list.end())
                filter_list.push_back(sub);
        } else {
            filter_list.erase(std::remove(filter_list.begin(), filter_list.end(), sub), filter_list.end());
        }
        std::string combined;
        for (size_t i = 0; i < filter_list.size(); i++) {
            if (i > 0) combined += "+";
            combined += filter_list[i];
        }
        set_cookies["filters"] = combined;
    }

    return redirect("/r/" + sub);
}

// ---- Quarantine exception handler ----
std::string subreddit_add_quarantine(const std::string& path,
                                      const std::string& query_string,
                                      const std::unordered_map<std::string, std::string>& params,
                                      std::unordered_map<std::string, std::string>& set_cookies) {
    std::string sub;
    auto sit = params.find("sub");
    if (sit != params.end()) sub = sit->second;

    // Parse redirect from query
    std::string redir;
    if (!query_string.empty()) {
        auto redir_val = param("?" + query_string, "redir");
        if (redir_val) redir = *redir_val;
    }

    std::string key = "allow_quaran_";
    for (char c : sub) key += std::tolower(c);
    set_cookies[key] = "true";

    return redirect(redir.empty() ? "/" : redir);
}

static std::string post_sort(const std::unordered_map<std::string, std::string>& cookies,
                              const std::string& override_val) {
    if (!override_val.empty()) return override_val;
    auto it = cookies.find("post_sort");
    if (it != cookies.end() && !it->second.empty()) return it->second;
    auto def = get_setting("REDLIB_DEFAULT_POST_SORT");
    if (def && !def->empty()) return *def;
    return "hot";
}

// ---- RSS handler ----
std::string subreddit_rss(const std::string& path,
                           const std::string& query_string,
                           const std::unordered_map<std::string, std::string>& params,
                           const std::unordered_map<std::string, std::string>& cookies) {
    if (!enable_rss()) {
        Preferences prefs = Preferences::from_cookies(cookies);
        return error_response("RSS is disabled on this instance.", prefs, path);
    }

    std::string sub;
    auto sit = params.find("sub");
    if (sit != params.end()) sub = sit->second;

    std::string sort = post_sort(cookies, "");
    std::string api_path = "/r/" + sub + "/" + sort + ".json?" + query_string;

    // Fetch subreddit data and posts
    try {
        auto sub_data = fetch_subreddit(sub, false);
        auto [posts, after] = Post::fetch(api_path, false);

        std::stringstream rss;
        rss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        rss << "<rss version=\"2.0\">\n<channel>\n";
        rss << "<title>" << sub_data.title << "</title>\n";
        rss << "<description>" << sub_data.description << "</description>\n";
        rss << "<link>" << to_absolute_url("/r/" + sub) << "</link>\n";

        for (const auto& post : posts) {
            rss << "<item>\n";
            rss << "<title>" << post.title << "</title>\n";
            rss << "<link>" << format_url(get_post_url(post)) << "</link>\n";
            rss << "<author>" << post.author.name << "</author>\n";
            rss << "<description>" << rewrite_urls(post.body) << "</description>\n";
            rss << "<comments>" << to_absolute_url(post.permalink) << "</comments>\n";
            rss << "<pubDate>" << post.created << "</pubDate>\n";
            rss << "</item>\n";
        }

        rss << "</channel>\n</rss>";
        return rss.str();
    } catch (const std::exception& e) {
        Preferences prefs = Preferences::from_cookies(cookies);
        return error_response(e.what(), prefs, path);
    }
}

} // namespace pinklib
