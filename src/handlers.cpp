#include "handlers.h"
#include "config.h"
#include "client.h"
#include "server.h"
#include "subreddit.h"
#include <sstream>

namespace pinklib {

// ---- Post ----
std::string post_item(const std::string& path,
                       const std::string& query_string,
                       const std::unordered_map<std::string, std::string>& params,
                       const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);

    std::string sub;
    auto sit = params.find("sub");
    if (sit != params.end()) sub = sit->second;

    std::string id;
    auto iit = params.find("id");
    if (iit != params.end()) id = iit->second;

    std::string req_url = path;
    if (!query_string.empty()) req_url += "?" + query_string;

    std::string sort;
    auto sortit = params.find("sort");
    if (sortit != params.end()) sort = sortit->second;
    if (sort.empty()) sort = get_cookie_val(cookies, "comment_sort");

    bool quarantined = (sit != params.end()) ? can_access_quarantine(cookies, sub) : false;

    std::string api_path = path + ".json?" + query_string + "&raw_json=1";
    if (!sort.empty()) api_path += "&sort=" + sort;

    try {
        auto res = reddit_json(api_path, quarantined);
        auto& data_children = res[0]["data"]["children"];
        if (data_children.empty()) return error_response("Post not found", prefs, req_url);

        auto post = parse_post(data_children[0]);

        if (post.nsfw && should_be_nsfw_gated(prefs.show_nsfw)) {
            return nsfw_landing_response(id, ResourceType::Post, prefs, req_url);
        }

        // Parse comments
        std::vector<Comment> comments;
        auto parse_comments = [&](const json& json_comments, const std::string& post_link,
                                   const std::string& post_author, auto& recurse) -> std::vector<Comment> {
            std::vector<Comment> result;
            auto children = json_comments["data"]["children"];
            if (!children.is_array()) return result;

            for (const auto& comment_json : children) {
                Comment c;
                auto& data = comment_json["data"];
                c.id = val(comment_json, "id");
                c.kind = comment_json.value("kind", "");
                c.body = rewrite_emotes(data.value("media_metadata", json::object()),
                                        val(comment_json, "body_html"));
                c.author.name = val(comment_json, "author");
                c.author.distinguished = val(comment_json, "distinguished");
                auto [rel_t, created_t] = time_str(data.value("created_utc", 0.0));
                c.rel_time = rel_t;
                c.created = created_t;
                int64_t sc = data.value("score", 0);
                c.score = format_num(sc);
                c.post_link = post_link;
                c.post_author = post_author;
                c.prefs = prefs;

                if (data.contains("replies") && data["replies"].is_object()) {
                    c.replies = recurse(data["replies"], post_link, post_author, recurse);
                }

                bool is_mod = data.value("distinguished", "") == "moderator";
                bool is_sticky = data.value("stickied", false);
                c.collapsed = (is_mod && is_sticky);

                result.push_back(c);
            }

            return result;
        };

        comments = parse_comments(res[1], post.permalink, post.author.name, parse_comments);

        json data;
        data["post"] = json::object();
        data["post"]["id"] = post.id;
        data["post"]["title"] = post.title;
        data["post"]["community"] = post.community;
        data["post"]["body"] = post.body;
        data["post"]["author"] = json::object();
        data["post"]["author"]["name"] = post.author.name;
        data["post"]["permalink"] = post.permalink;
        data["post"]["post_type"] = post.post_type;
        data["post"]["score"] = json::array({post.score.first, post.score.second});
        data["post"]["comments"] = json::array({post.comments.first, post.comments.second});
        data["post"]["rel_time"] = post.rel_time;
        data["post"]["created"] = post.created;
        data["post"]["upvote_ratio"] = post.upvote_ratio;
        data["post"]["media"]["url"] = post.media.url;
        data["post"]["media"]["alt_url"] = post.media.alt_url;
        data["post"]["media"]["width"] = post.media.width;
        data["post"]["media"]["height"] = post.media.height;
        data["post"]["media"]["poster"] = post.media.poster;
        data["post"]["domain"] = post.domain;
        data["post"]["nsfw"] = post.nsfw;
        data["post"]["poll"] = post.poll.has_value() ? json::object() : json();

        json comments_arr = json::array();
        for (const auto& c : comments) {
            json cj;
            cj["id"] = c.id;
            cj["kind"] = c.kind;
            cj["body"] = c.body;
            cj["author"]["name"] = c.author.name;
            cj["score"] = json::array({c.score.first, c.score.second});
            cj["rel_time"] = c.rel_time;
            cj["created"] = c.created;
            cj["collapsed"] = c.collapsed;
            cj["replies"] = json::array();
            comments_arr.push_back(cj);
        }
        data["comments"] = comments_arr;

        data["sort"] = sort;
        data["prefs"] = prefs.to_json();
        data["single_thread"] = false;
        data["url"] = req_url;
        data["url_without_query"] = path;
        data["comment_query"] = "";

        return render_template("post.html", data);
    } catch (const std::exception& e) {
        std::string msg = e.what();
        if (msg == "quarantined" || msg == "gated") {
            return redirect("/r/" + sub);
        }
        return error_response(msg, prefs, req_url);
    }
}

