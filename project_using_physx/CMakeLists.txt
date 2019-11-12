cmake_minimum_required(VERSION 3.13)
project(hello_physx)

find_package(PhysX REQUIRED)

add_executable(SnippetHelloWorld
    ./source/SnippetUtils.cpp
    ./source/ClassicMain.cpp
    ./source/SnippetHelloWorld.cpp
    )
target_include_directories(SnippetHelloWorld PRIVATE ./include)
target_link_libraries(SnippetHelloWorld PRIVATE
    PhysX::PhysXExtensions
    PhysX::PhysXCommon)


# Render Example
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_library(SnippetCameraRender
    ./source/SnippetCamera.cpp
    ./source/SnippetRender.cpp
    )
target_include_directories(SnippetCameraRender PUBLIC ./include)
target_link_libraries(SnippetCameraRender PUBLIC
    PhysX::PhysXExtensions
    PhysX::PhysXCommon)
target_link_libraries(SnippetCameraRender PUBLIC
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARY}
    )

add_executable(SnippetHelloWorldRender
    ./source/ClassicMain.cpp
    ./source/SnippetHelloWorld.cpp
    ./source/SnippetHelloWorldRender.cpp
    )
target_compile_definitions(SnippetHelloWorldRender PRIVATE RENDER_SNIPPET)
target_include_directories(SnippetHelloWorldRender PRIVATE ./include)
target_link_libraries(SnippetHelloWorldRender PRIVATE SnippetCameraRender)
