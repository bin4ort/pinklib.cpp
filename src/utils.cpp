#include "utils.h"
#include "config.h"
#include "templates_embedded.h"
#include <regex>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <zlib.h>
#include <inja/inja.hpp>

namespace pinklib {

// ---- Static assets registry ----
static const std::unordered_map<std::string, std::string>& template_files() {
    static std::unordered_map<std::string, std::string> tmpls;
    if (tmpls.empty()) {
        // Templates are loaded from files at runtime
    }
    return tmpls;
}

// ---- FlairPart ----
std::vector<FlairPart> FlairPart::parse(const std::string& flair_type,
        const json* rich_flair, const std::string& text_flair) {
    if (flair_type == "richtext" && rich_flair && rich_flair->is_array()) {
        std::vector<FlairPart> parts;
        for (const auto& part : *rich_flair) {
            std::string e = part.value("e", "");
            std::string t = part.value("t", "");
            std::string u = part.value("u", "");
            FlairPart fp;
            fp.flair_part_type = e;
            if (e == "text") fp.value = t;
            else if (e == "emoji") fp.value = format_url(u);
            parts.push_back(fp);
        }
        return parts;
    }
    if (flair_type == "text" && !text_flair.empty()) {
        return {FlairPart{"text", text_flair}};
    }
    return {};
}

// ---- PollOption ----
std::vector<PollOption> PollOption::parse(const json& options) {
    std::vector<PollOption> result;
    if (!options.is_array()) return result;
    for (const auto& opt : options) {
        PollOption o;
        std::string id_str = opt.value("id", "");
        if (!id_str.empty()) o.id = std::stoull(id_str);
        o.text = opt.value("text", "");
        if (opt.contains("vote_count") && !opt["vote_count"].is_null())
            o.vote_count = opt["vote_count"].get<uint64_t>();
        result.push_back(o);
    }
    return result;
}

// ---- Poll ----
std::optional<Poll> Poll::parse(const json& poll_data) {
    if (!poll_data.is_object() || poll_data.empty()) return std::nullopt;
    Poll p;
    p.total_vote_count = poll_data.value("total_vote_count", 0ULL);
    double vt = poll_data.value("voting_end_timestamp", 0.0);
    p.voting_end_timestamp = time_str(vt / 1000.0);
    p.poll_options = PollOption::parse(poll_data.value("options", json::array()));
    if (p.poll_options.empty()) return std::nullopt;
    return p;
}

uint64_t Poll::most_votes() const {
    uint64_t max_v = 0;
    for (const auto& o : poll_options) {
        if (o.vote_count.value_or(0) > max_v) max_v = *o.vote_count;
    }
    return max_v;
}

// ---- GalleryMedia ----
std::vector<GalleryMedia> GalleryMedia::parse(const json& items, const json& metadata) {
    std::vector<GalleryMedia> result;
    if (!items.is_array()) return result;
    for (const auto& item : items) {
        GalleryMedia gm;
        std::string media_id = item.value("media_id", "");
        const auto& image = metadata.value(media_id, json::object());
        const auto& s = image.value("s", json::object());
        std::string mime = image.value("m", "");
        if (mime == "image/gif") gm.url = s.value("gif", "");
        else gm.url = s.value("u", "");
        gm.url = format_url(gm.url);
        gm.width = s.value("x", 0);
        gm.height = s.value("y", 0);
        gm.caption = item.value("caption", "");
        gm.outbound_url = item.value("outbound_url", "");
        result.push_back(gm);
    }
    return result;
}

// ---- Awards ----
Awards Awards::parse(const json& items) {
    Awards awards;
    if (!items.is_array()) return awards;
    for (const auto& item : items) {
        Award a;
        a.name = item.value("name", "");
        a.description = item.value("description", "");
        auto icons = item.value("resized_icons", json::array());
        if (!icons.empty()) a.icon_url = format_url(icons[0].value("url", ""));
        a.count = item.value("count", 1);
        awards.push_back(a);
    }
    return awards;
}

// ---- Media ----
std::tuple<std::string, Media, std::vector<GalleryMedia>> Media::parse(const json& data) {
    std::vector<GalleryMedia> gallery;
    std::string post_type;
    const json* url_val = nullptr;
    const json* alt_url_val = nullptr;

    auto get_val = [&](const json& j, const std::string& key) -> const json* {
        if (j.contains(key) && !j[key].is_null()) return &j[key];
        return nullptr;
    };

    auto get_nested = [&](const json& j, std::initializer_list<const char*> keys) -> const json* {
        const json* cur = &j;
        for (auto k : keys) {
            if (!cur->contains(k) || (*cur)[k].is_null()) return nullptr;
            cur = &(*cur)[k];
        }
        return cur;
    };

    auto preview_video = get_nested(data, {"preview", "reddit_video_preview"});
    auto secure_video = get_nested(data, {"secure_media", "reddit_video"});
    auto crosspost_video = data.contains("crosspost_parent_list") && data["crosspost_parent_list"].is_array() && !data["crosspost_parent_list"].empty()
        ? get_nested(data["crosspost_parent_list"][0], {"secure_media", "reddit_video"}) : nullptr;

    if (preview_video && !preview_video->empty() && preview_video->contains("fallback_url")) {
        bool is_gif = preview_video->value("is_gif", false);
        post_type = is_gif ? "gif" : "video";
        url_val = &(*preview_video)["fallback_url"];
        if (preview_video->contains("hls_url")) alt_url_val = &(*preview_video)["hls_url"];
    } else if (secure_video && secure_video->is_object() && secure_video->contains("fallback_url")) {
        bool is_gif = secure_video->value("is_gif", false);
        post_type = is_gif ? "gif" : "video";
        url_val = &(*secure_video)["fallback_url"];
        if (secure_video->contains("hls_url")) alt_url_val = &(*secure_video)["hls_url"];
    } else if (crosspost_video && crosspost_video->is_object() && crosspost_video->contains("fallback_url")) {
        bool is_gif = crosspost_video->value("is_gif", false);
        post_type = is_gif ? "gif" : "video";
        url_val = &(*crosspost_video)["fallback_url"];
        if (crosspost_video->contains("hls_url")) alt_url_val = &(*crosspost_video)["hls_url"];
    } else if (data.value("post_hint", "") == "image") {
        auto preview_img = get_nested(data, {"preview", "images", "0"});
        auto mp4 = preview_img ? get_nested(*preview_img, {"variants", "mp4"}) : nullptr;
        if (mp4 && mp4->is_object()) {
            post_type = "gif";
            url_val = &(*mp4)["source"]["url"];
        } else {
            post_type = "image";
            if (data.value("domain", "") == "i.redd.it") url_val = &data["url"];
            else if (preview_img) url_val = &(*preview_img)["source"]["url"];
            else url_val = &data["url"];
        }
    } else if (data.value("is_self", false)) {
        post_type = "self";
        url_val = &data["permalink"];
    } else if (data.value("is_gallery", false)) {
        gallery = GalleryMedia::parse(data["gallery_data"]["items"], data["media_metadata"]);
        post_type = "gallery";
        url_val = &data["url"];
    } else if (data.contains("crosspost_parent_list") && data["crosspost_parent_list"].is_array() &&
               !data["crosspost_parent_list"].empty() &&
               data["crosspost_parent_list"][0].value("is_gallery", false)) {
        gallery = GalleryMedia::parse(
            data["crosspost_parent_list"][0]["gallery_data"]["items"],
            data["crosspost_parent_list"][0]["media_metadata"]);
        post_type = "gallery";
        url_val = &data["url"];
    } else if (data.value("is_reddit_media_domain", false) && data.value("domain", "") == "i.redd.it") {
        post_type = "image";
        url_val = &data["url"];
    } else {
        post_type = "link";
        url_val = &data["url"];
    }

    auto* source_img = get_nested(data, {"preview", "images", "0", "source"});
    int64_t w = source_img ? source_img->value("width", 0) : 0;
    int64_t h = source_img ? source_img->value("height", 0) : 0;
    std::string poster_url = source_img ? format_url(source_img->value("url", "")) : "";
    std::string alt_url = (alt_url_val && alt_url_val->is_string()) ?
        format_url(alt_url_val->get<std::string>()) : "";

    std::string url = (url_val && url_val->is_string()) ?
        format_url(url_val->get<std::string>()) : "";

    std::string download_name;
    if (post_type == "image" || post_type == "gif" || post_type == "video") {
        std::string perm_base = url_path_basename(data.value("permalink", ""));
        std::string url_base = url_path_basename(
            url_val && url_val->is_string() ? url_val->get<std::string>() : "");
        download_name = "pinklib_" + perm_base + "_" + url_base;
    }

    Media media{
        url,
        alt_url,
        w,
        h,
        poster_url,
        download_name
    };

    return {post_type, media, gallery};
}

// ---- Preferences ----
void Preferences::populate_available_themes() {
    available_themes = {"system"};
    // Theme names from the themes directory
    // These are discovered at runtime from the filesystem
    static const std::vector<std::string> themes = {
        "dark", "light", "black", "dracula", "nord", "gruvboxdark",
        "gruvboxlight", "tokyoNight", "violet", "gold", "laserwave",
        "doomone", "midnightPurple", "icebergDark", "rosebox",
        "libredditDark", "libredditLight", "libredditBlack"
    };
    for (const auto& t : themes) available_themes.push_back(t);
}

std::string Preferences::pref_setting(const std::unordered_map<std::string, std::string>& cookies,
                                       const std::string& name) {
    auto it = cookies.find(name);
    if (it != cookies.end() && !it->second.empty()) return it->second;

    // Try default from config
    auto key = "REDLIB_DEFAULT_" + name;
    std::transform(key.begin(), key.end(), key.begin(), ::toupper);
    auto def = get_setting(key);
    if (def && !def->empty()) return *def;

    return "";
}

Preferences Preferences::from_cookies(const std::unordered_map<std::string, std::string>& cookies) {
    Preferences p;
    p.populate_available_themes();
    p.theme = pref_setting(cookies, "theme");
    p.front_page = pref_setting(cookies, "front_page");
    p.layout = pref_setting(cookies, "layout");
    p.wide = pref_setting(cookies, "wide");
    p.blur_spoiler = pref_setting(cookies, "blur_spoiler");
    p.show_nsfw = pref_setting(cookies, "show_nsfw");
    p.blur_nsfw = pref_setting(cookies, "blur_nsfw");
    p.hide_hls_notification = pref_setting(cookies, "hide_hls_notification");
    p.video_quality = pref_setting(cookies, "video_quality");
    p.hide_sidebar_and_summary = pref_setting(cookies, "hide_sidebar_and_summary");
    p.use_hls = pref_setting(cookies, "use_hls");
    p.autoplay_videos = pref_setting(cookies, "autoplay_videos");
    p.fixed_navbar = pref_setting(cookies, "fixed_navbar");
    if (p.fixed_navbar.empty()) p.fixed_navbar = "on";
    p.disable_visit_reddit_confirmation = pref_setting(cookies, "disable_visit_reddit_confirmation");
    p.comment_sort = pref_setting(cookies, "comment_sort");
    p.post_sort = pref_setting(cookies, "post_sort");
    p.hide_awards = pref_setting(cookies, "hide_awards");
    p.hide_score = pref_setting(cookies, "hide_score");
    p.remove_default_feeds = pref_setting(cookies, "remove_default_feeds");

    // Subscriptions
    std::string subs = pref_setting(cookies, "subscriptions");
    if (!subs.empty()) {
        std::stringstream ss(subs);
        std::string item;
        while (std::getline(ss, item, '+')) {
            if (!item.empty()) p.subscriptions.push_back(item);
        }
    }

    // Filters
    std::string filts = pref_setting(cookies, "filters");
    if (!filts.empty()) {
        std::stringstream ss(filts);
        std::string item;
        while (std::getline(ss, item, '+')) {
            if (!item.empty()) p.filters.push_back(item);
        }
    }

    return p;
}

std::string Preferences::to_urlencoded() const {
    std::stringstream ss;
    auto add = [&](const std::string& key, const std::string& val) {
        if (!ss.str().empty()) ss << "&";
        ss << key << "=" << val;
    };

    if (!theme.empty()) add("theme", theme);
    if (!front_page.empty()) add("front_page", front_page);
    if (!layout.empty()) add("layout", layout);
    if (!wide.empty()) add("wide", wide);
    if (!blur_spoiler.empty()) add("blur_spoiler", blur_spoiler);
    if (!show_nsfw.empty()) add("show_nsfw", show_nsfw);
    if (!blur_nsfw.empty()) add("blur_nsfw", blur_nsfw);
    if (!use_hls.empty()) add("use_hls", use_hls);
    if (!hide_hls_notification.empty()) add("hide_hls_notification", hide_hls_notification);
    if (!autoplay_videos.empty()) add("autoplay_videos", autoplay_videos);
    if (!comment_sort.empty()) add("comment_sort", comment_sort);
    if (!post_sort.empty()) add("post_sort", post_sort);
    if (!hide_awards.empty()) add("hide_awards", hide_awards);
    if (!hide_score.empty()) add("hide_score", hide_score);
    if (!subscriptions.empty()) {
        std::string combined;
        for (size_t i = 0; i < subscriptions.size(); i++) {
            if (i > 0) combined += "+";
            combined += subscriptions[i];
        }
        add("subscriptions", combined);
    }
    if (!filters.empty()) {
        std::string combined;
        for (size_t i = 0; i < filters.size(); i++) {
            if (i > 0) combined += "+";
            combined += filters[i];
        }
        add("filters", combined);
    }

    return ss.str();
}

std::string Preferences::to_bincode_str() const {
    return to_urlencoded();
}

nlohmann::json Preferences::to_json() const {
    json j;
    j["theme"] = theme;
    j["front_page"] = front_page;
    j["layout"] = layout;
    j["wide"] = wide;
    j["blur_spoiler"] = blur_spoiler;
    j["show_nsfw"] = show_nsfw;
    j["blur_nsfw"] = blur_nsfw;
    j["hide_hls_notification"] = hide_hls_notification;
    j["use_hls"] = use_hls;
    j["autoplay_videos"] = autoplay_videos;
    j["fixed_navbar"] = fixed_navbar;
    j["disable_visit_reddit_confirmation"] = disable_visit_reddit_confirmation;
    j["comment_sort"] = comment_sort;
    j["post_sort"] = post_sort;
    j["hide_awards"] = hide_awards;
    j["hide_score"] = hide_score;
    j["hide_sidebar_and_summary"] = hide_sidebar_and_summary;
    j["video_quality"] = video_quality;
    j["remove_default_feeds"] = remove_default_feeds;
    j["subscriptions"] = subscriptions;
    j["filters"] = filters;
    j["available_themes"] = available_themes;
    return j;
}

// ---- Post ----
std::pair<std::vector<Post>, std::string> Post::fetch(const std::string& path, bool quarantine) {
    return {{}, ""};  // Implemented in client.cpp
}

// ---- val() ----
std::string val(const json& j, const std::string& k) {
    if (j.contains("data") && j["data"].contains(k)) {
        auto& v = j["data"][k];
        if (v.is_string()) return v.get<std::string>();
        if (v.is_number()) {
            double d = v.get<double>();
            if (d == static_cast<int64_t>(d))
                return std::to_string(static_cast<int64_t>(d));
            return std::to_string(d);
        }
        return v.dump();
    }
    return "";
}

// ---- format_num ----
std::pair<std::string, std::string> format_num(int64_t num) {
    auto abs_num = num < 0 ? -num : num;
    std::string truncated;
    if (abs_num >= 1'000'000) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (num / 1'000'000.0) << "m";
        truncated = ss.str();
    } else if (abs_num >= 1000) {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << (num / 1'000.0) << "k";
        truncated = ss.str();
    } else {
        truncated = std::to_string(num);
    }
    return {truncated, std::to_string(num)};
}

// ---- time_str ----
std::pair<std::string, std::string> time_str(double created) {
    auto created_tp = std::chrono::system_clock::from_time_t(static_cast<time_t>(created));
    auto now = std::chrono::system_clock::now();
    auto duration = now - created_tp;

    auto days = std::chrono::duration_cast<std::chrono::hours>(duration).count() / 24;
    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count();

    std::string rel_time;
    if (duration.count() < 0) duration = std::chrono::seconds(0);

    if (days > 30) {
        auto tt = std::chrono::system_clock::to_time_t(created_tp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&tt), "%b %d '%y");
        rel_time = ss.str();
    } else if (days > 0) {
        rel_time = std::to_string(days) + "d ago";
    } else if (hours > 0) {
        rel_time = std::to_string(hours) + "h ago";
    } else {
        rel_time = std::to_string(std::max(int64_t(1), minutes)) + "m ago";
    }

