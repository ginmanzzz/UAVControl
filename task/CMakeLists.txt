# Task模块 CMakeLists.txt

# 收集task模块的所有源文件
set(TASK_SOURCES
    Task.h
    Plan.h
    TaskManager.h
    TaskManager.cpp
    TaskUI.h
    TaskUI.cpp
    TaskLeftControlWidget.h
    TaskLeftControlWidget.cpp
    CreateTaskDialog.h
    CreateTaskDialog.cpp
    PlanDialog.h
    PlanDialog.cpp
    RegionDetailWidget.h
    RegionDetailWidget.cpp
    RegionFeatureDialog.h
    RegionFeatureDialog.cpp
)

# 收集map_region子模块的源文件
set(MAP_REGION_SOURCES
    map_region/MapRegionTypes.h
    map_region/MapPainter.h
    map_region/MapPainter.cpp
    map_region/InteractiveMapWidget.h
    map_region/InteractiveMapWidget.cpp
    map_region/Region.h
    map_region/Region.cpp
    map_region/RegionManager.h
    map_region/RegionManager.cpp
)

# 将所有源文件路径添加task/前缀，并传递给父目录
foreach(src ${TASK_SOURCES})
    list(APPEND TASK_MODULE_SOURCES "task/${src}")
endforeach()

foreach(src ${MAP_REGION_SOURCES})
    list(APPEND TASK_MODULE_SOURCES "task/${src}")
endforeach()

# 将源文件列表传递给父作用域
set(TASK_MODULE_SOURCES ${TASK_MODULE_SOURCES} PARENT_SCOPE)
