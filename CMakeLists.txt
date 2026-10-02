cmake_minimum_required(VERSION 3.22)

project(som
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Concurrent
)

# Выносим исходный код
set(CORE_SOURCES
    src/core/config/settings.h
    src/core/config/settings.cpp
    # Сюда позже добавим клиенты API, downloader и запуск
)

set(UI_SOURCES
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.ui
)

add_executable(som
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
)

target_include_directories(som PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(som PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
)

# Для Windows убираем консольное окно при релизе
if(WIN32 AND NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set_target_properties(som PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()
