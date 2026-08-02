# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-src"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-build"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-subbuild/brotli-populate-prefix"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-subbuild/brotli-populate-prefix/tmp"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-subbuild/brotli-populate-prefix/src/brotli-populate-stamp"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-subbuild/brotli-populate-prefix/src"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-subbuild/brotli-populate-prefix/src/brotli-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-subbuild/brotli-populate-prefix/src/brotli-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/brotli-subbuild/brotli-populate-prefix/src/brotli-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