    auto tt = std::chrono::system_clock::to_time_t(created_tp);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&tt), "%b %d %Y, %H:%M:%S UTC");

    return {rel_time, ss.str()};
}

// ---- URL helpers ----
std::optional<std::string> param(const std::string& path, const std::string& value) {
    // Simple query string parser
    auto qpos = path.find('?');
    if (qpos == std::string::npos) return std::nullopt;
    std::string query = path.substr(qpos + 1);
    std::stringstream ss(query);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eqpos = pair.find('=');
        if (eqpos != std::string::npos) {
            std::string key = pair.substr(0, eqpos);
            std::string val = pair.substr(eqpos + 1);
            if (key == value) return val;
        }
    }
    return std::nullopt;
}

std::string url_path_basename(const std::string& path) {
    auto pos = path.find_last_of('/');
    if (pos != std::string::npos) {
        std::string base = path.substr(pos + 1);
        auto qpos = base.find('?');
        if (qpos != std::string::npos) base = base.substr(0, qpos);
        if (!base.empty()) return base;
    }
    return path;
}

std::string to_absolute_url(const std::string& relative_path) {
    auto full_url = get_setting("REDLIB_FULL_URL");
    return (full_url ? *full_url : "") + relative_path;
}

std::string get_post_url(const Post& post) {
    if (post.post_type == "image" || post.post_type == "gallery" ||
        post.post_type == "gif" || post.post_type == "video")
        return to_absolute_url(post.permalink);
    if (post.out_url) {
        if (post.out_url->starts_with("/r/")) return to_absolute_url(*post.out_url);
        return *post.out_url;
    }
    return to_absolute_url(post.permalink);
}

