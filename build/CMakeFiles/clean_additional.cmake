# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/MapPainter_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/MapPainter_autogen.dir/ParseCache.txt"
  "CMakeFiles/drawing-demo_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/drawing-demo_autogen.dir/ParseCache.txt"
  "MapPainter_autogen"
  "drawing-demo_autogen"
  )
endif()
