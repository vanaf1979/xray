cmake_minimum_required(VERSION 3.31)
project(exray)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


find_package(Qt6 COMPONENTS
        Core
        Gui
        Widgets
        REQUIRED)

# Find OpenColorIO
find_package(OpenColorIO CONFIG REQUIRED)

find_package(OpenImageIO REQUIRED)

add_executable(exray main.cpp
        MainWindow.h
        MainWindow.cpp
        Viewport.h
        Viewport.cpp
        Image.h
        Image.cpp
        ColorManager.h
        ColorManager.cpp)

target_link_libraries(exray
        Qt::Core
        Qt::Gui
        Qt::Widgets
        OpenColorIO::OpenColorIO
        OpenImageIO::OpenImageIO
)

