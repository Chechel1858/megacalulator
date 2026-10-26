cmake_minimum_required(VERSION 3.14)

project(WaterOrderSystem
    VERSION 1.0.0
    DESCRIPTION "Система заказа воды: C++ сервер на cpp-httplib и сайт клиента"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Тип сборки" FORCE)
endif()

if(MSVC)
    # /utf-8 — исходники в UTF-8, иначе русские строки превратятся в кракозябры.
    add_compile_options(/utf-8 /W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra)
endif()

if(WIN32)
    # Windows 10+ (требование cpp-httplib) и без макросов min/max из windows.h.
    add_compile_definitions(_WIN32_WINNT=0x0A00 NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

find_package(Threads REQUIRED)

# Логика заказов без сетевой части — используется сервером и тестами.
add_library(water_core STATIC
    server/catalog.cpp
    server/order.cpp
    server/order_store.cpp)
target_include_directories(water_core PUBLIC server)
target_include_directories(water_core SYSTEM PUBLIC third_party)

add_executable(water_server
    server/main.cpp
    server/api.cpp)
target_link_libraries(water_server PRIVATE water_core Threads::Threads)
if(WIN32)
    target_link_libraries(water_server PRIVATE ws2_32)
endif()

# Копируем сайт рядом с water_server.exe при каждой сборке,
# чтобы сервер запускался двойным щелчком.
add_custom_target(copy_client ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/client $<TARGET_FILE_DIR:water_server>/client
    COMMENT "Копирование сайта клиента")

enable_testing()
add_executable(water_tests tests/tests.cpp)
target_link_libraries(water_tests PRIVATE water_core)
add_test(NAME water_tests COMMAND water_tests)
