# SPDX-License-Identifier: GPL-3.0-only
cmake_minimum_required(VERSION 3.22)
project(wetypex_desktop_demo LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Test)
add_executable(wetypex-desktop-demo main.cpp window.cpp window.hpp model.hpp settings_pages.cpp settings_pages.hpp transfer_page.cpp transfer_page.hpp)
target_link_libraries(wetypex-desktop-demo PRIVATE Qt6::Widgets)
enable_testing()
add_executable(demo-tests tests.cpp window.cpp window.hpp model.hpp settings_pages.cpp settings_pages.hpp transfer_page.cpp transfer_page.hpp)
target_link_libraries(demo-tests PRIVATE Qt6::Widgets Qt6::Test)
add_test(NAME desktop-demo COMMAND demo-tests)
set_tests_properties(desktop-demo PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
# Deliberately no install rule and no runtime/backend dependencies.
