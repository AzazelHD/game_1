cmake_minimum_required(VERSION 3.20)
project(TRPG VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Root CMakeLists is intentionally an orchestrator only.
# It configures external dependencies and engine discovery, then delegates target
# definition to game/CMakeLists.txt so there is exactly one source of truth for
# the executable source list.

# Development policy:
# - Debug game builds should consume engine artifacts directly from engine/build.
# - Release game builds should consume the installed engine package from install/.
#
# Paths are cache variables so they can be overridden per machine without editing
# this file.
set(TRPG_USE_INSTALLED_ENGINE OFF CACHE BOOL "Use installed engine package instead of engine build-tree package")
set(TRPG_ENGINE_DEV_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../engine/build" CACHE PATH "Path to engine build tree package (DEV flow)")
set(TRPG_ENGINE_FINAL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/../install" CACHE PATH "Path to installed engine package prefix (FINAL flow)")

# Engine package selection:
# - DEV   (default): consume build-tree package exported by engine/build.
# - FINAL: consume installed package from install/.
if(TRPG_USE_INSTALLED_ENGINE)
    list(PREPEND CMAKE_PREFIX_PATH "${TRPG_ENGINE_FINAL_PREFIX}")
else()
    list(PREPEND CMAKE_PREFIX_PATH "${TRPG_ENGINE_DEV_PREFIX}")
endif()

find_package(TRPGEngine CONFIG REQUIRED)

# Delegate the actual game target to the module CMakeLists.
add_subdirectory(game)