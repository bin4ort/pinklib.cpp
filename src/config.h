#pragma once

#include <string>
#include <optional>
#include <mutex>
#include <nlohmann/json.hpp>

namespace pinklib {

using json = nlohmann::json;

struct Config {
    std::optional<std::string> sfw_only;
    std::optional<std::string> default_theme;
    std::optional<std::string> default_front_page;
    std::optional<std::string> default_layout;
    std::optional<std::string> default_wide;
    std::optional<std::string> default_comment_sort;
    std::optional<std::string> default_post_sort;
    std::optional<std::string> default_blur_spoiler;
    std::optional<std::string> default_show_nsfw;
    std::optional<std::string> default_blur_nsfw;
    std::optional<std::string> default_use_hls;
    std::optional<std::string> default_hide_hls_notification;
    std::optional<std::string> default_hide_awards;
    std::optional<std::string> default_hide_sidebar_and_summary;
    std::optional<std::string> default_hide_score;
    std::optional<std::string> default_subscriptions;
    std::optional<std::string> default_filters;
    std::optional<std::string> default_disable_visit_reddit_confirmation;
    std::optional<std::string> banner;
    std::optional<std::string> robots_disable_indexing;
    std::optional<std::string> pushshift;
    std::optional<std::string> enable_rss;
    std::optional<std::string> full_url;
    std::optional<std::string> default_remove_default_feeds;
    std::optional<std::string> default_video_quality;
    std::optional<std::string> default_autoplay_videos;

    static Config load();
};

void init_config();
std::optional<std::string> get_setting(const std::string& name);

inline const char* DEFAULT_PUSHSHIFT_FRONTEND = "undelete.pullpush.io";

} // namespace pinklib
