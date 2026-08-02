# PinkLib

A private front-end for Reddit written in C++ (originally translated from [Redlib](https://github.com/redlib-org/redlib)).

PinkLib proxies Reddit content through its own server, removing trackers and providing a clean, fast interface. The default color accent is pink.

## Features

- Browse Reddit without JavaScript, cookies, or tracking
- Subreddit browsing, post viewing, comments, search, user profiles
- RSS feeds for subreddits and users
- Configurable themes (dark, light, and 15+ color themes)
- NSFW content filtering (SFW-only mode available)
- Subreddit subscriptions and filtering
- Brotli and gzip response compression
- OAuth-based Reddit API access

## Building

### Requirements

- CMake ≥ 3.20
- C++20 compiler (GCC ≥ 11 or Clang ≥ 14)
- OpenSSL development headers
- zlib development headers
- Python 3 (for template embedding)

### Build

```sh
cmake -B build -S .
cmake --build build -j$(nproc)
```

All other dependencies (httplib, nlohmann/json, inja, tomlplusplus, brotli) are fetched automatically by CMake.

The resulting binary is `build/pinklib`.

## Running

```sh
./build/pinklib -a 0.0.0.0 -p 8080
```

### Command-line options

| Flag | Description |
|---|---|
| `-a`, `--address ADDRESS` | Address to listen on (default: `[::]`) |
| `-p`, `--port PORT` | Port to listen on (default: `8080`, or `$PORT`) |
| `-4`, `--ipv4-only` | Listen on IPv4 only |
| `-6`, `--ipv6-only` | Listen on IPv6 only |
| `-h`, `--help` | Show help |

### Configuration

Configuration is loaded from (in order of precedence):

1. Environment variables (e.g., `REDLIB_DEFAULT_THEME=dark`)
2. `pinklib.toml` file
3. `redlib.toml` (legacy)
4. `libreddit.toml` (legacy)

See the [Redlib configuration documentation](https://github.com/redlib-org/redlib) for available settings.

## Project Structure

```
pinklib-cpp/
├── CMakeLists.txt          # Build system
├── LICENSE                 # AGPL-3.0
├── README.md
├── scripts/
│   └── embed_templates.py # Embeds HTML templates into C++ at build time
├── src/
│   ├── main.cpp            # Entry point, CLI args, route setup
│   ├── config.h/cpp        # Configuration loading (env vars, TOML)
│   ├── server.h/cpp        # HTTP server with routing and compression
│   ├── client.h/cpp        # Reddit API HTTP client
│   ├── oauth.h/cpp         # Reddit OAuth authentication
│   ├── oauth_resources.h/cpp
│   ├── utils.h/cpp         # Data types, formatting, URL rewriting
│   ├── subreddit.h/cpp     # Subreddit page handlers
│   ├── handlers.h/cpp      # Post, user, search, settings handlers
│   └── templates_embedded.h # Generated — HTML templates in C++ literals
├── static/
│   ├── style.css           # Main stylesheet (pink accent)
│   ├── themes/             # Theme CSS files
│   ├── *.js                # Client-side JavaScript
│   └── *.png, *.svg, *.ico # Static assets
└── templates/
    └── *.html              # Inja template files
```

## Dependencies

| Library | Purpose |
|---|---|
| [cpp-httplib](https://github.com/yhirose/cpp-httplib) | HTTP server and client |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON parsing |
| [inja](https://github.com/pantor/inja) | HTML template rendering |
| [tomlplusplus](https://github.com/marzer/tomlplusplus) | TOML configuration parsing |
| [Brotli](https://github.com/google/brotli) | Brotli compression |
| OpenSSL | TLS/SSL |
| zlib | gzip compression |

## License

PinkLib is licensed under the [GNU Affero General Public License v3.0](LICENSE).

This project is a translation of [Redlib](https://github.com/redlib-org/redlib), which is also AGPL-3.0 licensed.

## Credits

Original project: [Redlib](https://github.com/redlib-org/redlib) by Matthew Esposito, spikecodes, and contributors.
