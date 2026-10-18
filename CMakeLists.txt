cmake_minimum_required(VERSION 3.10)
project(HesloPlugin)

set(CMAKE_CXX_STANDARD 17)
# La correzione è nella riga qui sotto (-DIBM=1)
add_definitions(-DIBM=1 -DXPLM200 -DXPLM210 -DXPLM300 -DXPLM301)

include_directories(${XPLANE_SDK_PATH}/CHeaders/XPLM)

add_library(win.xpl SHARED HesloPhysics.cpp)
set_target_properties(win.xpl PROPERTIES PREFIX "")
target_link_libraries(win.xpl opengl32)
