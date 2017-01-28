cmake_minimum_required (VERSION 3.1)

# CMake include path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

project(gonepass)

SET( PKGDATADIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR} )

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)
pkg_check_modules(OPENSSL REQUIRED openssl)

set (CMAKE_CXX_STANDARD 11)

# GResource
include(GResource)
gresource(${CMAKE_CURRENT_SOURCE_DIR} gonepass.gresource.xml
    ${CMAKE_BINARY_DIR} RESOURCE_FILE)

include_directories (${GTKMM_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})
link_directories (${GTKMM_LIBRARY_DIRS} ${OPENSSL_LIBRARY_DIRS})

set (SOURCES
    main.cpp
    keychain.cpp
    totp.cpp
    ${RESOURCE_FILE}
)

set (HEADERS
    gonepass.h
)

add_executable(gonepass
    ${SOURCES}
)

target_link_libraries(gonepass
    ${GTKMM_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

install (TARGETS gonepass DESTINATION bin)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/gonepass.desktop.in
    ${CMAKE_BINARY_DIR}/gonepass.desktop
)

install (
    FILES
        ${CMAKE_BINARY_DIR}/gonepass.desktop
    DESTINATION
        share/applications
)

install (
    FILES
        gonepass-icon.png
    DESTINATION
        share/pixmaps
    RENAME
        gonepass.png
)
