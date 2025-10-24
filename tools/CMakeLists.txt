# 瓦片下载工具
cmake_minimum_required(VERSION 3.16)

project(TileDownloader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

set(CMAKE_AUTOMOC ON)

add_executable(tile_downloader
    tile_downloader_cli.cpp
    TileDownloader.cpp
    TileDownloader.h
)

target_link_libraries(tile_downloader
    Qt6::Core
    Qt6::Network
)

# 安装
install(TARGETS tile_downloader DESTINATION bin)
