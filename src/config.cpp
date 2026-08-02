#include "config.h"
#include <cstdlib>
#include <fstream>
#include <toml++/toml.hpp>

namespace pinklib {

Config CONFIG;
std::once_flag CONFIG_INIT_FLAG;

void init_config() {
    std::call_once(CONFIG_INIT_FLAG, []() {
        CONFIG = Config::load();
    });
}

Config Config::load() {
    Config config;

    // Parse TOML config file
    auto load_file = [](const std::string& name) -> std::optional<toml::table> {
        try {
            return toml::parse_file(name);
        } catch (...) {
            return std::nullopt;
        }
    };

    auto tbl = load_file("pinklib.toml");
    if (!tbl) tbl = load_file("redlib.toml");
    if (!tbl) tbl = load_file("libreddit.toml");

    auto parse = [&](const std::string& key) -> std::optional<std::string> {
        // Env var takes precedence
        const char* env = std::getenv(key.c_str());
        if (env && env[0] != '\0') return std::string(env);

        // Legacy env var
        std::string legacy = key;
        size_t pos = legacy.find("REDLIB_");
        if (pos != std::string::npos) {
            legacy.replace(pos, 7, "LIBREDDIT_");
            env = std::getenv(legacy.c_str());
            if (env && env[0] != '\0') return std::string(env);
        }

        // Config file
        if (tbl) {
            auto node = tbl->get(key);
            if (node) {
                if (auto str_val = node->as_string()) return std::string(str_val->get());
            }
            // Try legacy key in config
            if (pos != std::string::npos) {
                auto node2 = tbl->get(legacy);
                if (node2) {
                    if (auto str_val = node2->as_string()) return std::string(str_val->get());
                }
            }
        }
        return std::nullopt;
    };

    config.sfw_only = parse("REDLIB_SFW_ONLY");
    config.default_theme = parse("REDLIB_DEFAULT_THEME");
    config.default_front_page = parse("REDLIB_DEFAULT_FRONT_PAGE");
    config.default_layout = parse("REDLIB_DEFAULT_LAYOUT");
    config.default_wide = parse("REDLIB_DEFAULT_WIDE");
    config.default_comment_sort = parse("REDLIB_DEFAULT_COMMENT_SORT");
    config.default_post_sort = parse("REDLIB_DEFAULT_POST_SORT");
    config.default_blur_spoiler = parse("REDLIB_DEFAULT_BLUR_SPOILER");
    config.default_show_nsfw = parse("REDLIB_DEFAULT_SHOW_NSFW");
    config.default_blur_nsfw = parse("REDLIB_DEFAULT_BLUR_NSFW");
    config.default_use_hls = parse("REDLIB_DEFAULT_USE_HLS");
    config.default_hide_hls_notification = parse("REDLIB_DEFAULT_HIDE_HLS_NOTIFICATION");
    config.default_hide_awards = parse("REDLIB_DEFAULT_HIDE_AWARDS");
    config.default_hide_sidebar_and_summary = parse("REDLIB_DEFAULT_HIDE_SIDEBAR_AND_SUMMARY");
    config.default_hide_score = parse("REDLIB_DEFAULT_HIDE_SCORE");
    config.default_subscriptions = parse("REDLIB_DEFAULT_SUBSCRIPTIONS");
    config.default_filters = parse("REDLIB_DEFAULT_FILTERS");
    config.default_disable_visit_reddit_confirmation = parse("REDLIB_DEFAULT_DISABLE_VISIT_REDDIT_CONFIRMATION");
    config.banner = parse("REDLIB_BANNER");
    config.robots_disable_indexing = parse("REDLIB_ROBOTS_DISABLE_INDEXING");
    config.pushshift = parse("REDLIB_PUSHSHIFT_FRONTEND");
    config.enable_rss = parse("REDLIB_ENABLE_RSS");
    config.full_url = parse("REDLIB_FULL_URL");
    config.default_remove_default_feeds = parse("REDLIB_DEFAULT_REMOVE_DEFAULT_FEEDS");
    config.default_video_quality = parse("REDLIB_DEFAULT_VIDEO_QUALITY");
    config.default_autoplay_videos = parse("REDLIB_DEFAULT_AUTOPLAY_VIDEOS");

    return config;
}

std::optional<std::string> get_setting(const std::string& name) {
    init_config();

    #define MATCH_RETURN(key, field) if (name == key) return CONFIG.field

    MATCH_RETURN("REDLIB_SFW_ONLY", sfw_only);
    MATCH_RETURN("REDLIB_DEFAULT_THEME", default_theme);
    MATCH_RETURN("REDLIB_DEFAULT_FRONT_PAGE", default_front_page);
    MATCH_RETURN("REDLIB_DEFAULT_LAYOUT", default_layout);
    MATCH_RETURN("REDLIB_DEFAULT_WIDE", default_wide);
    MATCH_RETURN("REDLIB_DEFAULT_COMMENT_SORT", default_comment_sort);
    MATCH_RETURN("REDLIB_DEFAULT_POST_SORT", default_post_sort);
    MATCH_RETURN("REDLIB_DEFAULT_BLUR_SPOILER", default_blur_spoiler);
    MATCH_RETURN("REDLIB_DEFAULT_SHOW_NSFW", default_show_nsfw);
    MATCH_RETURN("REDLIB_DEFAULT_BLUR_NSFW", default_blur_nsfw);
    MATCH_RETURN("REDLIB_DEFAULT_USE_HLS", default_use_hls);
    MATCH_RETURN("REDLIB_DEFAULT_HIDE_HLS_NOTIFICATION", default_hide_hls_notification);
    MATCH_RETURN("REDLIB_DEFAULT_HIDE_AWARDS", default_hide_awards);
    MATCH_RETURN("REDLIB_DEFAULT_HIDE_SIDEBAR_AND_SUMMARY", default_hide_sidebar_and_summary);
    MATCH_RETURN("REDLIB_DEFAULT_HIDE_SCORE", default_hide_score);
    MATCH_RETURN("REDLIB_DEFAULT_SUBSCRIPTIONS", default_subscriptions);
    MATCH_RETURN("REDLIB_DEFAULT_FILTERS", default_filters);
    MATCH_RETURN("REDLIB_DEFAULT_DISABLE_VISIT_REDDIT_CONFIRMATION", default_disable_visit_reddit_confirmation);
    MATCH_RETURN("REDLIB_BANNER", banner);
    MATCH_RETURN("REDLIB_ROBOTS_DISABLE_INDEXING", robots_disable_indexing);
    MATCH_RETURN("REDLIB_PUSHSHIFT_FRONTEND", pushshift);
    MATCH_RETURN("REDLIB_ENABLE_RSS", enable_rss);
    MATCH_RETURN("REDLIB_FULL_URL", full_url);
    MATCH_RETURN("REDLIB_DEFAULT_REMOVE_DEFAULT_FEEDS", default_remove_default_feeds);

    #undef MATCH_RETURN

    return std::nullopt;
}

} // namespace pinklib
