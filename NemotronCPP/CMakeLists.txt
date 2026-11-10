cmake_minimum_required(VERSION 3.15)
project(NemotronCPP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows specific
if(WIN32)
    add_definitions(-DWIN32 -D_WIN32 -D_WINDOWS -D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP /bigobj")
endif()

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find Java
find_package(Java REQUIRED COMPONENTS Development)
find_package(JNI REQUIRED)

# Find JSON library (using nlohmann/json via FetchContent)
include(FetchContent)
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(nlohmann_json)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JAVA_INCLUDE_PATH}
    ${JAVA_INCLUDE_PATH2}
    ${nlohmann_json_INCLUDE_DIRS}
)

# Injector executable
add_executable(NemotronInjector
    src/injector.cpp
)

target_link_libraries(NemotronInjector
    ${JAVA_LIBRARIES}
    psapi
    version
)

set_target_properties(NemotronInjector PROPERTIES
    OUTPUT_NAME "NemotronInjector"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# DLL
add_library(NemotronDLL SHARED
    src/dllmain.cpp
    src/jni_helper.cpp
)

target_link_libraries(NemotronDLL
    ${JAVA_LIBRARIES}
    nlohmann_json::nlohmann_json
)

set_target_properties(NemotronDLL PROPERTIES
    OUTPUT_NAME "NemotronDLL"
    PREFIX ""
    SUFFIX ".dll"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Copy DLL to build root for easy access
add_custom_command(TARGET NemotronDLL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:NemotronDLL>
        $<TARGET_FILE_DIR:NemotronInjector>/NemotronDLL.dll
)

# Install targets
install(TARGETS NemotronInjector NemotronDLL
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
)

# CPack for packaging
include(CPack)
set(CPACK_PACKAGE_NAME "NemotronCPP")
set(CPACK_PACKAGE_VERSION "1.0.0")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Nemotron C++ Injector for Minecraft 1.8.9")
set(CPACK_GENERATOR "ZIP")