// ---- User ----
std::string user_profile(const std::string& path,
                          const std::string& query_string,
                          const std::unordered_map<std::string, std::string>& params,
                          const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);

    std::string name;
    auto nit = params.find("name");
    if (nit != params.end()) name = nit->second;
    else name = "reddit";

    std::string listing = "overview";
    auto lit = params.find("listing");
    if (lit != params.end()) listing = lit->second;

    std::string req_url = path;
    if (!query_string.empty()) req_url += "?" + query_string;

    std::string api_path = "/user/" + name + "/" + listing + ".json?" + query_string + "&raw_json=1";

    try {
        // Fetch user info
        UserData user;
        try {
            auto user_res = reddit_json("/user/" + name + "/about.json?raw_json=1", false);
            user.name = user_res["data"].value("name", name);
            user.title = user_res["data"]["subreddit"].value("title", "");
            user.karma = user_res["data"].value("total_karma", 0);
            user.nsfw = user_res["data"]["subreddit"].value("over_18", false);
        } catch (...) {
            user.name = name;
        }

        if (user.nsfw && should_be_nsfw_gated(prefs.show_nsfw)) {
            return nsfw_landing_response(name, ResourceType::User, prefs, req_url);
        }

        auto res = reddit_json(api_path, false);
        auto children = res["data"]["children"];

        std::vector<Post> posts;
        for (const auto& child : children) {
            posts.push_back(parse_post(child));
        }

        std::string after = res["data"].value("after", "");

        auto filters_set = std::unordered_set<std::string>(prefs.filters.begin(), prefs.filters.end());
        uint64_t num_filtered = 0;
        bool all_filtered = false;
        filter_posts(posts, filters_set, num_filtered, all_filtered);

        json data;
        data["user"]["name"] = user.name;
        data["user"]["title"] = user.title;
        data["user"]["karma"] = user.karma;
        data["user"]["nsfw"] = user.nsfw;

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
            pj["domain"] = p.domain;
            pj["media"]["url"] = p.media.url;
            pj["thumbnail"]["url"] = p.thumbnail.url;
            posts_arr.push_back(pj);
        }

        data["posts"] = posts_arr;
        data["sort"] = json::array({"", ""});
        data["ends"] = json::array({"", after});
        data["listing"] = listing;
        data["prefs"] = prefs.to_json();
        data["url"] = req_url;
        data["redirect_url"] = "";
        data["is_filtered"] = false;
        data["all_posts_filtered"] = all_filtered;
        data["all_posts_hidden_nsfw"] = false;
        data["no_posts"] = posts.empty();

        return render_template("user.html", data);
    } catch (const std::exception& e) {
        return error_response(e.what(), prefs, req_url);
    }
}

std::string user_rss(const std::string& path,
                      const std::string& query_string,
                      const std::unordered_map<std::string, std::string>& params,
                      const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);

    if (!enable_rss()) {
        return error_response("RSS is disabled on this instance.", prefs, path);
    }

    std::string name;
    auto nit = params.find("name");
    if (nit != params.end()) name = nit->second;

    std::string listing = "overview";
    auto lit = params.find("listing");
    if (lit != params.end()) listing = lit->second;

    try {
        auto [posts, after] = Post::fetch("/user/" + name + "/" + listing + ".json?" + query_string + "&raw_json=1", false);

        std::stringstream rss;
        rss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        rss << "<rss version=\"2.0\">\n<channel>\n";
        rss << "<title>" << name << "</title>\n";
        rss << "<link>" << to_absolute_url("/user/" + name) << "</link>\n";

        for (const auto& post : posts) {
            rss << "<item>\n";
            rss << "<title>" << post.title << "</title>\n";
            rss << "<link>" << format_url(get_post_url(post)) << "</link>\n";
            rss << "<author>" << post.author.name << "</author>\n";
            rss << "<description>" << rewrite_urls(post.body) << "</description>\n";
            rss << "<pubDate>" << post.created << "</pubDate>\n";
            rss << "</item>\n";
        }

        rss << "</channel>\n</rss>";
        return rss.str();
    } catch (const std::exception& e) {
        return error_response(e.what(), prefs, path);
    }
}

