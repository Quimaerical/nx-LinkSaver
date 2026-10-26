cmake_minimum_required(VERSION 3.16)
project(sw-LinkSaver VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source files
file(GLOB_RECURSE SOURCES
    src/*.cpp
)

include_directories(
    include
)

# Nintendo Switch Target Configuration
if(DEFINED NINTENDO_SWITCH OR DEFINED SWITCH OR EXISTS "/opt/devkitpro/libnx")
    message(STATUS "Configuring sw-LinkSaver for Nintendo Switch (.nro)")

    # devkitPro environment settings
    if(NOT DEFINED DEVKITPRO)
        set(DEVKITPRO "/opt/devkitpro")
    endif()

    include("${DEVKITPRO}/cmake/Switch.cmake" OPTIONAL)

    # Borealis integration via FetchContent if not provided externally
    include(FetchContent)
    FetchContent_Declare(
        borealis
        GIT_REPOSITORY https://github.com/natinusala/borealis.git
        GIT_TAG master
    )
    FetchContent_MakeAvailable(borealis)

    # Target executable
    add_executable(sw-LinkSaver ${SOURCES})

    target_include_directories(sw-LinkSaver PRIVATE
        include
        ${PORTLIBS}/include
    )

    target_link_libraries(sw-LinkSaver PRIVATE
        borealis
        curl
        mbedtls
        mbedx509
        mbedcrypto
        z
        nx
    )

    # Homebrew NACP metadata and NRO generation
    if(COMMAND nx_generate_nacp AND COMMAND nx_create_nro)
        set(APP_TITLE "LinkSaver")
        set(APP_AUTHOR "Sebastian")
        set(APP_VERSION "1.0.0")

        nx_generate_nacp(sw-LinkSaver.nacp
            NAME ${APP_TITLE}
            AUTHOR ${APP_AUTHOR}
            VERSION ${APP_VERSION}
        )

        nx_create_nro(sw-LinkSaver
            NACP sw-LinkSaver.nacp
            ICON "${CMAKE_CURRENT_SOURCE_DIR}/romfs/icon.jpg"
            ROMFS "${CMAKE_CURRENT_SOURCE_DIR}/romfs"
        )
    endif()

else()
    # Desktop build target (Linux / PC)
    message(STATUS "Configuring sw-LinkSaver for Desktop development")

    # Find system curl
    find_package(CURL REQUIRED)

    # Target library or executable for components
    add_executable(sw-LinkSaver-desktop ${SOURCES})

    target_include_directories(sw-LinkSaver-desktop PRIVATE
        include
    )

    target_link_libraries(sw-LinkSaver-desktop PRIVATE
        CURL::libcurl
        pthread
    )
endif()
