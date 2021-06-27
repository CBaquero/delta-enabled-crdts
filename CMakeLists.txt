cmake_minimum_required(VERSION 3.1.0)

# Adhere the version number
project(delta-enabled-crdts VERSION 0.1.0 LANGUAGES CXX)

enable_language(ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Compile in C++11 mode
if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
  set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

  set(CMAKE_CXX_STANDARD 11)  # force std = c++11
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

include_directories(
  ${CMAKE_BINARY_DIR}/
  ${CMAKE_SOURCE_DIR}/include/
  ${CMAKE_SOURCE_DIR}/include/crdts
)


# Targets
set(DELTA_CRDTS_LIBNAME delta-enabled-crdts)

# To build and install a shared library: "cmake -DBUILD_SHARED_LIBS:BOOL=ON ..."
add_library(${DELTA_CRDTS_LIBNAME}
  src/delta-crdts.cc
)

set_target_properties(${DELTA_CRDTS_LIBNAME} PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  ARCHIVE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib
  LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

include(GNUInstallDirs)
set(INCLUDE_INSTALL_DIR "${CMAKE_INSTALL_INCLUDEDIR}/delta-enabled-crdts" )
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/delta-enabled-crdts" )
set(RUNTIME_INSTALL_DIR "${CMAKE_INSTALL_BINDIR}" )
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}" )
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_DATADIR}/delta-enabled-crdts")

install(TARGETS ${DELTA_CRDTS_LIBNAME} EXPORT deltaCrdtsConfig
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FRAMEWORK DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(
  DIRECTORY ${CMAKE_SOURCE_DIR}/include/crdts
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES include/crdts/delta-crdts.h DESTINATION include/crdts)

export(
  TARGETS ${DELTA_CRDTS_LIBNAME}
  FILE deltaCrdtsConfig.cmake
)
# Tests, use "CTEST_OUTPUT_ON_FAILURE=1 make test" to see output from failed tests

# https://cmake.org/cmake/help/v3.0/module/CTest.html
include(CTest)

if(TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