// ---- Search ----
std::string search_find(const std::string& path,
                         const std::string& query_string,
                         const std::unordered_map<std::string, std::string>& params,
                         const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);

    std::string api_path = path + ".json?" + query_string + "&raw_json=1";
    auto q_val = param(api_path, "q");
    if (!q_val || q_val->empty()) return redirect("/");

    std::string req_url = path;
    if (!query_string.empty()) req_url += "?" + query_string;

    try {
        auto res = reddit_json(api_path, false);
        auto children = res["data"]["children"];

        std::vector<Post> posts;
        for (const auto& child : children) {
            posts.push_back(parse_post(child));
        }

        std::string after = res["data"].value("after", "");

        json data;
        data["posts"] = json::array();
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
            pj["domain"] = p.domain;
            pj["media"]["url"] = p.media.url;
            pj["thumbnail"]["url"] = p.thumbnail.url;
            data["posts"].push_back(pj);
        }

        data["subreddits"] = json::array();
        data["sub"] = "";
        data["params"] = json::object();
        data["params"]["q"] = q_val.value_or("");
        data["params"]["sort"] = "";
        data["params"]["t"] = "";
        data["params"]["after"] = after;
        data["params"]["before"] = "";
        data["prefs"] = prefs.to_json();
        data["url"] = req_url;
        data["is_filtered"] = false;
        data["all_posts_filtered"] = false;
        data["all_posts_hidden_nsfw"] = false;
        data["no_posts"] = posts.empty();

        return render_template("search.html", data);
    } catch (const std::exception& e) {
        return error_response(e.what(), prefs, req_url);
    }
}

// ---- Settings ----
std::string settings_get(const std::string& path,
                          const std::string& query_string,
                          const std::unordered_map<std::string, std::string>& params,
                          const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);

    json data;
    data["prefs"] = prefs.to_json();
    data["prefs"]["theme"] = prefs.theme;
    data["prefs"]["front_page"] = prefs.front_page;
    data["prefs"]["layout"] = prefs.layout;
    data["prefs"]["wide"] = prefs.wide;
    data["prefs"]["blur_spoiler"] = prefs.blur_spoiler;
    data["prefs"]["show_nsfw"] = prefs.show_nsfw;
    data["prefs"]["blur_nsfw"] = prefs.blur_nsfw;
    data["prefs"]["use_hls"] = prefs.use_hls;
    data["prefs"]["hide_hls_notification"] = prefs.hide_hls_notification;
    data["prefs"]["autoplay_videos"] = prefs.autoplay_videos;
    data["prefs"]["comment_sort"] = prefs.comment_sort;
    data["prefs"]["post_sort"] = prefs.post_sort;
    data["prefs"]["hide_awards"] = prefs.hide_awards;
    data["prefs"]["hide_score"] = prefs.hide_score;
    data["prefs"]["available_themes"] = prefs.available_themes;
    data["prefs"]["subscriptions"] = prefs.subscriptions;
    data["prefs"]["filters"] = prefs.filters;
    data["prefs"]["fixed_navbar"] = prefs.fixed_navbar;
    data["url"] = path;

    return render_template("settings.html", data);
}

std::string settings_set(const std::string& path,
                          const std::string& query_string,
                          const std::string& body,
                          const std::unordered_map<std::string, std::string>& params,
                          const std::unordered_map<std::string, std::string>& cookies,
                          std::unordered_map<std::string, std::string>& set_cookies) {
    // Parse form body
    std::unordered_map<std::string, std::string> form;
    std::stringstream ss(body);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eqpos = pair.find('=');
        if (eqpos != std::string::npos) {
            form[pair.substr(0, eqpos)] = pair.substr(eqpos + 1);
        }
    }

    static const std::vector<std::string> PREFS = {
        "theme", "front_page", "layout", "wide", "comment_sort", "post_sort",
        "blur_spoiler", "show_nsfw", "blur_nsfw", "use_hls",
        "hide_hls_notification", "autoplay_videos", "hide_sidebar_and_summary",
        "fixed_navbar", "hide_awards", "hide_score",
        "disable_visit_reddit_confirmation", "video_quality", "remove_default_feeds"
    };

    for (const auto& name : PREFS) {
        auto it = form.find(name);
        if (it != form.end()) {
            set_cookies[name] = it->second;
        } else {
            set_cookies[name] = ""; // Signal to remove
        }
    }

    return redirect("/settings");
}

