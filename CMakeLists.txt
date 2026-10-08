cmake_minimum_required(VERSION 3.16)
project(MultithreadedFileSearchEngine
    VERSION 1.0.0
    DESCRIPTION "Production-quality multithreaded file search engine in C++17"
    LANGUAGES CXX
)

# ============================================================
#  C++17 standard
# ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================
#  Compiler warnings
# ============================================================
if(MSVC)
    add_compile_options(/W4 /WX /permissive-)
    # Disable some overly strict MSVC warnings
    add_compile_options(/wd4100 /wd4458)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic -Wshadow
        -Wno-unused-parameter
    )
endif()

# ============================================================
#  Export compile commands (for IDE integration)
# ============================================================
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
#  Output directories
# ============================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ============================================================
#  Source files
# ============================================================
set(ENGINE_SOURCES
    src/Logger.cpp
    src/FileMetadata.cpp
    src/ThreadPool.cpp
    src/Tokenizer.cpp
    src/FileScanner.cpp
    src/InvertedIndex.cpp
    src/SearchEngine.cpp
    src/IndexManager.cpp
    src/BenchmarkManager.cpp
)

set(ENGINE_HEADERS
    include/Logger.h
    include/FileMetadata.h
    include/ThreadSafeQueue.h
    include/ThreadPool.h
    include/Tokenizer.h
    include/FileScanner.h
    include/InvertedIndex.h
    include/SearchEngine.h
    include/SearchResult.h
    include/IndexManager.h
    include/BenchmarkManager.h
)

# ============================================================
#  Core engine static library
#  (shared between the main executable and unit tests)
# ============================================================
add_library(search_engine_lib STATIC
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
)

target_include_directories(search_engine_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Link threading library
find_package(Threads REQUIRED)
target_link_libraries(search_engine_lib PUBLIC Threads::Threads)

# ============================================================
#  Main executable
# ============================================================
add_executable(search_engine src/main.cpp)
target_link_libraries(search_engine PRIVATE search_engine_lib)

# ============================================================
#  GoogleTest
# ============================================================
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# Prevent GoogleTest from overriding our compiler/linker settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

enable_testing()

# ============================================================
#  Unit test executable
# ============================================================
set(TEST_SOURCES
    tests/test_tokenizer.cpp
    tests/test_queue.cpp
    tests/test_index.cpp
    tests/test_search.cpp
    tests/test_scanner.cpp
    tests/test_persistence.cpp
)

add_executable(run_tests ${TEST_SOURCES})

target_include_directories(run_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(run_tests PRIVATE
    search_engine_lib
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(run_tests)

# ============================================================
#  Create benchmarks output directory
# ============================================================
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/benchmarks)

# ============================================================
#  Install rules (optional, for packaging)
# ============================================================
install(TARGETS search_engine
    RUNTIME DESTINATION bin
)

# ============================================================
#  Summary
# ============================================================
message(STATUS "==============================================")
message(STATUS "  Multithreaded File Search Engine")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "==============================================")
message(STATUS "  Targets:")
message(STATUS "    search_engine_lib  (static library)")
message(STATUS "    search_engine      (main executable)")
message(STATUS "    run_tests          (unit tests)")
message(STATUS "==============================================")
