#pragma once
#include <string>
#include <unordered_map>

namespace pinklib {

inline const std::unordered_map<std::string, std::string>& embedded_templates() {
    static const std::unordered_map<std::string, std::string> t = {
        {"base.html", R"T0000(
{% import "utils.html" as utils %}

<!DOCTYPE html>
<html lang="en" class="{% if prefs.fixed_navbar == "on" %}fixed_navbar{% endif %}">
	<head>
		{% block head %}
		<title>{% block title %}PinkLib{% endblock %}</title>
		<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
		<meta name="description" content="View on PinkLib, an alternative private front-end to Reddit.">
		<meta name="viewport" content="width=device-width, initial-scale=1.0">
		{% if crate::utils::disable_indexing() %}
		<meta name="robots" content="noindex, nofollow">
		{% endif %}
		<!-- General PWA -->
		<meta name="theme-color" content="#1F1F1F">
		<!-- iOS Application -->
		<meta name="apple-mobile-web-app-title" content="PinkLib">
		<meta name="apple-mobile-web-app-capable" content="yes">
		<meta name="apple-mobile-web-app-status-bar-style" content="default">
		<!-- Android -->
		<meta name="mobile-web-app-capable" content="yes">
		<!-- iOS Logo -->
		<link href="/touch-icon-iphone.png" rel="apple-touch-icon">
		<!-- OpenSearch description file -->
		<link rel="search" type="application/opensearchdescription+xml" title="Search PinkLib" href="/opensearch.xml">
		<!-- PWA Manifest -->
		<link rel="manifest" type="application/json" href="/manifest.json">
		<link rel="shortcut icon" type="image/x-icon" href="/favicon.ico"> 
		<link rel="stylesheet" type="text/css" href="/style.css?v={{ version }}">
		<!-- Video quality -->
		<div id="video_quality" data-value="{{ prefs.video_quality }}"></div>
		{% endblock %}
		</head>
	<body class="
		{% if prefs.layout != "" %}{{ prefs.layout }}{% endif %}
		{% if prefs.wide == "on" %} wide{% endif %}
		{% if prefs.theme != "system" %} {{ prefs.theme }}{% endif %}
		{% if prefs.fixed_navbar == "on" %} fixed_navbar{% endif %}">
		<!-- NAVIGATION BAR -->
		<nav class="
			{% if prefs.fixed_navbar == "on" %} fixed_navbar{% endif %}">
			<div id="logo">
				<a id="pinklib" href="/"><span id="pink">pink</span><span id="lib">lib.</span></a>
				{% block subscriptions %}{% endblock %}
			</div>
			{% block search %}{% endblock %}
			<div id="links">
				<a id="reddit_link" {% if prefs.disable_visit_reddit_confirmation != "on" %}href="#popup"{% else %}href="https://www.reddit.com{{ url }}" rel="nofollow"{% endif %}>
					<span>reddit</span>
					<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
						<path d="M22 2L12 22"/>
						<path d="M2 6.70587C3.33333 8.07884 3.33333 11.5971 3.33333 11.5971M3.33333 19.647V11.5971M3.33333 11.5971C3.33333 11.5971 5.125 7.47817 8 7.47817C10.875 7.47817 12 8.85114 12 8.85114"/>
					</svg>
				</a>
				{% if prefs.disable_visit_reddit_confirmation != "on" %}
					{% call utils::visit_reddit_confirmation(url) %}
				{% endif %}
				<a id="settings_link" href="/settings">
					<span>settings</span>
					<svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
						<title>settings</title>
						<circle cx="12" cy="12" r="3"/><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"/>
					</svg>
				</a>
			</div>
		</nav>
		
		<!-- MAIN CONTENT -->
		{% block body %}
		<main>
			{% block content %}
			{% endblock %}
		</main>
		{% endblock %}

		<!-- FOOTER -->
		{% block footer %}
			<footer>
				<div class="footer-buttons">
					<p><span id="version">v{{ version }}&emsp;</span><a href="/info" title="View instance information">ⓘ View instance info</a>&emsp;<a href="https://github.com/redlib-org/redlib" title="View code on GitHub">&lt;&gt; Code</a></p>
				</div>
			</footer>
		{% endblock %}
	</body>
</html>

)T0000"},
        {"comment.html", R"T0000(
{% import "utils.html" as utils %}

{% if kind == "more" && parent_kind == "t1" %}
<a class="deeper_replies" href="{{ post_link }}{{ parent_id }}">&rarr; More replies ({{ more_count }})</a>
{% else if kind == "t1" %}
<div id="{{ id }}" class="comment">
	<div class="comment_left">
    <p class="comment_score" title="{{ score.1 }}">
         {% if prefs.hide_score != "on" %}
         {{ score.0 }}
         {% else %}
         &#x2022;
         {% endif %}
    </p>
        <div class="line"></div>
	</div>
	<details class="comment_right" {% if !collapsed || highlighted %}open{% endif %}>
		<summary class="comment_data">
			{% if author.name != "[deleted]" %}
				<a class="comment_author {{ author.distinguished }} {% if author.name == post_author %}op{% endif %}" href="/user/{{ author.name }}">u/{{ author.name }}</a>
			{% else %}
				<span class="comment_author {{ author.distinguished }}">u/[deleted]</span>
			{% endif %}
			{% if author.flair.flair_parts.len() > 0 %}
				<small class="author_flair">{% call utils::render_flair(author.flair.flair_parts) %}</small>
			{% endif %}
			<a href="{{ post_link }}{{ id }}/?context=3#{{ id }}" class="created" title="{{ created }}">{{ rel_time }}</a>
			{% if edited.0 != "".to_string() %}<span class="edited" title="{{ edited.1 }}">edited {{ edited.0 }}</span>{% endif %}
			{% if !awards.is_empty() && prefs.hide_awards != "on" %}
			<span class="dot">&bull;</span>
			{% for award in awards.clone() %}
			<span class="award" title="{{ award.name }}">
				<img alt="{{ award.name }}" src="{{ award.icon_url }}" width="16" height="16"/>
			</span>
			{% endfor %}
			{% endif %}
		</summary>
		{% if is_filtered %}
		<div class="comment_body_filtered {% if highlighted %}highlighted{% endif %}">(Filtered content)</div>
		{% else %}
		<div class="comment_body {% if highlighted %}highlighted{% endif %}">{{ body|safe }}</div>
		{% endif %}
		<blockquote class="replies">{% for c in replies -%}{{ c.render().unwrap()|safe }}{%- endfor %}
		</blockquote>
	</details>
</div>
{% endif %}

)T0000"},
        {"duplicates.html", R"T0000(
{% extends "base.html" %}
{% import "utils.html" as utils %}

{% block title %}{{ post.title }} - r/{{ post.community }}{% endblock %}

{% block search %}
	{% call utils::search(["/r/", post.community.as_str()].concat(), "") %}
{% endblock %}

{% block root %}/r/{{ post.community }}{% endblock %}{% block location %}r/{{ post.community }}{% endblock %}
{% block head %}
	{% call super() %}
{% endblock %}

{% block subscriptions %}
	{% call utils::sub_list(post.community.as_str()) %}
{% endblock %}

{% block content %}
    <div id="column_one">
		{% call utils::post(post) %}

        <!-- DUPLICATES -->
        {% if post.num_duplicates == 0 %}
            <span class="listing_warn">(No duplicates found)</span>
        {% else if post.flags.nsfw && prefs.show_nsfw != "on" %}
            <span class="listing_warn">(Enable "Show NSFW posts" in <a href="/settings">settings</a> to show duplicates)</span>
        {% else %}
            <div id="duplicates_msg"><h3>Duplicates</h3></div>
            {% if num_posts_filtered > 0 %}
            <span class="listing_warn">
                {% if all_posts_filtered %}
                    (All posts have been filtered)
                {% else %}
                    (Some posts have been filtered)
                {% endif %}
            </span>
            {% endif %}

            <div id="sort">
                <div id="sort_options">
                    <a {% if params.sort.is_empty() || params.sort.eq("num_comments") %}class="selected"{% endif %} href="?sort=num_comments">
                        Number of comments
                    </a>
                    <a {% if params.sort.eq("new") %}class="selected"{% endif %} href="?sort=new">
                        New
                    </a>
                </div>
            </div>

            <div id="posts">
            {% for post in duplicates -%}
                {# TODO: utils::post should be reworked to permit a truncated display of a post as below #}
                {% if !(post.flags.nsfw) || prefs.show_nsfw == "on" %}
                <div class="post {% if post.flags.stickied %}stickied{% endif %}" id="{{ post.id }}">
                        <p class="post_header">
                            {% let community -%}
                            {% if post.community.starts_with("u_") -%}
                                {% let community = format!("u/{}", &post.community[2..]) -%}
                            {% else -%}
                                {% let community = format!("r/{}", post.community) -%}
                            {% endif -%}
                            <a class="post_subreddit" href="/r/{{ post.community }}">{{ post.community }}</a>
                            <span class="dot">&bull;</span>
                            <a class="post_author {{ post.author.distinguished }}" href="/u/{{ post.author.name }}">u/{{ post.author.name }}</a>
                            <span class="dot">&bull;</span>
                            <span class="created" title="{{ post.created }}">{{ post.rel_time }}</span>
                            {% if !post.awards.is_empty() && prefs.hide_awards != "on" %}
                                {% for award in post.awards.clone() %}
                                <span class="award" title="{{ award.name }}">
                                    <img alt="{{ award.name }}" src="{{ award.icon_url }}" width="16" height="16"/>
                                </span>
                                {% endfor %}
                            {% endif %}
                        </p>
                        <h2 class="post_title">
                            {% if post.flair.flair_parts.len() > 0 %}
                                <a href="/r/{{ post.community }}/search?q=flair_name%3A%22{{ post.flair.text }}%22&restrict_sr=on"
                                    class="post_flair"
                                    style="color:{{ post.flair.foreground_color }}; background:{{ post.flair.background_color }};"
                                    dir="ltr">{% call utils::render_flair(post.flair.flair_parts) %}</a>
                            {% endif %}
                            <a href="{{ post.permalink }}">{{ post.title }}</a>{% if post.flags.nsfw %} <small class="nsfw">NSFW</small>{% endif %}
                        </h2>
      
                        <div class="post_score" title="{{ post.score.1 }}">
                          {% if prefs.hide_score != "on" %}
                          {{ post.score.0 }}
                          {% else %}
                          &#x2022;
                          {% endif %}
                        <span class="label"> Upvotes</span></div>
                        <div class="post_footer">
                            <a href="{{ post.permalink }}" class="post_comments" title="{{ post.comments.1 }} comments">{{ post.comments.0 }} comments</a>
                        </div>

                </div>
                {% endif %}
            {%- endfor %}
            </div>

            <footer>
                {% if params.before != "" %}
                <a href="?before={{ params.before }}{% if !params.sort.is_empty() %}&sort={{ params.sort }}{% endif %}" accesskey="P">PREV</a>
                {% endif %}

                {% if params.after != "" %}
                <a href="?after={{ params.after }}{% if !params.sort.is_empty() %}&sort={{ params.sort }}{% endif %}" accesskey="N">NEXT</a>
                {% endif %}
            </footer>
        {% endif %}
    </div>
{% endblock %}

)T0000"},
        {"error.html", R"T0000(
{% extends "base.html" %}
{% block title %}Error: {{ msg }}{% endblock %}
{% block sortstyle %}{% endblock %}
{% block content %}
<div id="error">
	<h1>{{ msg }}</h1>
	<h3><a href="https://www.redditstatus.com/">Reddit Status</a></h3>
	<br />
	<h3 id="update-status"></h3>
	<br />
	<h3 id="update-status"><a id="random-instance"></a></h3>
	<br>
	<div id="git_commit" data-value="{{ crate::instance_info::INSTANCE_INFO.git_commit }}"></div>
	<script src="/check_update.js"></script>
	
	<h3>Expected something to work? <a
			href="https://github.com/redlib-org/redlib/issues/new?assignees=&labels=bug&projects=&template=bug_report.md&title=%F0%9F%90%9B+Bug+Report%3A+{{ msg }}">Report
			an issue</a></h3>
	<br />
	<p id="error-446">If you're getting a "Failed to parse page JSON data" error, please check <a href="https://github.com/redlib-org/redlib/issues/446" target="_blank">#446</a></p>
	<br />
	<h3>Head back <a href="/">home</a>?</h3>
</div>
{% endblock %}

)T0000"},
        {"info.html", R"T0000(
{% extends "base.html" %}
{% import "utils.html" as utils %}

{% block title %}Info: {{ msg }}{% endblock %}
{% block sortstyle %}{% endblock %}

{% block subscriptions %}
	{% call utils::sub_list("") %}
{% endblock %}

{% block search %}
	{% call utils::search("".to_owned(), "") %}
{% endblock %}

{% block content %}
<div id="error">
	<h2>{{ msg }}</h2>
	<br />
</div>
{% endblock %}

)T0000"},
        {"message.html", R"T0000(
{% extends "base.html" %}
{% block title %}{{ title }}{% endblock %}
{% block sortstyle %}{% endblock %}
{% block content %}
	<div id="message">
		<h1>{{ title }}</h1>
		<br>
		{{ body|safe }}
	</div>
{% endblock %}

)T0000"},
        {"nsfwlanding.html", R"T0000(
{% extends "base.html" %}
{% block title %}NSFW content gated{% endblock %}
{% block sortstyle %}{% endblock %}
{% block content %}
<div id="nsfw_landing">
    <h1>
        &#128561;
        {% if res_type == crate::utils::ResourceType::Subreddit %}
        r/{{ res }} is a NSFW community!
        {% else if res_type == crate::utils::ResourceType::User %}
        u/{{ res }}'s content is NSFW!
        {% else if res_type == crate::utils::ResourceType::Post %}
        This post is NSFW!
        {% endif %}
    </h1>
    <br />

    <p>
        {% if crate::utils::sfw_only() %}
        This instance of PinkLib is SFW-only.</p>
        {% else %}
        Enable "Show NSFW posts" in <a href="/settings">settings</a> to view this {% if res_type == crate::utils::ResourceType::Subreddit %}subreddit{% else if res_type == crate::utils::ResourceType::User %}user's posts or comments{% else if res_type == crate::utils::ResourceType::Post %}post{% endif %}. <br>
        {% if res_type == crate::utils::ResourceType::Post %} You can also quickly bypass this gate and view the post by clicking on this <a href="/settings/update/?show_nsfw=on&redirect={{url}}">link</a>.{% endif %}
        {% endif %}
    </p>
</div>
{% endblock %}
{% block footer %}
{% endblock %}


)T0000"},
        {"post.html", R"T0000(
{% extends "base.html" %}
{% import "utils.html" as utils %}

{% block title %}
	{% if single_thread %}
		{{ comments[0].author.name }} comments on {{ post.title }} - r/{{ post.community }}
	{% else %}
		{{ post.title }} - r/{{ post.community }}
	{% endif %}
{% endblock %}

{% block search %}
	{% call utils::search(["/r/", post.community.as_str()].concat(), "") %}
{% endblock %}

{% block root %}/r/{{ post.community }}{% endblock %}{% block location %}r/{{ post.community }}{% endblock %}
{% block head %}
	{% call super() %}
	<!-- Meta Tags -->
	<meta name="author" content="u/{{ post.author.name }}">
	<meta name="title" content="{{ post.title }} - r/{{ post.community }}">
	<meta property="og:title" content="{{ post.title }} - r/{{ post.community }}">
	<meta property="og:description" content="View on PinkLib, an alternative private front-end to Reddit.">
	<meta property="og:url" content="{{ post.permalink }}">
	<meta property="twitter:url" content="{{ post.permalink }}">
	<meta property="twitter:title" content="{{ post.title }} - r/{{ post.community }}">
	<meta property="twitter:description" content="View on PinkLib, an alternative private front-end to Reddit.">
	{% if post.post_type == "image" %}
	<meta property="og:type" content="image">
	<meta property="og:image" content="{{ post.thumbnail.url }}">
	<meta property="twitter:card" content="summary_large_image">
	<meta property="twitter:image" content="{{ post.thumbnail.url }}">
	{% else if post.post_type == "video" || post.post_type == "gif" %}
	<meta property="twitter:card" content="video">
	<meta property="og:type" content="video">
	<meta property="og:video" content="{{ post.media.url }}">
	<meta property="og:video:type" content="video/mp4">
	{% else %}
	<meta property="og:type" content="website">
	{% if single_thread %}
	<script src="/highlighted.js" defer></script>
	{% endif %}
	{% endif %}
{% endblock %}

{% block subscriptions %}
	{% call utils::sub_list(post.community.as_str()) %}
{% endblock %}

{% block content %}
	<div id="column_one">
		{% call utils::post(post) %}

		<!-- SORT FORM -->
       <div id="commentQueryForms">
		<form id="sort">
			<p id="comment_count">{{post.comments.0}} {% if post.comments.0 == "1" %}comment{% else %}comments{% endif %} <span id="sorted_by">sorted by </span></p>
			<select name="sort" title="Sort comments by" id="commentSortSelect"> 
				{% call utils::options(sort, ["confidence", "top", "new", "controversial", "old"], "confidence") %}
      </select>
        <button id="sort_submit" class="submit">
          <svg width="15" viewBox="0 0 110 100" fill="none" stroke-width="10" stroke-linecap="round">
              <path d="M20 50 H100" />
              <path d="M75 15 L100 50 L75 85" />
              &rarr;
          </svg>
      </button>
      </form>
      <!-- SEARCH FORM -->
      <form id="sort">
        <input id="search" class="commentQuery" type="search" name="q" value="{{ comment_query }}" placeholder="Search comments">
        <input type="hidden" name="type" value="comment">
      </form>
      </div>
  
      <div>
      {% if comment_query != "" %}
      Comments containing "{{ comment_query }}"&nbsp;|&nbsp;<a id="allCommentsLink" href="{{ url_without_query }}">All comments</a>
      {% endif %}
      </div>

		<!-- COMMENTS -->
		{% for c in comments -%}
		<div class="thread">
			{% if single_thread %}
			<p class="thread_nav"><a href="{{ post.permalink }}">View all comments</a></p>
			{% if c.parent_kind == "t1" %}
			<p class="thread_nav"><a href="?context=9999">Show parent comments</a></p>
			{% endif %}
			{% endif %}
			
			{{ c.render().unwrap()|safe }}
		</div>
		{%- endfor %}

	</div>
{% endblock %}

)T0000"},
        {"search.html", R"T0000(
{% extends "base.html" %}
{% import "utils.html" as utils %}

{% block title %}PinkLib: search results - {{ params.q }}{% endblock %}

{% block subscriptions %}
	{% call utils::sub_list("") %}
{% endblock %}

{% block content %}
	<div id="column_one">
		<form id="search_sort">
			<div class="search_widget_divider_box">
				<input id="search" type="text" name="q" placeholder="Search" value="{{ params.q|safe }}" title="Search redlib">
				<div class="search_widget_divider_box">
					{% if sub != "" %}
					<div id="inside">
						<input type="checkbox" name="restrict_sr" id="restrict_sr" {% if params.restrict_sr != "" %}checked{% endif %}>
						<label for="restrict_sr" class="search_label">in r/{{ sub }}</label>
					</div>
					{% endif %}
					{% if params.typed == "sr_user" %}<input type="hidden" name="type" value="sr_user">{% endif %}
					<select id="sort_options" name="sort" title="Sort results by">
						{% call utils::options(params.sort, ["relevance", "hot", "top", "new", "comments"], "") %}
					</select>
					{% if params.sort != "new" %}
					<select id="timeframe" name="t" title="Timeframe"> 
						{% call utils::options(params.t, ["hour", "day", "week", "month", "year", "all"], "all") %}
					</select>
					{% endif %}
				</div>
			</div>

			<button id="sort_submit" class="submit">
				<svg width="15" viewBox="0 0 110 100" fill="none" stroke-width="10" stroke-linecap="round">
					<path d="M20 50 H100" />
					<path d="M75 15 L100 50 L75 85" />
					&rarr;
				</svg>
			</button>
 		</form>

		{% if !is_filtered %}
		{% if subreddits.len() > 0 || params.typed == "sr_user" %}
		<div id="search_subreddits">
			{% if params.typed == "sr_user" %}
			<a href="?q={{ params.q }}&sort={{ params.sort }}&t={{ params.t }}" class="search_subreddit" id="more_subreddits">← Back to post/comment results</a>
			{% endif %}
			{% for subreddit in subreddits %}
			<a href="{{ subreddit.url }}" class="search_subreddit">
				<div class="search_subreddit_left">{% if subreddit.icon != "" %}<img loading="lazy" src="{{ subreddit.icon|safe }}" alt="r/{{ subreddit.name }} icon">{% endif %}</div>
				<div class="search_subreddit_right">
					<p class="search_subreddit_header"> 
						<span class="search_subreddit_name">r/{{ subreddit.name }}</span>
						<span class="dot">&bull;</span>
						<span class="search_subreddit_members" title="{{ subreddit.subscribers.1 }} Members">{{ subreddit.subscribers.0 }} Members</span>
					</p>
					<p class="search_subreddit_description">{{ subreddit.description }}</p>
				</div>
			</a>
			{% endfor %}
			{% if params.typed != "sr_user" %}
			<a href="?q={{ params.q }}&sort={{ params.sort }}&t={{ params.t }}&type=sr_user" class="search_subreddit" id="more_subreddits">More subreddit results →</a>
			{% endif %}
		</div>
		{% endif %}
		{% endif %}

		{% if all_posts_hidden_nsfw %}
		<span class="listing_warn">All posts are hidden because they are NSFW. Enable "Show NSFW posts" in settings to view.</span>
		{% endif %}

		{% if no_posts %}
			<center>No posts were found.</center>
		{% endif %}

		{% if all_posts_filtered %}
			<span class="listing_warn">(All content on this page has been filtered)</span>
		{% else if is_filtered %}
			<span class="listing_warn">(Content from r/{{ sub }} has been filtered)</span>
		{% else if params.typed != "sr_user" %}
			{% for post in posts %}
				{% if post.flags.nsfw && prefs.show_nsfw != "on" %}
				{% else if !post.title.is_empty() %}
					{% call utils::post_in_list(post) %}
				{% else %}
					<div class="comment">
						<div class="comment_left">
							<p class="comment_score" title="{{ post.score.1 }}">
                            {% if prefs.hide_score != "on" %}
                            {{ post.score.0 }}
                            {% else %}
                            &#x2022;
                            {% endif %}
                            </p>
							<div class="line"></div>
						</div>
						<details class="comment_right" open>
							<summary class="comment_data">
								<a class="comment_link" href="{{ post.permalink }}">COMMENT</a>
								<span class="created" title="{{ post.created }}">{{ post.rel_time }}</span>
							</summary>
							<p class="comment_body">{{ post.body }}</p>
						</details>
					</div>
				{% endif %}
			{% endfor %}
		{% endif %}
		{% if prefs.use_hls == "on" %}
		<script src="/hls.min.js"></script>
		<script src="/playHLSVideo.js"></script>
		{% endif %}

		{% if params.typed != "sr_user" %}
		<footer>
			{% if params.before != "" %}
			<a href="?q={{ params.q|safe }}&restrict_sr={{ params.restrict_sr }}
				&sort={{ params.sort }}&t={{ params.t }}
				&before={{ params.before }}" accesskey="P">PREV</a>
			{% endif %}

			{% if params.after != "" %}
			<a href="?q={{ params.q|safe }}&restrict_sr={{ params.restrict_sr }}
				&sort={{ params.sort }}&t={{ params.t }}
				&after={{ params.after }}" accesskey="N">NEXT</a>
			{% endif %}
		</footer>
		{% endif %}
	</div>
{% endblock %}

)T0000"},
        {"settings.html", R"T0000(
{% extends "base.html" %}
{% import "utils.html" as utils %}

{% block title %}PinkLib Settings{% endblock %}

{% block subscriptions %}
{% call utils::sub_list("") %}
{% endblock %}

{% block search %}
{% call utils::search("".to_owned(), "") %}
{% endblock %}

{% block content %}
<div id="settings">
	<form action="/settings" method="POST">
		<div class="prefs">
			<fieldset>
				<legend>Appearance</legend>
				<div class="prefs-group">
					<label for="theme">Theme:</label>
					<select name="theme" id="theme">
						{% call utils::options(prefs.theme, prefs.available_themes, "system") %}
					</select>
				</div>
			</fieldset>
			<fieldset>
				<legend>Interface</legend>
				<div class="prefs-group">
					<label for="remove_default_feeds">Remove default feeds</label>
					<input type="hidden" value="off" name="remove_default_feeds">
					<input type="checkbox" name="remove_default_feeds" id="remove_default_feeds" {% if
						prefs.remove_default_feeds=="on" %}checked{% endif %}>
				</div>
				<div class="prefs-group">
					<label for="front_page">Front page:</label>
					<select name="front_page" id="front_page">
						{% call utils::options(prefs.front_page, ["default", "popular", "all"], "default") %}
					</select>
				</div>
				<div class="prefs-group">
					<label for="layout">Layout:</label>
					<select name="layout" id="layout">
						{% call utils::options(prefs.layout, ["card", "clean", "compact"], "card") %}
					</select>
				</div>
				<div class="prefs-group">
					<label for="wide">Wide UI:</label>
					<input type="hidden" value="off" name="wide">
					<input type="checkbox" name="wide" id="wide" {% if prefs.wide=="on" %}checked{% endif %}>
				</div>
			</fieldset>
			<fieldset>
				<legend>Content</legend>
				<div class="prefs-group">
					<label for="video_quality">Video quality:</label>
					<select name="video_quality" id="video_quality">
						{% call utils::options(prefs.video_quality, ["best", "medium", "worst"], "best") %}
					</select>
				</div>
				<div class="prefs-group">
					<label for="post_sort" title="Applies only to subreddit feeds">Default subreddit post sort:</label>
					<select name="post_sort">
						{% call utils::options(prefs.post_sort, ["hot", "new", "top", "rising", "controversial"], "hot")
						%}
					</select>
				</div>
				<div class="prefs-group">
					<label for="comment_sort">Default comment sort:</label>
					<select name="comment_sort" id="comment_sort">
						{% call utils::options(prefs.comment_sort, ["confidence", "top", "new", "controversial", "old"],
						"confidence") %}
					</select>
				</div>
				<div class="prefs-group">
					<label for="blur_spoiler">Blur spoiler previews:</label>
					<input type="hidden" value="off" name="blur_spoiler">
					<input type="checkbox" name="blur_spoiler" id="blur_spoiler" {% if prefs.blur_spoiler=="on"
						%}checked{% endif %}>
				</div>
				{% if !crate::utils::sfw_only() %}
				<div class="prefs-group">
					<label for="show_nsfw">Show NSFW posts:</label>
					<input type="hidden" value="off" name="show_nsfw">
					<input type="checkbox" name="show_nsfw" id="show_nsfw" {% if prefs.show_nsfw=="on" %}checked{% endif
						%}>
				</div>
				<div class="prefs-group">
					<label for="blur_nsfw">Blur NSFW previews:</label>
					<input type="hidden" value="off" name="blur_nsfw">
					<input type="checkbox" name="blur_nsfw" id="blur_nsfw" {% if prefs.blur_nsfw=="on" %}checked{% endif
						%}>
				</div>
				{% endif %}
				<div class="prefs-group">
					<label for="autoplay_videos">Autoplay videos</label>
					<input type="hidden" value="off" name="autoplay_videos">
					<input type="checkbox" name="autoplay_videos" id="autoplay_videos" {% if prefs.autoplay_videos=="on"
						%}checked{% endif %}>
				</div>
				<div class="prefs-group">
					<label for="fixed_navbar">Keep navbar fixed</label>
					<input type="hidden" value="off" name="fixed_navbar">
					<input type="checkbox" name="fixed_navbar" {% if prefs.fixed_navbar=="on" %}checked{% endif %}>
				</div>
				<div class="prefs-group">
					<label for="hide_sidebar_and_summary">Hide the summary and sidebar</label>
					<input type="hidden" value="off" name="hide_sidebar_and_summary">
					<input type="checkbox" name="hide_sidebar_and_summary" {% if prefs.hide_sidebar_and_summary=="on"
						%}checked{% endif %}>
				</div>
				<div class="prefs-group">
					<label for="use_hls">Use HLS for videos</label>
					<details id="feeds">
						<summary>Why?</summary>
						<div id="feed_list" class="helper">Reddit videos require JavaScript (via HLS.js) to be enabled
							to be played with audio. Therefore, this toggle lets you either use PinkLib JS-free or
							utilize this feature.</div>
					</details>
					<input type="hidden" value="off" name="use_hls">
					<input type="checkbox" name="use_hls" id="use_hls" {% if prefs.use_hls=="on" %}checked{% endif %}>
				</div>
				<div class="prefs-group">
					<label for="hide_hls_notification">Hide notification about possible HLS usage</label>
					<input type="hidden" value="off" name="hide_hls_notification">
					<input type="checkbox" name="hide_hls_notification" id="hide_hls_notification" {% if
						prefs.hide_hls_notification=="on" %}checked{% endif %}>
				</div>
				<div class="prefs-group">
					<label for="hide_awards">Hide awards</label>
					<input type="hidden" value="off" name="hide_awards">
					<input type="checkbox" name="hide_awards" id="hide_awards" {% if prefs.hide_awards=="on" %}checked{%
						endif %}>
				</div>
				<div class="prefs-group">
					<label for="hide_score">Hide score</label>
					<input type="hidden" value="off" name="hide_score">
					<input type="checkbox" name="hide_score" id="hide_score" {% if prefs.hide_score=="on" %}checked{%
						endif %}>
				</div>
				<div class="prefs-group">
					<label for="disable_visit_reddit_confirmation">Do not confirm before visiting content on
						Reddit</label>
					<input type="hidden" value="off" name="disable_visit_reddit_confirmation">
					<input type="checkbox" name="disable_visit_reddit_confirmation" {% if
						prefs.disable_visit_reddit_confirmation=="on" %}checked{% endif %}>
				</div>
			</fieldset>
			<input id="save" type="submit" value="Save">
		</div>
	</form>
	{% if prefs.subscriptions.len() > 0 %}
	<div class="prefs" id="settings_subs">
		<legend>Subscribed Feeds</legend>
		{% for sub in prefs.subscriptions %}
		<div>
			{% let feed -%}
			{% if sub.starts_with("u_") -%}{% let feed = format!("u/{}", &sub[2..]) -%}{% else -%}{% let feed =
			format!("r/{}", sub) -%}{% endif -%}
			<a href="/{{ feed }}">{{ feed }}</a>
			<form action="/r/{{ sub }}/unsubscribe/?redirect=settings" method="POST">
				<button class="unsubscribe">Unsubscribe</button>
			</form>
		</div>
		{% endfor %}
	</div>
	{% endif %}
	{% if !prefs.filters.is_empty() %}
	<div class="prefs" id="settings_filters">
		<legend>Filtered Feeds</legend>
		{% for sub in prefs.filters %}
		<div>
			{% let feed -%}
			{% if sub.starts_with("u_") -%}{% let feed = format!("u/{}", &sub[2..]) -%}{% else -%}{% let feed =
			format!("r/{}", sub) -%}{% endif -%}
			<a href="/{{ feed }}">{{ feed }}</a>
			<form action="/r/{{ sub }}/unfilter/?redirect=settings" method="POST">
				<button class="unfilter">Unfilter</button>
			</form>
		</div>
		{% endfor %}
	</div>
	{% endif %}

	<div id="settings_note">
		<p><b>Note:</b> settings and subscriptions are saved in browser cookies. Clearing your cookies will reset them.
		</p>
		<br>
		{% match prefs.to_urlencoded() %}
		{% when Ok with (encoded_prefs) %}
		<p>You can restore your current settings and subscriptions after clearing your cookies using <a
				href="/settings/restore/?{{ encoded_prefs }}">this link</a>.</p>
		{% when Err with (err) %}
		<p>There was an error creating your restore link: {{ err }}</p>
		<p>Please report this issue</p>
		{% endmatch %}
		<br />
		<div>
			<script src="/copy.js"></script>
			<label for="bincode_str">Or, export/import here (be sure to save first):</label>
			<br />
			<input type="text" id="bincode_str" name="bincode_str"
				value="{% match prefs.to_bincode_str() %}{% when Ok with (bincode_str) %}{{ bincode_str }}{% when Err with (err) %}Error: {{ err }}{% endmatch %}"
				readonly>
			<button id="copy" class="copy">Copy</button>

			<br />
			<form action="/settings/encoded-restore/" method="POST">
				<input type="text" id="encoded_prefs" name="encoded_prefs" value=""
					placeholder="Paste your encoded settings here">
				<button class="import" type="submit">Import</button>
			</form>
		</div>
	</div>
</div>

{% endblock %}
)T0000"},
        {"subreddit.html", R"T0000(
{% extends "base.html" %}
{% import "utils.html" as utils %}

{% block title %}
	{% if sub.title != "" %}{{ sub.title }}
	{% else if sub.name != "" %}{{ sub.name }}
	{% else %}PinkLib{% endif %}
{% endblock %}

{% block search %}
	{% call utils::search(["/r/", sub.name.as_str()].concat(), "") %}
{% endblock %}

{% block subscriptions %}
	{% call utils::sub_list(sub.name.as_str()) %}
{% endblock %}

{% block body %}
	<main>
		{% if !is_filtered %}
		<div id="column_one">
			<form id="sort">
				<div id="sort_options">
					{% if sub.name.is_empty() %}
						{% call utils::sort("", ["hot", "new", "top", "rising", "controversial"], sort.0) %}
					{% else %}
						{% call utils::sort(["/r/", sub.name.as_str()].concat(), ["hot", "new", "top", "rising", "controversial"], sort.0) %}
					{% endif %}
				</div>
				{% if sort.0 == "top" || sort.0 == "controversial" %}<select id="timeframe" name="t" title="Timeframe">
					{% call utils::options(sort.1, ["hour", "day", "week", "month", "year", "all"], "day") %}
				</select>
				<button id="sort_submit" class="submit">
					<svg width="15" viewBox="0 0 110 100" fill="none" stroke-width="10" stroke-linecap="round">
						<path d="M20 50 H100" />
						<path d="M75 15 L100 50 L75 85" />
						&rarr;
					</svg>
				</button>
				{% endif %}
			</form>

			{% if sub.name.contains("+") %}
				<form action="/r/{{ sub.name }}/subscribe?redirect={{ redirect_url }}" method="POST">
					<button id="multisub" class="subscribe" title="Subscribe to each sub in this multireddit">Subscribe to Multireddit</button>
				</form>
			{% endif %}

			{% if all_posts_hidden_nsfw %}
			<center>All posts are hidden because they are NSFW. Enable "Show NSFW posts" in settings to view.</center>
			{% endif %}

			{% if no_posts %}
			<center>No posts were found.</center>
			{% endif %}

			{% if all_posts_filtered %}
				 <center>(All content on this page has been filtered)</center>
			{% else %}
			<div id="posts">
			{% for post in posts %}
			{% if !(post.flags.nsfw && prefs.show_nsfw != "on") %}
			<hr class="sep" />
			{% call utils::post_in_list(post) %}
			{% endif %}
			{% endfor %}
			{% if prefs.use_hls == "on" %}
			<script src="/hls.min.js"></script>
			<script src="/playHLSVideo.js"></script>
			{% endif %}
			</div>
			{% endif %}

			<footer>
				{% if !ends.0.is_empty() %}
				<a href="?sort={{ sort.0 }}&t={{ sort.1 }}&before={{ ends.0 }}" accesskey="P">PREV</a>
				{% endif %}

				{% if !ends.1.is_empty() %}
				<a href="?sort={{ sort.0 }}&t={{ sort.1 }}&after={{ ends.1 }}" accesskey="N">NEXT</a>
				{% endif %}
			</footer>
		</div>
		{% endif %}
		{% if is_filtered || (!sub.name.is_empty() && sub.name != "all" && sub.name != "popular" && !sub.name.contains("+")) && prefs.hide_sidebar_and_summary != "on" %}
		<aside>
			{% if is_filtered %}
				<center>(Content from r/{{ sub.name }} has been filtered)</center>
			{% endif %}
			{% if !sub.name.is_empty() && sub.name != "all" && sub.name != "popular" && !sub.name.contains("+") %}
			<details class="panel" id="subreddit" open>
				<summary id="subreddit_label">Subreddit</summary>
				{% if sub.wiki %}
				<div id="top">
					<div>Posts</div>
					<a href="/r/{{ sub.name }}/wiki/index">Wiki</a>
				</div>
				{% endif %}
				<div id="sub_meta">
					<img loading="lazy" id="sub_icon" src="{{ sub.icon }}" alt="Icon for r/{{ sub.name }}">
					<h1 id="sub_title">{{ sub.title }}</h1>
					<p id="sub_name">r/{{ sub.name }}</p>
					<p id="sub_description">{{ sub.description }}</p>
					<div id="sub_details">
						<label>Members</label>
						<label>Active</label>
						<div title="{{ sub.members.1 }}">{{ sub.members.0 }}</div>
						<div title="{{ sub.active.1 }}">{{ sub.active.0 }}</div>
					</div>
					<div id="sub_actions">
						<div id="sub_subscription">
							{% if prefs.subscriptions.contains(sub.name) %}
								<form action="/r/{{ sub.name }}/unsubscribe?redirect={{ redirect_url }}" method="POST">
									<button class="unsubscribe">Unsubscribe</button>
								</form>
							{% else %}
								<form action="/r/{{ sub.name }}/subscribe?redirect={{ redirect_url }}" method="POST">
									<button class="subscribe">Subscribe</button>
								</form>
							{% endif %}
						</div>
						<div id="sub_filter">
							{% if prefs.filters.contains(sub.name) %}
								<form action="/r/{{ sub.name }}/unfilter?redirect={{ redirect_url }}" method="POST">
									<button class="unfilter">Unfilter</button>
								</form>
							{% else %}
							<form action="/r/{{ sub.name }}/filter?redirect={{ redirect_url }}" method="POST">
								<button class="filter">Filter</button>
							</form>
							{% endif %}
						</div>
						{% if crate::utils::enable_rss() %}
						<div id="sub_rss">
                            <a href="/r/{{ sub.name }}.rss" title="RSS feed for r/{{ sub.name }}">
                                <button class="subscribe">RSS feed</button >
                            </a>
						</div>
						{% endif %}
				</div>
			</details>
			<details class="panel" id="sidebar">
				<summary id="sidebar_label">Sidebar</summary>
				<div id="sidebar_contents">
					{{ sub.info|safe }}
					{# <hr>
					<h2>Moderators</h2>
					<br>
					<ul>
					{% for moderator in sub.moderators %}
					<li><a style="color: var(--accent)" href="/u/{{ moderator }}">{{ moderator }}</a></li>
					{% endfor %}
					</ul> #}
				</div>
			</details>
			{% endif %}
		</aside>
		{% endif %}
	</main>
{% endblock %}

)T0000"},
        {"user.html", R"T0000(
{% extends "base.html" %} {% import "utils.html" as utils %} {% block search %}
{% call utils::search("".to_owned(), "") %} {% endblock %} {% block title %}{{
user.name.replace("u/", "") }} (u/{{ user.name }}) - PinkLib{% endblock %} {%
block subscriptions %} {% call utils::sub_list("") %} {% endblock %} {% block
body %}
<main>
    {% if !is_filtered %}
    <div id="column_one">
        <form id="sort">
            <div id="listing_options">
                {% call utils::sort(["/user/", user.name.as_str()].concat(),
                ["overview", "comments", "submitted"], listing) %}
            </div>
            <select id="sort_select" name="sort">
                {% call utils::options(sort.0, ["hot", "new", "top",
                "controversial"], "") %}</select
            >{% if sort.0 == "top" || sort.0 == "controversial" %}<select
                id="timeframe"
                name="t"
            >
                {% call utils::options(sort.1, ["hour", "day", "week", "month",
                "year", "all"], "all") %}</select
            >{% endif %}<button id="sort_submit" class="submit">
                <svg
                    width="15"
                    viewBox="0 0 110 100"
                    fill="none"
                    stroke-width="10"
                    stroke-linecap="round"
                >
                    <path d="M20 50 H100" />
                    <path d="M75 15 L100 50 L75 85" />
                    &rarr;
                </svg>
            </button>
        </form>

        {% if all_posts_hidden_nsfw %}
        <center>
            All posts are hidden because they are NSFW. Enable "Show NSFW posts"
            in settings to view.
        </center>
        {% endif %} {% if no_posts %}
        <center>No posts were found.</center>
        {% endif %} {% if all_posts_filtered %}
        <center>(All content on this page has been filtered)</center>
        {% else %}
        <div id="posts">
            {% for post in posts %} {% if post.flags.nsfw && prefs.show_nsfw !=
            "on" %} {% else if !post.title.is_empty() %} {% call
            utils::post_in_list(post) %} {% else %}
            <div class="comment user-comment">
                <div class="comment_left">
                    <p class="comment_score" title="{{ post.score.1 }}">
                        {% if prefs.hide_score != "on" %} {{ post.score.0 }} {%
                        else %} &#x2022; {% endif %}
                    </p>
                    <div class="line"></div>
                </div>
                <details class="comment_right" open>
                    <summary class="comment_data">
                        <a
                            class="comment_link"
                            href="{{ post.permalink }}#{{ post.id }}"
                            title="{{ post.link_title }}"
                            >{{ post.link_title }}</a
                        >
                        <div class="user_comment_data_divider">
                            <span class="created-in">&nbsp;in&nbsp;</span>
                            <a
                                class="comment_subreddit"
                                href="/r/{{ post.community }}"
                                >r/{{ post.community }}</a
                            >
                            <span class="dot">&bull;</span>
                            <span class="created" title="{{ post.created }}"
                                >&nbsp;{{ post.rel_time }}</span
                            >
                        </div>
                    </summary>
                    <p class="comment_body">{{ post.body|safe }}</p>
                </details>
            </div>
            {% endif %} {% endfor %} {% if prefs.use_hls == "on" %}
            <script src="/hls.min.js"></script>
            <script src="/playHLSVideo.js"></script>
            {% endif %}
        </div>
        {% endif %}

        <footer>
            {% if ends.0 != "" %}
            <a
                href="?sort={{ sort.0 }}&t={{ sort.1 }}&before={{ ends.0 }}"
                accesskey="P"
                >PREV</a
            >
            {% endif %} {% if ends.1 != "" %}
            <a
                href="?sort={{ sort.0 }}&t={{ sort.1 }}&after={{ ends.1 }}"
                accesskey="N"
                >NEXT</a
            >
            {% endif %}
        </footer>
    </div>
    {% endif %}
    <aside>
        {% if is_filtered %}
        <center>(Content from u/{{ user.name }} has been filtered)</center>
        {% endif %}
        <div class="panel" id="user">
            <img
                loading="lazy"
                id="user_icon"
                src="{{ user.icon }}"
                alt="User icon"
            />
            <h1 id="user_title">{{ user.title }}</h1>
            <p id="user_name">u/{{ user.name }}</p>
            <div id="user_description">{{ user.description }}</div>
            <div id="user_details">
                <label>Karma</label>
                <label>Created</label>
                <div>{{ user.karma }}</div>
                <div>{{ user.created }}</div>
            </div>
            <div id="user_actions">
                {% let name = ["u_", user.name.as_str()].join("") %}
                <div id="user_subscription">
                    {% if prefs.subscriptions.contains(name) %}
                    <form
                        action="/r/{{ name }}/unsubscribe?redirect={{ redirect_url }}"
                        method="POST"
                    >
                        <button class="unsubscribe">Unfollow</button>
                    </form>
                    {% else %}
                    <form
                        action="/r/{{ name }}/subscribe?redirect={{ redirect_url }}"
                        method="POST"
                    >
                        <button class="subscribe">Follow</button>
                    </form>
                    {% endif %}
                </div>
                <div id="user_filter">
                    {% if prefs.filters.contains(name) %}
                    <form
                        action="/r/{{ name }}/unfilter?redirect={{ redirect_url }}"
                        method="POST"
                    >
                        <button class="unfilter">Unfilter</button>
                    </form>
                    {% else %}
                    <form
                        action="/r/{{ name }}/filter?redirect={{ redirect_url }}"
                        method="POST"
                    >
                        <button class="filter">Filter</button>
                    </form>
                    {% endif %}
                </div>
                {% if crate::utils::enable_rss() %}
                <div id="user_rss">
                    <a
                        href="/u/{{ user.name }}.rss"
                        title="RSS feed for u/{{ user.name }}"
                    >
                        <button class="subscribe">RSS feed</button>
                    </a>
                </div>
                {% endif %}
            </div>
        </div>
    </aside>
</main>
{% endblock %}

)T0000"},
        {"utils.html", R"T0000(
{% macro options(current, values, default) -%}
	{% for value in values %}
		<option value="{{ value }}" {% if current == value.to_string() || (current == "" && value.to_string() == default.to_string()) %}selected{% endif %}>
			{{ format!("{}{}", value.get(0..1).unwrap_or_default().to_uppercase(), value.get(1..).unwrap_or_default()) }}
		</option>
	{% endfor %}
{%- endmacro %}

{% macro sort(root, methods, selected) -%}
	{% for method in methods %}
		<a {% if method.to_string() == selected.to_string() %}class="selected"{% endif %} href="{{ root }}/{{ method }}">
			{{ format!("{}{}", method.get(0..1).unwrap_or_default().to_uppercase(), method.get(1..).unwrap_or_default()) }}
		</a>
	{% endfor %}
{%- endmacro %}

{% macro search(root, search) -%}
<form action="{% if root != "/r/" && !root.is_empty() %}{{ root }}{% endif %}/search" id="searchbox">
	<input id="search" type="text" name="q" placeholder="Search" title="Search PinkLib" value="{{ search }}">
	{% if root != "/r/" && !root.is_empty() %}
	<div id="inside">
		<input type="checkbox" name="restrict_sr" id="restrict_sr" checked>
		<label for="restrict_sr" class="search_label" title="Restrict search to this subreddit">in {{ root }}</label>
	</div>
	{% endif %}
	<button class="submit">
		<svg width="15" viewBox="0 0 110 100" fill="none" stroke-width="10" stroke-linecap="round">
			<path d="M20 50 H100" />
			<path d="M75 15 L100 50 L75 85" />
			&rarr;
		</svg>
	</button>
</form>
{%- endmacro %}

{% macro render_flair(flair_parts) -%}
	{% for flair_part in flair_parts.clone() %}{% if flair_part.flair_part_type == "emoji" %}<span class="emoji" style="background-image:url('{{ flair_part.value }}');"></span>{% else if flair_part.flair_part_type == "text" && !flair_part.value.is_empty() %}<span>{{ flair_part.value }}</span>{% endif %}{% endfor %}
{%- endmacro %}

{% macro sub_list(current) -%}
	<details id="feeds">
		<summary>Feeds</summary>
		<div id="feed_list">
			<p>MAIN FEEDS</p>
			<a href="/">Home</a>
			{% if prefs.remove_default_feeds != "on" %}
				<a href="/r/popular">Popular</a>
				<a href="/r/all">All</a>
			{% endif %}
			{% if prefs.subscriptions.len() > 0 %}
				<p>REDDIT FEEDS</p>
				{% for sub in prefs.subscriptions %}
					<a href="/r/{{ sub }}" {% if sub == current %}class="selected"{% endif %}>{{ sub }}</a>
				{% endfor %}
			{% endif %}
		</div>
	</details>
{%- endmacro %}

{% macro render_hls_notification(redirect_url) -%}
{% if post.post_type == "video" && !post.media.alt_url.is_empty() && prefs.hide_hls_notification != "on" %}
<div class="post_notification"><p><a href="/settings/update/?use_hls=on&redirect={{ redirect_url }}">Enable HLS</a> to view with audio, or <a href="/settings/update/?hide_hls_notification=on&redirect={{ redirect_url }}">disable this notification</a></p></div>
{% endif %}
{%- endmacro %}

{% macro post(post) -%}
{% set post_should_be_blurred = post.flags.spoiler && prefs.blur_spoiler=="on" -%}
<!-- POST CONTENT -->
<div class="post highlighted{% if post_should_be_blurred %} post_blurred{% endif %}">
	<p class="post_header">
		<a class="post_subreddit" href="/r/{{ post.community }}">r/{{ post.community }}</a>
		<span class="dot">&bull;</span>
		<a class="post_author {{ post.author.distinguished }}" href="/user/{{ post.author.name }}">u/{{ post.author.name }}</a>
		{% if post.author.flair.flair_parts.len() > 0 %}
			<small class="author_flair">{% call render_flair(post.author.flair.flair_parts) %}</small>
		{% endif %}
		<span class="dot">&bull;</span>
		<span class="created" title="{{ post.created }}">{{ post.rel_time }}</span>
		{% if !post.awards.is_empty() && prefs.hide_awards != "on" %}
		<span class="dot">&bull;</span>
		<span class="awards">
			{% for award in post.awards.clone() %}
			<span class="award" title="{{ award.name }}">
				<img alt="{{ award.name }}" src="{{ award.icon_url }}" width="16" height="16"/>
				{{ award.count }}
			</span>
			{% endfor %}
		</span>
		{% endif %}
	</p>
	<h1 class="post_title">
		{% if post.flair.flair_parts.len() > 0 %}
			<a href="/r/{{ post.community }}/search?q=flair_name%3A%22{{ post.flair.text }}%22&restrict_sr=on"
				class="post_flair"
				style="color:{{ post.flair.foreground_color }}; background:{{ post.flair.background_color }};">{% call render_flair(post.flair.flair_parts) %}</a>
		{% endif %}
		{{ post.title }}
		{% if post.flags.nsfw %} <small class="nsfw">NSFW</small>{% endif %}
		{% if post.flags.spoiler %} <small class="spoiler">Spoiler</small>{% endif %}
	</h1>

	<!-- POST MEDIA -->
	<!-- post_type: {{ post.post_type }} -->
	{% if post.post_type == "image" %}
	<div class="post_media_content">
		<a href="{{ post.media.url }}" class="post_media_image" >
			{% if post.media.height == 0 || post.media.width == 0 %}
			<!-- i.redd.it images special case -->
			<img width="100%" height="100%" loading="lazy" alt="Post image" src="{{ post.media.url }}"/>
			{% else %}
			<svg
				width="{{ post.media.width }}px"
				height="{{ post.media.height }}px"
				xmlns="http://www.w3.org/2000/svg">
					<image width="100%" height="100%" href="{{ post.media.url }}"/>
					<desc>
						<img loading="lazy" alt="Post image" src="{{ post.media.url }}"/>
					</desc>
			</svg>
			{% endif %}
		</a>
	</div>
	{% else if post.post_type == "video" || post.post_type == "gif" %}
	{% if prefs.use_hls == "on" && !post.media.alt_url.is_empty() %}
	<script src="/hls.min.js"></script>
	<div class="post_media_content">
		<video class="post_media_video short {% if prefs.autoplay_videos == "on" %}hls_autoplay{% endif %}" {% if post.media.width > 0 && post.media.height > 0 %}width="{{ post.media.width }}" height="{{ post.media.height }}"{% endif %} poster="{{ post.media.poster }}" preload="none" controls>
			<source src="{{ post.media.alt_url }}" type="application/vnd.apple.mpegurl" />
			<source src="{{ post.media.url }}" type="video/mp4" />
		</video>
	</div>
	<script src="/playHLSVideo.js"></script>
	{% else %}
	<div class="post_media_content">
		<video class="post_media_video" src="{{ post.media.url }}" controls {% if prefs.autoplay_videos == "on" %}autoplay{% endif %} loop><a href={{ post.media.url }}>Video</a></video>
	</div>
	{% call render_hls_notification(post.permalink[1..]) %}
	{% endif %}
	{% else if post.post_type == "gallery" %}
	<div class="gallery">
	{% for image in post.gallery -%}
		<figure>
			<a href="{{ image.url }}" ><img loading="lazy" alt="Gallery image" src="{{ image.url }}"/></a>
			<figcaption>
				<p>{{ image.caption }}</p>
				{% if image.outbound_url.len() > 0 %}
				<p><a class="outbound_url" href="{{ image.outbound_url }}" rel="nofollow">{{ image.outbound_url }}</a>
				{% endif %}
			</figcaption>
		</figure>
	{%- endfor %}
	</div>
	{% else if post.post_type == "link" %}
	<a id="post_url" href="{{ post.media.url }}" rel="nofollow">{{ post.media.url }}</a>
	{% endif %}

	<!-- POST BODY -->
	<div class="post_body">
		{{ post.body|safe }}
		{% call poll(post) %}
	</div>
	<div class="post_score" title="{{ post.score.1 }}">
    {% if prefs.hide_score != "on" %}
    {{ post.score.0 }}
    {% else %}
    &#x2022;
    {% endif %}
    <span class="label"> Upvotes</span></div>
	<div class="post_footer">
		<ul id="post_links">
			<li>
				<a href="{{ post.permalink }}">
					<span class="desktop_item">perma</span>link
				</a>
			</li>
			{% if post.num_duplicates > 0 %}
			<li>
				<a href="/r/{{ post.community }}/duplicates/{{ post.id }}">
					dup<span class="desktop_item">licat</span>es
				</a>
			</li>
			{% endif %}
			{% if post.post_type == "link" %}
			<li class="desktop_item"><a target="_blank" href="https://archive.is/latest/{{ post.media.url }}">archive.is</a></li>
			<li class="mobile_item"><a target="_blank" href="https://archive.is/latest/{{ post.media.url }}">archive</a></li>
			{% endif %}
			{% call external_reddit_link(post.permalink) %}

			{% if post.media.download_name != "" %}
			<li>
				<a href="{{ post.media.url }}" download="{{ post.media.download_name }}">
					<span class="mobile_item">dl</span>
					<span class="desktop_item">download</span>
				</a>
			</li>
			{% endif %}
		</ul>
		<p>{{ post.upvote_ratio }}%<span id="upvoted"> Upvoted</span></p>
	</div>
</div>
{%- endmacro %}

{% macro external_reddit_link(permalink) %}
<li>
	<a
		{% if prefs.disable_visit_reddit_confirmation != "on" %}
		href="#popup"
		{% else %}
		href="https://reddit.com{{ permalink }}"
		rel="nofollow"
		{% endif %}
	>reddit</a>

	{% if prefs.disable_visit_reddit_confirmation != "on" %}
		{% call visit_reddit_confirmation(permalink) %}
	{% endif %}
</li>
{% endmacro %}

{% macro post_in_list(post) -%}
{% set post_should_be_blurred = (post.flags.nsfw && prefs.blur_nsfw=="on") || (post.flags.spoiler && prefs.blur_spoiler=="on") -%}
<div class="post{% if post.flags.stickied %} stickied{% endif %}{% if post_should_be_blurred %} post_blurred{% endif %}" id="{{ post.id }}">
	<p class="post_header">
		{% let community -%}
		{% if post.community.starts_with("u_") -%}
			{% let community = format!("u/{}", &post.community[2..]) -%}
		{% else -%}
			{% let community = format!("r/{}", post.community) -%}
		{% endif -%}
		<a class="post_subreddit" href="/{{ community }}">{{ community }}</a>
		<span class="dot">&bull;</span>
		<a class="post_author {{ post.author.distinguished }}" href="/u/{{ post.author.name }}">u/{{ post.author.name }}</a>
		<span class="dot">&bull;</span>
		<span class="created" title="{{ post.created }}">{{ post.rel_time }}</span>
		{% if !post.awards.is_empty() && prefs.hide_awards != "on" %}
			{% for award in post.awards.clone() %}
			<span class="award" title="{{ award.name }}">
				<img alt="{{ award.name }}" src="{{ award.icon_url }}" width="16" height="16"/>
			</span>
			{% endfor %}
		{% endif %}
	</p>
	<h2 class="post_title">
		{% if post.flair.flair_parts.len() > 0 %}
			<a href="/r/{{ post.community }}/search?q=flair_name%3A%22{{ post.flair.text }}%22&restrict_sr=on"
				class="post_flair"
				style="color:{{ post.flair.foreground_color }}; background:{{ post.flair.background_color }};"
				dir="ltr">{% call render_flair(post.flair.flair_parts) %}</a>
		{% endif %}
		<a href="{{ post.permalink }}">{{ post.title }}</a>{% if post.flags.nsfw %} <small class="nsfw">NSFW</small>{% endif %}{% if post.flags.spoiler %} <small class="spoiler">Spoiler</small>{% endif %}
	</h2>
	<!-- POST MEDIA/THUMBNAIL -->
	{% if (prefs.layout.is_empty() || prefs.layout == "card") && post.post_type == "image" %}
	<div class="post_media_content">
		<a href="{{ post.media.url }}" class="post_media_image {% if post.media.height < post.media.width*2 %}short{% endif %}" >
			{% if post.media.height == 0 || post.media.width == 0 %}
			<!-- i.redd.it images speical case -->
			<img width="100%" height="100%" loading="lazy" alt="Post image" src="{{ post.media.url }}"/>
			{% else %}
			<svg
				width="{{ post.media.width }}px"
				height="{{ post.media.height }}px"
				xmlns="http://www.w3.org/2000/svg">
					<image width="100%" height="100%" href="{{ post.media.url }}"/>
					<desc>
						<img loading="lazy" alt="Post image" src="{{ post.media.url }}"/>
					</desc>
			</svg>
			{% endif %}
		</a>
	</div>
	{% else if (prefs.layout.is_empty() || prefs.layout == "card") && (post.post_type == "gif" || post.post_type == "video") %}
	{% if prefs.use_hls == "on" && !post.media.alt_url.is_empty() %}
	<div class="post_media_content">
        <video class="post_media_video short{% if prefs.autoplay_videos == "on" %} hls_autoplay{% endif %}" {% if post.media.width > 0 && post.media.height > 0 %}width="{{ post.media.width }}" height="{{ post.media.height }}"{% endif %} poster="{{ post.media.poster }}" controls preload="none">
			<source src="{{ post.media.alt_url }}" type="application/vnd.apple.mpegurl" />
			<source src="{{ post.media.url }}" type="video/mp4" />
		</video>
	</div>
	{% else %}
	<div class="post_media_content">
		<video class="post_media_video short" src="{{ post.media.url }}" {% if post.media.width > 0 && post.media.height > 0 %}width="{{ post.media.width }}" height="{{ post.media.height }}"{% endif %} poster="{{ post.media.poster }}" preload="none" controls {% if prefs.autoplay_videos == "on" %}autoplay{% endif %}><a href={{ post.media.url }}>Video</a></video>
	</div>
	{% call render_hls_notification(format!("{}%23{}", &self.url[1..].replace("&", "%26").replace("+", "%2B"), post.id)) %}
	{% endif %}
	{% else if post.post_type != "self" %}
	<a class="post_thumbnail{% if post.thumbnail.url.is_empty() %} no_thumbnail{% endif %}" href="{% if post.post_type == "link" %}{{ post.media.url }}{% else %}{{ post.permalink }}{% endif %}" rel="nofollow">
		{% if post.thumbnail.url.is_empty() %}
		<svg viewBox="0 0 100 106" width="140" height="53" xmlns="http://www.w3.org/2000/svg">
			<title>Thumbnail</title>
			<path d="M35,15h-15a10,10 0,0,0 0,20h25a10,10 0,0,0 10,-10m-12.5,0a10, 10 0,0,1 10, -10h25a10,10 0,0,1 0,20h-15" fill="none" stroke-width="5" stroke-linecap="round"/>
		</svg>
		{% else %}
		<div style="max-width:{{ post.thumbnail.width }}px;max-height:{{ post.thumbnail.height }}px;">
			<svg width="{{ post.thumbnail.width }}px" height="{{ post.thumbnail.height }}px" xmlns="http://www.w3.org/2000/svg">
				<image width="100%" height="100%" href="{{ post.thumbnail.url }}"/>
				<desc>
					<img loading="lazy" alt="Thumbnail" src="{{ post.thumbnail.url }}"/>
				</desc>
			</svg>
		</div>
		{% endif %}
		<span>{% if post.post_type == "link" %}{{ post.domain }}{% else %}{{ post.post_type }}{% endif %}</span>
	</a>
	{% endif %}
	<div class="post_score" title="{{ post.score.1 }}">
    {% if prefs.hide_score != "on" %}
    {{ post.score.0 }}
    {% else %}
    &#x2022;
    {% endif %}
    <span class="label"> Upvotes</span></div>
	<div class="post_body post_preview">
		{{ post.body|safe }}
	</div>

	{% call poll(post) %}

	<div class="post_footer">
		<a href="{{ post.permalink }}" class="post_comments" title="{{ post.comments.1 }} {% if post.comments.1 == "1" %}comment{% else %}comments{% endif %}">{{ post.comments.0 }} {% if post.comments.1 == "1" %}comment{% else %}comments{% endif %}</a>
	</div>
</div>
{%- endmacro %}

{% macro visit_reddit_confirmation(url) -%}
<div class="popup" id="popup">
	<div class="popup-inner">
		<h1>You are about to leave PinkLib</h1>
		<p>Do you want to continue?</p>
		<p id="reddit_url">https://www.reddit.com{{ url }}</p>
		<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 639.24 563">
			<defs>
				<style>.cls-1{fill:#000000;}.cls-2{fill:#f8aa00;}</style>
			</defs>
			<path class="cls-2" d="M322.03,0c1.95,2.5,4.88,.9,7.33,1.65,10.5,3.21,17.65,10.39,22.83,19.35,93.64,162.06,186.98,324.29,280.25,486.56,15.73,20.19,2.49,51.27-22.92,54.37-1.21,.19-2.72-.54-3.49,1.08H239.03c-70.33-2.43-141.6,.79-212.08-1.74-17.49-4.92-23.16-15.88-26.91-32.26l-.04-1.97C88.74,354.76,194.49,188.2,289.92,18.43c6.2-10.66,15.03-16.94,27.61-17.36,.95-.03,2.05,.18,2.51-1.07h2Zm-2.43,545c94.95-.02,189.9,.04,284.85-.02,11.84-.73,20.75-13.19,16.68-23.55C523.83,355.97,430.74,187.62,332.05,23.07c-7.93-9.02-22.2-6.58-27.23,3.22C230.28,156.11,155.21,285.64,80.41,415.31c-19.88,34.41-39.31,69.07-59.78,103.14-2.43,4.05-4.24,8.8-1.68,14.18,3.92,8.24,9.59,12.37,18.82,12.37,93.95,0,187.9,0,281.85,0Z"/>
			<path class="cls-1" d="M319.61,545c-93.95,0-187.9,0-281.85,0-9.22,0-14.89-4.13-18.82-12.37-2.56-5.38-.75-10.13,1.68-14.18,20.47-34.07,39.9-68.73,59.78-103.14C155.21,285.64,230.28,156.11,304.82,26.29c5.03-9.8,19.3-12.24,27.23-3.22,98.7,164.55,191.79,332.9,289.1,498.35,4.06,10.36-4.85,22.82-16.68,23.55-94.94,.06-189.9,0-284.85,.02Zm.44-462.31C238.88,223.22,158.17,362.95,77.28,503h485.54c-80.94-140.13-161.61-279.79-242.77-420.31Z"/>
			<path class="cls-2" d="M320.05,82.69c81.16,140.52,161.83,280.18,242.77,420.31H77.28C158.17,362.95,238.88,223.22,320.05,82.69Zm36.05,118.99c-.14-46.75-68.32-52.32-74.66-4.76,.73,51.49,9.2,102.97,12.63,154.49,1.18,13.14,10.53,21.81,23.32,22.76,13.12,.97,23.89-9.13,24.96-21.58,4.44-49.99,9.4-101.22,13.76-150.91Zm-36.56,271.4c48.8,.76,49.24-74.7-.31-75.47-53.45,3-46.02,78.12,.31,75.47Z"/>
			<path class="cls-1" d="M356.1,201.67c-4.36,49.69-9.31,100.91-13.76,150.91-1.07,12.45-11.84,22.56-24.96,21.58-12.79-.95-22.14-9.63-23.31-22.76-3.43-51.52-11.9-103-12.63-154.49,6.33-47.53,74.51-42.03,74.66,4.76Z"/>
			<path class="cls-1" d="M319.54,473.08c-46.34,2.64-53.75-72.47-.31-75.47,49.56,.78,49.1,76.24,.31,75.47Z"/>
		</svg>
		<a id="goback" href="#">No, go back!</a>
		<a id="toreddit" href="https://www.reddit.com{{ url }}" rel="nofollow">Yes, take me to Reddit</a>
	</div>
</div>
{%- endmacro %}

{% macro poll(post) -%}
	{% match post.poll %}
		{% when Some with (poll) %}
			{% let widest = poll.most_votes() %}
			<div class="post_poll">
				<span>{{ poll.total_vote_count }} votes,</span>
				<span title="{{ poll.voting_end_timestamp.1 }}">{{ poll.voting_end_timestamp.0 }}</span>
				{% for option in poll.poll_options %}
				<div class="poll_option">
					{# Posts without vote_count (all open polls) will show up without votes.
						This is an issue with Reddit API, it doesn't work on Old Reddit either. #}
					{% match option.vote_count %}
						{% when Some with (vote_count) %}
							{% if vote_count.eq(widest) || widest == 0 %}
								<div class="poll_chart most_voted"></div>
							{% else %}
								<div class="poll_chart" style="width: {{ (vote_count * 100) / widest }}%"></div>
							{% endif %}
							<span>{{ vote_count }}</span>
						{% when None %}
							<div class="poll_chart most_voted"></div>
							<span></span>
					{% endmatch %}
					<span>{{ option.text }}</span>
				</div>
				{% endfor %}
			</div>
		{% when None %}
	{% endmatch %}
{%- endmacro %}

)T0000"},
        {"wall.html", R"T0000(
{% extends "base.html" %}
{% block title %}{{ msg }}{% endblock %}
{% block sortstyle %}{% endblock %}
{% block content %}
	<div id="wall">
		<h1>{{ title }}</h1>
		<br>
		<p>{{ msg }}</p>
		<form action="/r/{{ sub }}?redir={{ url }}" method="POST">
			<input id="save" type="submit" value="Continue">
		</form>
	</div>
{% endblock %}

)T0000"},
        {"wiki.html", R"T0000(
{% extends "base.html" %}
{% import "utils.html" as utils %}

{% block title %}
	{% if sub != "" %}{{ page }} - {{ sub }}
	{% else %}PinkLib{% endif %}
{% endblock %}

{% block search %}
	{% call utils::search(["/r/", sub.as_str()].concat(), "") %}
{% endblock %}

{% block subscriptions %}
	{% call utils::sub_list(sub.as_str()) %}
{% endblock %}

{% block body %}
	<main>
		<div class="panel" id="column_one">
			<div id="top">
				<a href="/r/{{ sub }}">Posts</a>
				<div>Wiki</div>
			</div>
			<div id="wiki">
				{{ wiki|safe }}
			</div>
		</div>
	</main>
{% endblock %}

)T0000"},
    };
    return t;
}

} // namespace pinklib
