# Launch模块 CMakeLists.txt

# 收集launch模块的所有源文件
set(LAUNCH_SOURCES
    LaunchUI.h
    LaunchUI.cpp
)

# 将所有源文件路径添加launch/前缀，并传递给父目录
foreach(src ${LAUNCH_SOURCES})
    list(APPEND LAUNCH_MODULE_SOURCES "launch/${src}")
endforeach()

# 将源文件列表传递给父作用域
set(LAUNCH_MODULE_SOURCES ${LAUNCH_MODULE_SOURCES} PARENT_SCOPE)
