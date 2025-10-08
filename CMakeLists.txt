cmake_minimum_required(VERSION 3.19)
project(DrawingDemo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置 Qt6 和 MapLibre 路径
set(CMAKE_PREFIX_PATH
    "$ENV{HOME}/Qt6/6.7.3/gcc_64"
    "$ENV{HOME}/projects/maplibre-native-qt/install"
)

# 查找 Qt
find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS
    Core
    Gui
    Widgets
    REQUIRED
)

# 查找 QMapLibre（需要先安装或设置 CMAKE_PREFIX_PATH）
find_package(QMapLibre COMPONENTS Widgets REQUIRED)

# 添加 MapPainter 库
add_library(MapPainter STATIC
    MapPainter.h
    MapPainter.cpp
)

target_link_libraries(MapPainter
    PUBLIC
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        QMapLibre::Widgets
)

# 添加主程序可执行文件
add_executable(drawing-demo
    main.cpp
    InteractiveMapWidget.h
    InteractiveMapWidget.cpp
)

# 链接库
target_link_libraries(drawing-demo
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
        Qt${QT_VERSION_MAJOR}::Widgets
        QMapLibre::Widgets
        MapPainter
)

# 设置 RPATH，使可执行文件能自动找到共享库
# 禁用 RUNPATH，强制使用 RPATH（优先级更高）
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

set_target_properties(drawing-demo PROPERTIES
    INSTALL_RPATH "$ENV{HOME}/Qt6/6.7.3/gcc_64/lib:$ENV{HOME}/projects/maplibre-native-qt/install/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
    LINK_FLAGS "-Wl,--disable-new-dtags"
)
