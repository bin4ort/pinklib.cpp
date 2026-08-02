cmake_minimum_required(VERSION 3.20)
project(pinklib VERSION 0.36.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Dependencies via FetchContent ----
include(FetchContent)

# inja template engine
set(INJA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INJA_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(INJA_INSTALL OFF CACHE BOOL "" FORCE)
set(INJA_USE_EMBEDDED_JSON ON CACHE BOOL "" FORCE)
FetchContent_Declare(inja
  GIT_REPOSITORY https://github.com/pantor/inja.git
  GIT_TAG v3.4.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(inja)

# cpp-httplib
FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(httplib)

# inja template engine (includes nlohmann/json embedded)
set(INJA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(INJA_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(INJA_INSTALL OFF CACHE BOOL "" FORCE)
set(INJA_USE_EMBEDDED_JSON ON CACHE BOOL "" FORCE)
FetchContent_Declare(inja
  GIT_REPOSITORY https://github.com/pantor/inja.git
  GIT_TAG v3.4.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(inja)

# tomlplusplus
FetchContent_Declare(tomlplusplus
  GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
  GIT_TAG v3.4.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(tomlplusplus)

# ---- Find system packages ----
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# ---- Brotli ----
FetchContent_Declare(brotli
  GIT_REPOSITORY https://github.com/google/brotli.git
  GIT_TAG v1.1.0
)
FetchContent_MakeAvailable(brotli)

# ---- Source files ----
set(PINKLIB_SOURCES
  src/main.cpp
  src/config.cpp
  src/server.cpp
  src/client.cpp
  src/utils.cpp
  src/subreddit.cpp
  src/handlers.cpp
  src/oauth.cpp
  src/oauth_resources.cpp
)

set(PINKLIB_HEADERS
  src/config.h
  src/server.h
  src/client.h
  src/utils.h
  src/oauth.h
  src/oauth_resources.h
  src/handlers.h
  src/subreddit.h
)

add_executable(pinklib ${PINKLIB_SOURCES} ${PINKLIB_HEADERS})

target_include_directories(pinklib PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${inja_SOURCE_DIR}/third_party/include
  ${inja_SOURCE_DIR}/single_include
)

target_link_libraries(pinklib PRIVATE
  httplib::httplib
  inja
  tomlplusplus::tomlplusplus
  OpenSSL::SSL
  OpenSSL::Crypto
  ZLIB::ZLIB
  brotlienc
  brotlidec
  brotlicommon
)

# Enable threading
find_package(Threads REQUIRED)
target_link_libraries(pinklib PRIVATE Threads::Threads)

# ---- Compiler flags ----
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(pinklib PRIVATE -Wall -Wextra -Wpedantic -O2)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(pinklib PRIVATE -O3 -flto)
  target_link_options(pinklib PRIVATE -flto -s)
endif()
