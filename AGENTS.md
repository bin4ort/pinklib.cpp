# AGENTS.md

Guidelines for code assistants working on the PinkLib C++ codebase.

## Project Overview

PinkLib is a C++20 translation of [Redlib](https://github.com/redlib-org/redlib), a private front-end for Reddit. The original Rust source is at `redlib-src/` (git submodule) for reference.

## Build

```sh
# Requires: cmake ≥ 3.20, C++20 compiler, OpenSSL, zlib, curl, python3
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
# Binary: build/pinklib
```

All C++ deps (httplib, nlohmann/json, inja, tomlplusplus, brotli) are fetched via `FetchContent`. Templates are embedded at build time by `scripts/embed_templates.py`.

## Run

```sh
./build/pinklib                    # defaults: 127.0.0.1:8095
./build/pinklib -a 0.0.0.0 -p 8080
```

## Architecture

```
src/
├── main.cpp             # Entry point, CLI args, route registration
├── config.h/cpp         # TOML + env var configuration
├── server.h/cpp         # HTTP server (wraps cpp-httplib), routing, content-type detection
├── client.h/cpp         # Reddit API HTTP client (uses libcurl), JSON fetching
├── oauth.h/cpp          # Reddit OAuth token acquisition (client ID: ohXpoqrZYub1kg)
├── oauth_resources.h/cpp # Android app version list for device spoofing
├── utils.h/cpp          # Data types (Post, Comment, User, Subreddit, etc.), formatters, template engine
├── subreddit.h/cpp      # Subreddit page, wiki, sidebar, RSS, subscriptions
├── handlers.h/cpp       # Post, user, search, settings, duplicates, instance info
├── templates_embedded.h  # Generated — HTML templates as C++ raw string literals
├── static_embedded.h     # Generated — CSS/JS/fonts/icons as C++ strings
static/                  # Source static files (embedded at build time)
templates/               # Source HTML templates (embedded at build time)
scripts/embed_templates.py
redlib-src/              # Original Rust source (git submodule, reference only)
```

## Key Design Decisions

### HTTP Client: libcurl (not cpp-httplib)

Reddit blocks non-browser TLS fingerprints. cpp-httplib's custom TLS was detected and returned 403. Switched to libcurl which uses system OpenSSL with a standard fingerprint that passes Reddit's checks.

### Template Engine: inja + custom pre-processor

The original Redlib uses Askama (Rust-specific). inja (Jinja2-like for C++) doesn't support:
- `{% extends %}` / `{% block %}` — resolved by `resolve_extends()` in utils.cpp
- `{% macro %}` / `{% call %}` — expanded by `parse_macros()` / `expand_calls()`
- `crate::utils::*()` — converted to inja variables by regex

The settings, error, info, and NSFW landing pages bypass templates entirely and generate HTML directly in C++ to avoid template compatibility issues.

### OAuth

Reddit requires OAuth for API access. The token is fetched at startup from `www.reddit.com/api/v1/access_token` using Redlib's Android app client ID (`ohXpoqrZYub1kg`). The token is used for requests to `oauth.reddit.com`.

### Templates

Templates are embedded in the binary via `templates_embedded.h`. Static files (CSS, JS, images) are embedded via `static_embedded.h`. Both are generated at build time by Python scripts. The server never reads from filesystem at runtime for these assets.

## Common Issues

### "Reddit API returned 403"

libcurl is needed, not cpp-httplib. Ensure curl dev headers are installed (`libcurl-devel` / `libcurl4-openssl-dev`).

### Template Error: "failed accessing file"

inja's `{% include %}` triggers filesystem access. All includes must be resolved by the pre-processor before passing to inja. Add inline resolution in `render_template()` if needed.

### "[json.exception.type_error] type must be string"

A JSON field was accessed with `.get<std::string>()` but the value is null. Always guard with `.is_string()` or `.value("key", "default")`.

### "[json.exception.type_error] cannot use operator[] with a string argument with null"

Accessing `null_json["key"]` where `null_json` is a null JSON value. Common in `data["preview"]["images"]` when preview doesn't exist. Use `get_nested()` helper or check `.contains()` / `.is_null()` before chaining `[]`.

### Server hangs on start

OAuth token fetch runs in a background thread. Server starts immediately. If OAuth fails, pages that need Reddit data show error pages. Settings/info/about work without Reddit API.

## Adding New Routes

Register routes in `main.cpp` before the catch-all `/:path`:

```cpp
// Static page:
app.get("/mypage", pinklib::RequestHandler([](auto&, auto& path, auto& params, auto& query, auto&, auto& cookies) {
    return "Hello";
}));

// Reddit-dependent page:
app.at("/r/:sub/mypage", pinklib::RequestHandler([](auto&, auto& path, auto& params, auto& query, auto&, auto& cookies) {
    json data = reddit_json("/r/" + params.at("sub") + "/mypage.json?raw_json=1", false);
    // ... build response ...
}));
```

## Configuration

Set via env vars (highest priority), `pinklib.toml`, `redlib.toml`, or `libreddit.toml`:

```toml
REDLIB_DEFAULT_THEME = "dark"
REDLIB_SFW_ONLY = "off"
REDLIB_ENABLE_RSS = "on"
REDLIB_FULL_URL = "https://pinklib.example.com"
```

See `config.h` for all options.
