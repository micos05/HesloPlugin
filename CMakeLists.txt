cmake_minimum_required(VERSION 3.10)
project(HesloPlugin)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DIBM=1 -D_CRT_SECURE_NO_WARNINGS -DXPLM200 -DXPLM210 -DXPLM300 -DXPLM301)

include_directories(${XPLANE_SDK_PATH}/CHeaders/XPLM)

add_library(win.xpl SHARED HesloPhysics.cpp)
set_target_properties(win.xpl PROPERTIES PREFIX "" SUFFIX "")

target_link_libraries(win.xpl "${XPLANE_SDK_PATH}/Libraries/Win/XPLM_64.lib" opengl32)
