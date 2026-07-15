cmake_minimum_required(VERSION 3.21)

project(GridGuard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets)
qt_standard_project_setup()
set(CMAKE_AUTOMOC OFF)

qt_add_executable(GridGuard
    resources.qrc
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/game/enums.h
    src/game/levelconfig.h
    src/game/levelconfig.cpp
    src/game/spritemanager.h
    src/game/spritemanager.cpp
    src/game/entities.h
    src/game/entities.cpp
    src/game/gamecontroller.h
    src/game/gamecontroller.cpp
)

target_link_libraries(GridGuard PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    set_target_properties(GridGuard PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

add_custom_command(TARGET GridGuard POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:GridGuard>/config"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/config/levels.json"
            "$<TARGET_FILE_DIR:GridGuard>/config/levels.json"
    COMMENT "Copying editable level configuration"
)
