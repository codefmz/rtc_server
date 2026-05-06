file(GLOB SRC "./*.cpp")

add_library(winMedia STATIC ${SRC})
target_link_libraries(winMedia PUBLIC media threadpool utils 
    d3d11 dxgi mf mfplat mfuuid wmcodecdspuuid runtimeobject windowsapp
)
target_include_directories(winMedia PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
