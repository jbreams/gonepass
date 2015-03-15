cmake_minimum_required (VERSION 3.0)

# CMake include path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

project(gonepass)

SET( PKGDATADIR ${CMAKE_INSTALL_PREFIX}/${DATA_INSTALL_DIR} )

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(JANSSON REQUIRED jansson)
pkg_check_modules(OPENSSL REQUIRED openssl)



# GResource
include(GResource)
gresource(${CMAKE_CURRENT_SOURCE_DIR} gonepassapp.gresource.xml
    ${CMAKE_BINARY_DIR} RESOURCE_FILE)

include_directories (${GTK3_INCLUDE_DIRS} ${JANSSON_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS})
link_directories (${GTK3_LIBRARY_DIRS} ${JANSSON_LIBRARY_DIRS} ${OPENSSL_LIBRARY_DIRS})

set (SOURCES
    appwindow.c
    gonepassapp.c
    item_builder.c
    main.c
    unlockdialog.c
    ${RESOURCE_FILE}
)

set (HEADERS
    gonepass.h
)

add_executable(gonepass
    ${SOURCES}
)

target_link_libraries(gonepass
    ${GTK3_LIBRARIES}
    ${JANSSON_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

# GSettings
include(GSettings)
add_schemas(gonepass ${CMAKE_CURRENT_SOURCE_DIR})

install (TARGETS gonepass DESTINATION bin)
