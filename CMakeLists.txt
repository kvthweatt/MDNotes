cmake_minimum_required(VERSION 3.16)
project(markdown_editor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Widgets WebEngineWidgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CMARK REQUIRED libcmark)

add_executable(markdown_editor main.cpp)

target_link_libraries(markdown_editor
    PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::WebEngineWidgets
    ${CMARK_LIBRARIES}
)

target_include_directories(markdown_editor
    PRIVATE
    ${CMARK_INCLUDE_DIRS}
)
