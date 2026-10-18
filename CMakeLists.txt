cmake_minimum_required(VERSION 3.10)
project(HesloPlugin)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DXPLM200 -DXPLM210 -DXPLM300 -DXPLM301)

# Aggiungi qui il percorso dell'SDK di X-Plane
set(XPLANE_SDK_PATH "C:/Librerie/XPlaneSDK") 

include_directories(
    ${XPLANE_SDK_PATH}/CHeaders/XPLM
)

add_library(win.xpl SHARED HesloPhysics.cpp)

# Rimuove il prefisso "lib" imposto da CMake, richiesto da X-Plane
set_target_properties(win.xpl PROPERTIES PREFIX "")

target_link_libraries(win.xpl opengl32)