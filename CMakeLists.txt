cmake_minimum_required(VERSION 3.10)
project(HesloPlugin)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DIBM=1 -DXPLM200 -DXPLM210 -DXPLM300 -DXPLM301)

if(NOT XPLANE_SDK_PATH)
    set(XPLANE_SDK_PATH "C:/Librerie/XPlaneSDK")
endif()

include_directories(${XPLANE_SDK_PATH}/CHeaders/XPLM)

add_library(win.xpl SHARED HesloPhysics.cpp)
set_target_properties(win.xpl PROPERTIES PREFIX "")
target_link_libraries(win.xpl opengl32)
