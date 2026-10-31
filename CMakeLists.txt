set(CMAKE_TOOLCHAIN_FILE "C:/bin/vcpkg/scripts/buildsystems/vcpkg.cmake")
set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "")
cmake_minimum_required(VERSION 3.20)
project(OpenGLCourseWork)

find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

add_executable(app main.cpp)

target_link_libraries(app PRIVATE glfw glad::glad glm::glm)