std::string settings_restore(const std::string& path,
                              const std::string& query_string,
                              const std::unordered_map<std::string, std::string>& params,
                              const std::unordered_map<std::string, std::string>& cookies,
                              std::unordered_map<std::string, std::string>& set_cookies) {
    // Parse query params and set cookies
    for (const auto& [key, val] : params) {
        set_cookies[key] = val;
    }

    std::string redir_path = "/";
    auto redir = param("?" + query_string, "redirect");
    if (redir) redir_path = *redir;
    if (!redir_path.starts_with("/")) redir_path = "/" + redir_path;

    return redirect(redir_path);
}

std::string settings_update(const std::string& path,
                             const std::string& query_string,
                             const std::unordered_map<std::string, std::string>& params,
                             const std::unordered_map<std::string, std::string>& cookies,
                             std::unordered_map<std::string, std::string>& set_cookies) {
    return settings_restore(path, query_string, params, cookies, set_cookies);
}

// ---- Duplicates ----
std::string duplicates_item(const std::string& path,
                             const std::string& query_string,
                             const std::unordered_map<std::string, std::string>& params,
                             const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);
    std::string req_url = path;
    if (!query_string.empty()) req_url += "?" + query_string;

    std::string api_path = path + ".json?" + query_string + "&raw_json=1";

    try {
        auto res = reddit_json(api_path, false);
        auto post = parse_post(res[0]["data"]["children"][0]);

        std::vector<Post> duplicates;
        auto children = res[1]["data"]["children"];
        if (children.is_array()) {
            for (const auto& child : children) {
                duplicates.push_back(parse_post(child));
            }
        }

        json data;
        data["params"]["before"] = "";
        data["params"]["after"] = res[1]["data"].value("after", "");
        data["params"]["sort"] = "";

        json post_j;
        post_j["id"] = post.id;
        post_j["title"] = post.title;
        post_j["author"]["name"] = post.author.name;
        post_j["permalink"] = post.permalink;
        post_j["rel_time"] = post.rel_time;
        post_j["score"] = json::array({post.score.first, post.score.second});
        post_j["comments"] = json::array({post.comments.first, post.comments.second});
        post_j["nsfw"] = post.nsfw;
        data["post"] = post_j;

        json dups_arr = json::array();
        for (const auto& d : duplicates) {
            json dj;
            dj["id"] = d.id;
            dj["title"] = d.title;
            dj["author"]["name"] = d.author.name;
            dj["permalink"] = d.permalink;
            dj["rel_time"] = d.rel_time;
            dj["score"] = json::array({d.score.first, d.score.second});
            dj["comments"] = json::array({d.comments.first, d.comments.second});
            dups_arr.push_back(dj);
        }
        data["duplicates"] = dups_arr;
        data["prefs"] = prefs.to_json();
        data["url"] = req_url;
        data["num_posts_filtered"] = 0;
        data["all_posts_filtered"] = false;

        return render_template("duplicates.html", data);
    } catch (const std::exception& e) {
        return error_response(e.what(), prefs, req_url);
    }
}

// ---- Instance Info ----
std::string instance_info_page(const std::string& path,
                                const std::string& query_string,
                                const std::unordered_map<std::string, std::string>& params,
                                const std::unordered_map<std::string, std::string>& cookies) {
    Preferences prefs = Preferences::from_cookies(cookies);

    std::string extension = "";
    auto eit = params.find("extension");
    if (eit != params.end()) extension = eit->second;

    if (extension == "json") {
        json info;
        info["package_name"] = "pinklib";
        info["crate_version"] = "0.36.0";
        info["compile_mode"] = "Release";
        auto banner = get_setting("REDLIB_BANNER");
        info["banner"] = banner.value_or("");
        return info.dump();
    }

    if (extension == "txt") {
        std::stringstream txt;
        txt << "Package name: pinklib\n";
        txt << "Version: 0.36.0\n";
        txt << "Compile mode: Release\n";
        return txt.str();
    }

    // HTML
    json data;
    data["title"] = "Instance Information";
    data["body"] = "<h2>PinkLib v0.36.0</h2><p>A private front-end for Reddit written in C++</p>";
    data["prefs"] = prefs.to_json();
    data["url"] = path;

    return render_template("message.html", data);
}

} // namespace pinklib
