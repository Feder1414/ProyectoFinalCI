cmake_minimum_required (VERSION 3.10)

project(Tutorial03_Texturing CXX)

set(SOURCE
    src/Tutorial03_Texturing.cpp
    src/Cubo.cpp
)

set(INCLUDE
    src/Tutorial03_Texturing.hpp
    src/POMMaterial.h
    src/FigureBase.h
    src/Cubo.h
    src/ShadowMap.h
    src/DungeonGenerator.h
    src/DungeonScene.h
    
)

set(SHADERS
    assets/cube.vsh
    assets/cube.psh
    assets/ShadowMapVS.vsh
)

#set(ASSETS
#   assets/DGLogo.png
#  assets/brick_wall.png
#    assets/brick_wall_dis.png
#    assets/bricks2_dis.jpg
#    assets/bricks2.jpg
#    assets/bricks2_nor.jpg
#    assets/rock.jpg
#    assets/rock_dis.jpg
#    assets/rock_nor.jpg
#    assets/gray_rocks.png
#    assets/gray_rocks_dis.png
#    assets/gray_rocks_nor.png
#    assets/rocksLow.jpg
#    assets/rocksLow_dis.png
#    assets/rocksLow_nor.png
#    assets/SOI_LeatherPadded/LeatherPadded_01_BC.png
#    assets/SOI_LeatherPadded/LeatherPadded_01_H.png
#    assets/SOI_LeatherPadded/LeatherPadded_01_N.png
#)

#file(GLOB_RECURSE ASSETS
#    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.*"
#    CONFIGURE_DEPENDS
#)
file(GLOB_RECURSE ASSETS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.*"
)

add_sample_app("Tutorial03_Texturing" "DiligentSamples/Tutorials" "${SOURCE}" "${INCLUDE}" "${SHADERS}" "${ASSETS}")

target_link_libraries(Tutorial03_Texturing 
PRIVATE 
    Diligent-AssetLoader
    Diligent-RenderStateNotation
    DiligentFX
)


target_include_directories(Tutorial03_Texturing
PRIVATE
    ../../../DiligentFX/Shaders/Common/public/
)