// ---- format_url ----
std::string format_url(const std::string& url) {
    if (url.empty() || url == "self" || url == "default" || url == "nsfw" || url == "spoiler")
        return "";

    static std::regex re_video(R"(https?://v\.redd\.it/(.*)/DASH_([0-9]{2,4}(\.mp4|$|\?source=fallback)))");
    static std::regex re_video_hls(R"(https?://v\.redd\.it/(.+)/(HLSPlaylist\.m3u8.*)$)");
    static std::regex re_images(R"(https?://i\.redd\.it/(.*))");
    static std::regex re_thumbs_a(R"(https?://a\.thumbs\.redditmedia\.com/(.*))");
    static std::regex re_thumbs_b(R"(https?://b\.thumbs\.redditmedia\.com/(.*))");
    static std::regex re_emoji(R"(https?://emoji\.redditmedia\.com/(.*)/(.*))");
    static std::regex re_preview(R"(https?://preview\.redd\.it/(.*))");
    static std::regex re_ext_preview(R"(https?://external-preview\.redd\.it/(.*))");
    static std::regex re_styles(R"(https?://styles\.redditmedia\.com/(.*))");
    static std::regex re_static(R"(https?://www\.redditstatic\.com/(.*))");
    static std::regex re_www(R"(https?://www\.reddit\.com/(.*))");
    static std::regex re_old(R"(https?://old\.reddit\.com/(.*))");
    static std::regex re_np(R"(https?://np\.reddit\.com/(.*))");
    static std::regex re_plain(R"(https?://reddit\.com/(.*))");

    std::smatch m;

    if (std::regex_search(url, m, re_video)) return "/vid/" + m[1].str() + "/" + m[2].str();
    if (std::regex_search(url, m, re_video_hls)) return "/hls/" + m[1].str() + "/" + m[2].str();
    if (std::regex_search(url, m, re_images)) return "/img/" + m[1].str();
    if (std::regex_search(url, m, re_thumbs_a)) return "/thumb/a/" + m[1].str();
    if (std::regex_search(url, m, re_thumbs_b)) return "/thumb/b/" + m[1].str();
    if (std::regex_search(url, m, re_emoji)) return "/emoji/" + m[1].str() + "/" + m[2].str();
    if (std::regex_search(url, m, re_preview)) return "/preview/pre/" + m[1].str();
    if (std::regex_search(url, m, re_ext_preview)) return "/preview/external-pre/" + m[1].str();
    if (std::regex_search(url, m, re_styles)) return "/style/" + m[1].str();
    if (std::regex_search(url, m, re_static)) return "/static/" + m[1].str();
    if (std::regex_search(url, m, re_www)) return "/" + m[1].str();
    if (std::regex_search(url, m, re_old)) return "/" + m[1].str();
    if (std::regex_search(url, m, re_np)) return "/" + m[1].str();
    if (std::regex_search(url, m, re_plain)) return "/" + m[1].str();

    return url;
}

