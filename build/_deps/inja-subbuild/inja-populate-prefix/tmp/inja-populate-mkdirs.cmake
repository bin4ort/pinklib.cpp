# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-src"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-build"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-subbuild/inja-populate-prefix"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-subbuild/inja-populate-prefix/tmp"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-subbuild/inja-populate-prefix/src/inja-populate-stamp"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-subbuild/inja-populate-prefix/src"
  "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-subbuild/inja-populate-prefix/src/inja-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-subbuild/inja-populate-prefix/src/inja-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/stry4ok/Development/Projects/pinklib-cpp/build/_deps/inja-subbuild/inja-populate-prefix/src/inja-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
