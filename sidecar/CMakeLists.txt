cmake_minimum_required(VERSION 3.10)
project(beagle_sidecar VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Carrier SDK (adjust path as needed)
pkg_check_modules(CARRIER REQUIRED elacarrier)

# HTTP server library (using cpp-httplib header-only library)
include(FetchContent)
FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.14.3
)
FetchContent_MakeAvailable(httplib)

# JSON library (nlohmann/json)
FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

# WebSocket library (uWebSockets)
FetchContent_Declare(
  uwebsockets
  GIT_REPOSITORY https://github.com/uNetworking/uWebSockets.git
  GIT_TAG v20.48.0
)
FetchContent_MakeAvailable(uwebsockets)

# Source files
set(SOURCES
  src/main.cpp
  src/sidecar_server.cpp
  src/carrier_client.cpp
  src/config.cpp
  src/utils.cpp
)

# Executable
add_executable(beagle-sidecar ${SOURCES})

# Include directories
target_include_directories(beagle-sidecar PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CARRIER_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(beagle-sidecar PRIVATE
  ${CARRIER_LIBRARIES}
  Threads::Threads
  httplib::httplib
  nlohmann_json::nlohmann_json
)

# Installation
install(TARGETS beagle-sidecar
  RUNTIME DESTINATION bin
)

# Install systemd service file
install(FILES systemd/beagle-sidecar.service
  DESTINATION /lib/systemd/system
)

# Install default config
install(FILES config/default-config.json
  DESTINATION /etc/beagle-sidecar
  RENAME config.json
)