// ---- rewrite_urls ----
std::string rewrite_urls(const std::string& input_text) {
    static std::regex reddit_regex(R"(href="(https|http|)://(www\.|old\.|np\.|amp\.|new\.|)(reddit\.com|redd\.it)/)");
    static std::regex preview_regex(R"(https?://(external-preview|preview|i)\.redd\.it(.*))");
    static std::regex emoji_regex(R"(https?://(www|).redditstatic\.com/(.*))");
    static std::regex preview_link_regex(R"(/(img|preview/)(pre|external-pre)?/(.*?)>)");

    std::string result = std::regex_replace(input_text, reddit_regex, "href=\"/");
    result = std::regex_replace(result, std::regex(R"(%5C)"), "");
    result = std::regex_replace(result, std::regex(R"(\\_)"), "_");

    // Replace preview URLs - simplified
    std::string prev_result;
    do {
        prev_result = result;
        std::smatch m;
        if (std::regex_search(result, m, preview_regex)) {
            std::string full = m[0].str();
            std::string formatted = format_url(full);
            result = m.prefix().str() + formatted + m.suffix().str();
        }
    } while (result != prev_result);

    return result;
}

// ---- rewrite_emotes ----
std::string rewrite_emotes(const json& media_metadata, const std::string& comment) {
    std::string result = comment;
    if (!media_metadata.is_object()) return render_bullet_lists(result);

    static std::regex emote_link(R"(https://reddit-econ-prod-assets-permanent\.s3\.amazonaws\.com/asset-manager/(.*))");
    static std::regex emote_id(R"regex("emote\|.*\|(.*)")regex");

    for (const auto& [key, val] : media_metadata.items()) {
        if (!val.is_object()) continue;
        std::string s_u = val.value("s", json::object()).value("u", "");
        if (s_u.empty()) continue;
        std::string id_str = val.value("id", "");
        std::smatch id_m;
        if (std::regex_search(id_str, id_m, emote_id)) {
            std::string emote_id_str = ":" + id_m[1].str() + ":";
            std::smatch link_m;
            if (std::regex_search(s_u, link_m, emote_link)) {
                int64_t size = val.value("s", json::object()).value("y", 20);
                std::string replacement = "<img loading=\"lazy\" src=\"/emote/" +
                    link_m[1].str() + "\" width=\"" + std::to_string(size) +
                    "\" height=\"" + std::to_string(size) +
                    "\" style=\"vertical-align:text-bottom\">";
                size_t pos = result.find(emote_id_str);
                if (pos != std::string::npos) {
                    result.replace(pos, emote_id_str.length(), replacement);
                }
            }
        }
    }

    result = render_bullet_lists(result);
    return rewrite_urls(result);
}

// ---- render_bullet_lists ----
std::string render_bullet_lists(const std::string& input_text) {
    static std::regex bullet(R"((?m)^- (.*)$)");
    static std::regex consecutive(R"(</ul>\n<ul>)");
    std::string r = std::regex_replace(input_text, bullet, "<ul><li>$1</li></ul>");
    r = std::regex_replace(r, consecutive, "");
    return r;
}

// ---- Condition checkers ----
bool sfw_only() {
    auto val = get_setting("REDLIB_SFW_ONLY");
    return val && *val == "on";
}

bool enable_rss() {
    auto val = get_setting("REDLIB_ENABLE_RSS");
    return val && *val == "on";
}

bool disable_indexing() {
    auto val = get_setting("REDLIB_ROBOTS_DISABLE_INDEXING");
    return val && *val == "on";
}

bool should_be_nsfw_gated(const std::string& show_nsfw) {
    return show_nsfw != "on" || sfw_only();
}

// ---- Cookie helpers ----
std::unordered_map<std::string, std::string> parse_cookies(const std::string& cookie_header) {
    std::unordered_map<std::string, std::string> cookies;
    std::stringstream ss(cookie_header);
    std::string cookie;
    while (std::getline(ss, cookie, ';')) {
        size_t start = cookie.find_first_not_of(" \t");
        if (start != std::string::npos) cookie = cookie.substr(start);
        auto eqpos = cookie.find('=');
        if (eqpos != std::string::npos) {
            std::string key = cookie.substr(0, eqpos);
            std::string val = cookie.substr(eqpos + 1);
            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            cookies[key] = val;
        }
    }
    return cookies;
}

std::string get_cookie_val(const std::unordered_map<std::string, std::string>& cookies,
                            const std::string& name) {
    auto it = cookies.find(name);
    return it != cookies.end() ? it->second : "";
}

void filter_posts(std::vector<Post>& posts,
                  const std::unordered_set<std::string>& filters,
                  uint64_t& num_filtered, bool& all_filtered) {
    uint64_t before = posts.size();
    posts.erase(std::remove_if(posts.begin(), posts.end(), [&](const Post& p) {
        return filters.count(p.community) > 0 ||
               filters.count("u_" + p.author.name) > 0;
    }), posts.end());
    uint64_t after = posts.size();
    num_filtered = before - after;
    all_filtered = after == 0 && before > 0;
}

// ---- parse_post ----
Post parse_post(const json& post) {
    auto& data = post["data"];
    try {
    auto [rel_time, created] = time_str(data.value("created_utc", 0.0));
    int64_t score = data.value("score", 0);
    double ratio = data.value("upvote_ratio", 1.0) * 100.0;

    auto [post_type, media, gallery] = Media::parse(data);
    uint64_t created_ts = static_cast<uint64_t>(data.value("created_utc", 0.0));
    Awards awards = Awards::parse(data.value("all_awardings", json::array()));

    std::string body;
    if (val(post, "removed_by_category") == "moderator") {
        auto ps_frontend = get_setting("REDLIB_PUSHSHIFT_FRONTEND").value_or(std::string(DEFAULT_PUSHSHIFT_FRONTEND));
        body = "<div class=\"md\"><p>[removed] &mdash; <a href=\"https://" + ps_frontend +
               data.value("permalink", "") + "\">view removed post</a></p></div>";
    } else {
        body = rewrite_urls(val(post, "selftext_html"));
        if (body.empty()) body = rewrite_urls(val(post, "body_html"));
    }

    Post p;
    p.id = val(post, "id");
    p.title = val(post, "title");
    p.community = val(post, "subreddit");
    p.body = body;
    p.author.name = val(post, "author");
    p.author.flair.flair_parts = FlairPart::parse(
        data.value("author_flair_type", ""),
        data.contains("author_flair_richtext") && data["author_flair_richtext"].is_array()
            ? &data["author_flair_richtext"] : nullptr,
        data.value("author_flair_text", ""));
    p.author.flair.text = val(post, "link_flair_text");
    p.author.flair.background_color = val(post, "author_flair_background_color");
    p.author.flair.foreground_color = val(post, "author_flair_text_color");
    p.author.distinguished = val(post, "distinguished");
    p.permalink = val(post, "permalink");
    p.link_title = val(post, "link_title");
    p.poll = Poll::parse(data.value("poll_data", json::object()));
    if (data.value("hide_score", false)) {
        p.score = {"\u2022", "Hidden"};
    } else {
        p.score = format_num(score);
    }
    p.upvote_ratio = static_cast<int64_t>(ratio);
    p.post_type = post_type;
    p.media = media;
    p.thumbnail.url = format_url(val(post, "thumbnail"));
    p.thumbnail.width = data.value("thumbnail_width", 0);
    p.thumbnail.height = data.value("thumbnail_height", 0);
    p.flair.flair_parts = FlairPart::parse(
        data.value("link_flair_type", ""),
        data.contains("link_flair_richtext") && data["link_flair_richtext"].is_array()
            ? &data["link_flair_richtext"] : nullptr,
        data.value("link_flair_text", ""));
    p.flair.text = val(post, "link_flair_text");
    p.flair.background_color = val(post, "link_flair_background_color");
    p.flair.foreground_color = val(post, "link_flair_text_color") == "dark" ? "black" : "white";
    p.flags.spoiler = data.value("spoiler", false);
    p.flags.nsfw = data.value("over_18", false);
    p.flags.stickied = data.value("stickied", false) || data.value("pinned", false);
    p.domain = val(post, "domain");
    p.rel_time = rel_time;
    p.created = created;
    p.created_ts = created_ts;
    p.num_duplicates = data.value("num_duplicates", 0ULL);
    p.comments = format_num(data.value("num_comments", 0));
    p.gallery = gallery;
    p.awards = awards;
    p.nsfw = data.value("over_18", false);
    p.ws_url = val(post, "websocket_url");
    if (data.contains("url_overridden_by_dest") && data["url_overridden_by_dest"].is_string())
        p.out_url = data["url_overridden_by_dest"].get<std::string>();

    return p;
    } catch (const std::exception& e) {
        std::cerr << "[DEBUG] parse_post inner error: " << e.what() << " | post data keys: ";
        for (auto& [k, v] : data.items()) std::cerr << k << " ";
        std::cerr << std::endl;
        throw;
    }
}

// ---- deflate ----
std::string deflate_compress(const std::vector<uint8_t>& input) {
    uLongf destLen = compressBound(input.size());
    std::vector<uint8_t> dest(destLen);
    if (compress(dest.data(), &destLen, input.data(), input.size()) == Z_OK) {
        dest.resize(destLen);
        return std::string(dest.begin(), dest.end());
    }
    return "";
}

std::string deflate_decompress(const std::vector<uint8_t>& input) {
    uLongf destLen = input.size() * 4;
    std::vector<uint8_t> dest(destLen);
    while (true) {
        int res = uncompress(dest.data(), &destLen, input.data(), input.size());
        if (res == Z_BUF_ERROR) {
            destLen *= 2;
            dest.resize(destLen);
        } else if (res == Z_OK) {
            dest.resize(destLen);
            return std::string(dest.begin(), dest.end());
        } else {
            return "";
        }
    }
}

// ---- Template Rendering ----
static std::string load_raw_template(const std::string& name) {
    const auto& tmpls = embedded_templates();
    auto it = tmpls.find(name);
    if (it != tmpls.end()) return it->second;
    std::string path = "templates/" + name;
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::stringstream buf;
    buf << file.rdbuf();
    return buf.str();
}

// Parse macros from a template file (e.g. utils.html)
// Returns map: macro_name → (arg_names, body)
static std::unordered_map<std::string, std::pair<std::vector<std::string>, std::string>>
parse_macros(const std::string& content) {
    std::unordered_map<std::string, std::pair<std::vector<std::string>, std::string>> macros;
    static std::regex re_macro(
        R"(\{%\-?\s*macro\s+(\w+)\(([^)]*)\)\s*\-?%\}\s*([\s\S]*?)\{%\-?\s*endmacro\s*\-?%\})");
    auto it = std::sregex_iterator(content.begin(), content.end(), re_macro);
    for (; it != std::sregex_iterator(); ++it) {
        std::string name = (*it)[1].str();
        std::string args_str = (*it)[2].str();
        std::string body = (*it)[3].str();
        // Parse argument names
        std::vector<std::string> args;
        std::stringstream ss(args_str);
        std::string arg;
        while (std::getline(ss, arg, ',')) {
            // trim whitespace
            size_t start = arg.find_first_not_of(" \t");
            size_t end = arg.find_last_not_of(" \t");
            if (start != std::string::npos) arg = arg.substr(start, end - start + 1);
            if (!arg.empty()) args.push_back(arg);
        }
        macros[name] = {args, body};
    }
    return macros;
}

// Expand {% call ns::macro_name(args...) %} within content
static std::string expand_calls(std::string content, const std::string& ns,
                                 const std::unordered_map<std::string,
                                 std::pair<std::vector<std::string>, std::string>>& macros) {
    static std::regex re_call(R"(\{%\-?\s*call\s+(\w+)::(\w+)\(([^)]*)\)\s*\-?%\})");
    static std::regex re_call_short(R"(\{%\-?\s*call\s+(\w+)\(([^)]*)\)\s*\-?%\})");

    auto expand_one = [&](const std::smatch& m) -> std::string {
        std::string module_or_name = m[1].str();
        std::string macro_name;
        std::string args_str;

        if (m.size() > 3) {
            // call module::name(args)
            macro_name = m[2].str();
            args_str = m[3].str();
        } else {
            // call name(args)
            macro_name = module_or_name;
            args_str = m[2].str();
        }

        auto mit = macros.find(macro_name);
        if (mit == macros.end()) return "";

        const auto& [param_names, body] = mit->second;

        // Parse call arguments (comma-separated, but respect nested parens)
        std::vector<std::string> call_args;
        int depth = 0;
        std::string current;
        for (char c : args_str) {
            if (c == '(') depth++;
            else if (c == ')') depth--;
            if (c == ',' && depth == 0) {
                call_args.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        if (!current.empty()) call_args.push_back(current);
        // trim call_args
        for (auto& a : call_args) {
            size_t s = a.find_first_not_of(" \t");
            size_t e = a.find_last_not_of(" \t");
            if (s != std::string::npos) a = a.substr(s, e - s + 1);
        }

        // Substitute args into body
        std::string result = body;
        for (size_t i = 0; i < param_names.size() && i < call_args.size(); i++) {
            // Replace {{ param_name }} with call_arg in body
            std::string var_pattern = "{{ " + param_names[i] + " }}";
            std::string var_pattern_dot = "{{ " + param_names[i] + ".";
            size_t pos = 0;
            while ((pos = result.find(var_pattern_dot, pos)) != std::string::npos) {
                // Find the closing }}
                size_t end = result.find(" }}", pos);
                if (end == std::string::npos) break;
                // Replace {{ param.field }} with {{ call_arg.field }}
                std::string replacement = "{{ " + call_args[i] + "." + result.substr(pos + var_pattern_dot.length(), end - pos - var_pattern_dot.length());
                result.replace(pos, end + 3 - pos, replacement);
                pos += replacement.length();
            }
            pos = 0;
            while ((pos = result.find(var_pattern, pos)) != std::string::npos) {
                result.replace(pos, var_pattern.length(), "{{ " + call_args[i] + " }}");
                pos += call_args[i].length() + 6;
            }
        }
        return result;
    };

    // Process namespaced calls first (utils::name)
    std::string result;
    while (true) {
        std::smatch m;
        if (!std::regex_search(content, m, re_call)) break;
        std::string replacement = expand_one(m);
        content.replace(m.position(), m.length(), replacement);
    }

    // Process short calls (name)
    while (true) {
        std::smatch m;
        if (!std::regex_search(content, m, re_call_short)) break;
        std::string replacement = expand_one(m);
        content.replace(m.position(), m.length(), replacement);
    }
    return content;
}

// Strip macro definitions from content (after we've parsed them)
static std::string strip_macros(const std::string& content) {
    static std::regex re_macro(R"(\{%\-?\s*macro\s+[\s\S]*?\{%\-?\s*endmacro\s*\-?%\})");
    return std::regex_replace(content, re_macro, "");
}

// Extract blocks from template content, handling nesting properly
static std::unordered_map<std::string, std::string> extract_blocks(const std::string& content) {
    std::unordered_map<std::string, std::string> blocks;
    static std::regex re_block_open(R"(\{%\-?\s*block\s+(\w+)\s*\-?%\})");
    static std::regex re_block_close(R"(\{%\-?\s*endblock\s*\-?%\})");

    auto it = std::sregex_iterator(content.begin(), content.end(), re_block_open);
    for (; it != std::sregex_iterator(); ++it) {
        std::string name = (*it)[1].str();
        size_t start = (*it).position() + (*it).length();
        // Find matching endblock by tracking depth
        int depth = 1;
        size_t pos = start;
        auto close_it = std::sregex_iterator(content.begin() + start, content.end(), re_block_close);
        auto open_it = std::sregex_iterator(content.begin() + start, content.end(), re_block_open);
        // Walk through all block tags to find the matching close
        while (depth > 0) {
            size_t next_open = std::string::npos;
            size_t next_close = std::string::npos;
            auto oit = std::sregex_iterator(content.begin() + pos, content.end(), re_block_open);
            auto cit = std::sregex_iterator(content.begin() + pos, content.end(), re_block_close);
            if (oit != std::sregex_iterator()) next_open = pos + oit->position();
            if (cit != std::sregex_iterator()) next_close = pos + cit->position();
            if (next_close == std::string::npos) break;
            if (next_open != std::string::npos && next_open < next_close) {
                depth++;
                pos = next_open + oit->length();
            } else {
                depth--;
                if (depth == 0) {
                    size_t body_end = next_close;
                    blocks[name] = content.substr(start, body_end - start);
                }
                pos = next_close + cit->length();
            }
        }
    }
    return blocks;
}

// Resolve {% extends "parent" %} and {% block name %}...{% endblock %}
// Properly handles nested blocks by tracking depth.
static std::string resolve_extends(const std::string& content) {
    static std::regex re_extends(R"(\{%\-?\s*extends\s+\"([^\"]+)\"\s*\-?%\})");
    std::smatch m;
    if (!std::regex_search(content, m, re_extends)) return content;

    std::string parent_name = m[1].str();
    std::string parent_content = load_raw_template(parent_name);
    if (parent_content.empty()) return content;

    // Parse macros from utils.html (used by many templates)
    std::string utils_content = load_raw_template("utils.html");
    auto macros = parse_macros(utils_content);

    static std::regex re_block_open(R"(\{%\-?\s*block\s+(\w+)\s*\-?%\})");
    static std::regex re_block_close(R"(\{%\-?\s*endblock\s*\-?%\})");
    std::unordered_map<std::string, std::string> child_blocks;
    std::vector<std::tuple<size_t, size_t, std::string, std::string>> replacements;
    std::string child_after_extends = content.substr(m.position() + m.length());

    // Extract all blocks from the child (properly handle nesting)
    child_blocks = extract_blocks(child_after_extends);

    // Collect parent block positions with proper nesting matching
    {
        auto it_p = std::sregex_iterator(parent_content.begin(), parent_content.end(), re_block_open);
        for (; it_p != std::sregex_iterator(); ++it_p) {
            std::string name = (*it_p)[1].str();
            size_t start = (*it_p).position();
            size_t body_start = start + (*it_p).length();
            int depth = 1;
            size_t pos = body_start;
            while (depth > 0) {
                size_t next_open = std::string::npos, next_close = std::string::npos;
                auto oit = std::sregex_iterator(parent_content.begin() + pos, parent_content.end(), re_block_open);
                auto cit = std::sregex_iterator(parent_content.begin() + pos, parent_content.end(), re_block_close);
                if (oit != std::sregex_iterator()) next_open = pos + oit->position();
                if (cit != std::sregex_iterator()) next_close = pos + cit->position();
                if (next_close == std::string::npos) break;
                if (next_open != std::string::npos && next_open < next_close) {
                    depth++;
                    pos = next_open + oit->length();
                } else {
                    depth--;
                    if (depth == 0) {
                        auto cit2 = child_blocks.find(name);
                        std::string repl = (cit2 != child_blocks.end() && !cit2->second.empty())
                            ? cit2->second
                            : parent_content.substr(body_start, next_close - body_start);
                        replacements.emplace_back(start, next_close + cit->length() - start, name, repl);
                    }
                    pos = next_close + cit->length();
                }
            }
        }
    }
    std::sort(replacements.begin(), replacements.end(),
              [](const auto& a, const auto& b) { return std::get<0>(a) > std::get<0>(b); });

    std::string resolved = parent_content;
    for (const auto& [pos, len, name, repl] : replacements) {
        resolved.replace(pos, len, repl);
    }

    // Expand all {% call utils::name(args) %} and {% call name(args) %}
    resolved = expand_calls(resolved, "utils", macros);

    // Convert {% import "utils.html" as utils %} to include (inline the content)
    static std::regex re_import(R"(\{%\-?\s*import\s+\"([^\"]+)\"\s+as\s+\w+\s*\-?%\})");
    {
        std::smatch im;
        while (std::regex_search(resolved, im, re_import)) {
            std::string imported = strip_macros(load_raw_template(im[1].str()));
            resolved.replace(im.position(), im.length(), imported);
        }
    }

    // Convert crate::utils::disable_indexing() calls to template var
    static std::regex re_crate(R"(\{\%\s*if\s+crate::utils::disable_indexing\(\)\s*%\})");
    resolved = std::regex_replace(resolved, re_crate, "{% if disable_indexing %}");

    // Convert crate::utils::enable_rss() calls to template var
    static std::regex re_rss(R"(\{\%\s*if\s+crate::utils::enable_rss\(\)\s*%\})");
    resolved = std::regex_replace(resolved, re_rss, "{% if enable_rss %}");

    // Convert other crate calls
    static std::regex re_sfw(R"(\{\%\s*if\s+crate::utils::sfw_only\(\)\s*%\})");
    resolved = std::regex_replace(resolved, re_sfw, "{% if sfw_only %}");
    static std::regex re_nsfw(R"(\{\%\s*if\s+\!crate::utils::sfw_only\(\)\s*%\})");
    resolved = std::regex_replace(resolved, re_nsfw, "{% if not sfw_only %}");

    // Convert ResourceType enum comparisons
    static std::regex re_rt_sub(R"(crate::utils::ResourceType::Subreddit)");
    resolved = std::regex_replace(resolved, re_rt_sub, "\"Subreddit\"");
    static std::regex re_rt_user(R"(crate::utils::ResourceType::User)");
    resolved = std::regex_replace(resolved, re_rt_user, "\"User\"");
    static std::regex re_rt_post(R"(crate::utils::ResourceType::Post)");
    resolved = std::regex_replace(resolved, re_rt_post, "\"Post\"");

    // Convert Rust method calls like sub.name.as_str() to just variable
    static std::regex re_rust_call(R"(\.as_str\(\))");
    resolved = std::regex_replace(resolved, re_rust_call, "");
    static std::regex re_rust_concat(R"(\.concat\(\))");
    resolved = std::regex_replace(resolved, re_rust_concat, "");

    // Convert Rust array syntax [expr, expr] to inja list
    static std::regex re_rust_array(R"(\[([^\]]+)\])");
    // Skip if it contains inja markers like {{, {% - it's already inja
    // Just handle simple cases like ["/r/", sub.name] -> need manual fix in templates

    // Convert Rust string slicing post.permalink[1..]
    static std::regex re_slice(R"(\.permalink\[1\.\.\])");
    resolved = std::regex_replace(resolved, re_slice, ".permalink");

    // Convert format!("{}%23{}", ...) calls - drop them, handled in C++
    static std::regex re_format(R"(format\!\(\"[^\"]*\",\s*[^)]+\))");
    resolved = std::regex_replace(resolved, re_format, "\"\"");

    // Convert env vars references
    static std::regex re_env(R"(\{\{\s*crate::instance_info::INSTANCE_INFO\.git_commit\s*\}\})");
    resolved = std::regex_replace(resolved, re_env, "{{ git_commit }}");

    return resolved;
}

std::string render_template(const std::string& template_name, const json& data) {
    try {
        // Inject globals
        json d = data;
        d["disable_indexing"] = disable_indexing();
        d["enable_rss"] = enable_rss();
        d["sfw_only"] = sfw_only();
        d["version"] = "0.36.0";
        d["git_commit"] = "cpp";

        std::string raw = load_raw_template(template_name);
        if (raw.empty()) return "<html><body>Template not found: " + template_name + "</body></html>";

        // Resolve extends/blocks/macros/imports
        std::string resolved = resolve_extends(raw);

        // Final cleanup: convert any remaining {% block %}/{% endblock %} to nothing
        // (they should have been resolved by now, but just in case)
        static std::regex re_block_any(R"(\{%\-?\s*block\s+\w+.*?%\})");
        resolved = std::regex_replace(resolved, re_block_any, "");
        static std::regex re_endblock_any(R"(\{%\-?\s*endblock\s*\-?%\})");
        resolved = std::regex_replace(resolved, re_endblock_any, "");

        // Handle remaining includes
        static std::regex re_include(R"(\{%\-?\s*include\s+\"([^\"]+)\"\s*\-?%\})");
        std::smatch inc;
        while (std::regex_search(resolved, inc, re_include)) {
            std::string included = load_raw_template(inc[1].str());
            resolved.replace(inc.position(), inc.length(), included);
        }

        auto env = inja::Environment{};
        return env.render(resolved, d);
    } catch (const std::exception& e) {
        return "<html><body><h1>Template Error</h1><p>" + std::string(e.what()) + "</p></body></html>";
    }
}

} // namespace pinklib
