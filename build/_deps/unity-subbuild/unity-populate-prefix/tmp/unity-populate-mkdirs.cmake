# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-src"
  "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-build"
  "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-subbuild/unity-populate-prefix"
  "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-subbuild/unity-populate-prefix/tmp"
  "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-subbuild/unity-populate-prefix/src/unity-populate-stamp"
  "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-subbuild/unity-populate-prefix/src"
  "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-subbuild/unity-populate-prefix/src/unity-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-subbuild/unity-populate-prefix/src/unity-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/facu/SO1/Sistemas-Operativos-1/build/_deps/unity-subbuild/unity-populate-prefix/src/unity-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
