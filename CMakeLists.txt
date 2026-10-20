cmake_minimum_required(VERSION 3.10)
project(WalkingRobotOpenGL CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)

if (WIN32)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include ${OPENGL_INCLUDE_DIRS})
    add_definitions(-DFREEGLUT_STATIC)
    add_executable(WalkingRobot main.cpp glut_compat.cpp)
    target_link_libraries(WalkingRobot ${OPENGL_LIBRARIES} gdi32 user32)
else()
    find_package(GLUT REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
    add_executable(WalkingRobot main.cpp)
    target_link_libraries(WalkingRobot ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES})
endif()
