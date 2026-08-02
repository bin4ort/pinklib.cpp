#pragma once

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <nlohmann/json.hpp>

namespace pinklib {

inline const char* REDDIT_URL_BASE = "https://oauth.reddit.com";
inline const char* REDDIT_URL_BASE_HOST = "oauth.reddit.com";
inline const char* ALTERNATIVE_REDDIT_URL_BASE = "https://www.reddit.com";
inline const char* ALTERNATIVE_REDDIT_URL_BASE_HOST = "www.reddit.com";

using json = nlohmann::json;

// ---- Enums ----
enum class ResourceType { Subreddit, User, Post };

// ---- Data Types ----

struct FlairPart {
    std::string flair_part_type;
    std::string value;
    static std::vector<FlairPart> parse(const std::string& flair_type,
            const json* rich_flair, const std::string& text_flair);
};

struct Flair {
    std::vector<FlairPart> flair_parts;
    std::string text;
    std::string background_color;
    std::string foreground_color;
};

struct Author {
    std::string name;
    Flair flair;
    std::string distinguished;
};

struct PollOption {
    uint64_t id = 0;
    std::string text;
    std::optional<uint64_t> vote_count;
    static std::vector<PollOption> parse(const json& options);
};

struct Poll {
    std::vector<PollOption> poll_options;
    std::pair<std::string, std::string> voting_end_timestamp;
    uint64_t total_vote_count = 0;
    static std::optional<Poll> parse(const json& poll_data);
    uint64_t most_votes() const;
};

struct Flags {
    bool spoiler = false;
    bool nsfw = false;
    bool stickied = false;
};

struct GalleryMedia {
    std::string url;
    int64_t width = 0;
    int64_t height = 0;
    std::string caption;
    std::string outbound_url;
    static std::vector<GalleryMedia> parse(const json& items, const json& metadata);
};

struct Media {
    std::string url;
    std::string alt_url;
    int64_t width = 0;
    int64_t height = 0;
    std::string poster;
    std::string download_name;
    static std::tuple<std::string, Media, std::vector<GalleryMedia>> parse(const json& data);
};

struct Award {
    std::string name;
    std::string icon_url;
    std::string description;
    int64_t count = 0;
};

struct Awards : public std::vector<Award> {
    static Awards parse(const json& items);
};

struct Comment;

struct Preferences {
    std::vector<std::string> available_themes;
    std::string theme;
    std::string front_page;
    std::string layout;
    std::string wide;
    std::string blur_spoiler;
    std::string show_nsfw;
    std::string blur_nsfw;
    std::string hide_hls_notification;
    std::string video_quality;
    std::string hide_sidebar_and_summary;
    std::string use_hls;
    std::string autoplay_videos;
    std::string fixed_navbar;
    std::string disable_visit_reddit_confirmation;
    std::string comment_sort;
    std::string post_sort;
    std::vector<std::string> subscriptions;
    std::vector<std::string> filters;
    std::string hide_awards;
    std::string hide_score;
    std::string remove_default_feeds;

    static Preferences from_cookies(const std::unordered_map<std::string, std::string>& cookies);
    void populate_available_themes();
    std::string to_urlencoded() const;
    std::string to_bincode_str() const;
    nlohmann::json to_json() const;

private:
    static std::string pref_setting(const std::unordered_map<std::string, std::string>& cookies,
                                    const std::string& name);
};

struct Post {
    std::string id;
    std::string title;
    std::string community;
    std::string body;
    Author author;
    std::string permalink;
    std::string link_title;
    std::optional<Poll> poll;
    std::pair<std::string, std::string> score;
    int64_t upvote_ratio = 0;
    std::string post_type;
    Flair flair;
    Flags flags;
    Media thumbnail;
    Media media;
    std::string domain;
    std::string rel_time;
    std::string created;
    uint64_t created_ts = 0;
    uint64_t num_duplicates = 0;
    std::pair<std::string, std::string> comments;
    std::vector<GalleryMedia> gallery;
    Awards awards;
    bool nsfw = false;
    std::optional<std::string> out_url;
    std::string ws_url;

    static std::pair<std::vector<Post>, std::string> fetch(const std::string& path, bool quarantine);
};

struct Comment {
    std::string id;
    std::string kind;
    std::string parent_id;
    std::string parent_kind;
    std::string post_link;
    std::string post_author;
    std::string body;
    Author author;
    std::pair<std::string, std::string> score;
    std::string rel_time;
    std::string created;
    std::pair<std::string, std::string> edited;
    std::vector<Comment> replies;
    bool highlighted = false;
    Awards awards;
    bool collapsed = false;
    bool is_filtered = false;
    int64_t more_count = 0;
    Preferences prefs;
};

struct UserData {
    std::string name;
    std::string title;
    std::string icon;
    int64_t karma = 0;
    std::string created;
    std::string banner;
    std::string description;
    bool nsfw = false;
};

struct SubredditData {
    std::string name;
    std::string title;
    std::string description;
    std::string info;
    std::string icon;
    std::pair<std::string, std::string> members;
    std::pair<std::string, std::string> active;
    bool wiki = false;
    bool nsfw = false;
};

// ---- Template Data ----
struct ErrorTemplateData {
    std::string msg;
    Preferences prefs;
    std::string url;
};

struct InfoTemplateData {
    std::string msg;
    Preferences prefs;
    std::string url;
};

struct NSFWLandingTemplateData {
    std::string res;
    ResourceType res_type;
    Preferences prefs;
    std::string url;
};

// ---- Utility Functions ----
std::string format_url(const std::string& url);
std::string rewrite_urls(const std::string& input_text);
std::string rewrite_emotes(const json& media_metadata, const std::string& comment);
std::string render_bullet_lists(const std::string& input_text);
std::string val(const json& j, const std::string& k);
std::pair<std::string, std::string> format_num(int64_t num);
std::pair<std::string, std::string> time_str(double created);
std::optional<std::string> param(const std::string& path, const std::string& value);
std::string url_path_basename(const std::string& path);
std::string get_post_url(const Post& post);
std::string to_absolute_url(const std::string& relative_path);
bool sfw_only();
bool enable_rss();
bool disable_indexing();
bool should_be_nsfw_gated(const std::string& show_nsfw);

Post parse_post(const json& post);
std::string render_template(const std::string& template_name, const json& data);
std::string deflate_compress(const std::vector<uint8_t>& input);
std::string deflate_decompress(const std::vector<uint8_t>& input);

void filter_posts(std::vector<Post>& posts,
                  const std::unordered_set<std::string>& filters,
                  uint64_t& num_filtered, bool& all_filtered);

// ---- Cookie Helpers ----
std::unordered_map<std::string, std::string> parse_cookies(const std::string& cookie_header);
std::string get_cookie_val(const std::unordered_map<std::string, std::string>& cookies,
                           const std::string& name);

} // namespace pinklib
