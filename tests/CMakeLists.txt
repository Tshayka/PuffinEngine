cmake_minimum_required(VERSION 2.8.2)

project(PuffinEngineTest)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

add_executable(${PROJECT_NAME} "BufferTest.cpp" "PuffinEngineTest.cpp" "main.cpp")

target_link_libraries (${PROJECT_NAME} gtest gmock pthread)
set (gtest_disable_pthreads on)

find_package(Vulkan)
if (Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} "${Vulkan_LIBRARY}")
    include_directories(${PROJECT_NAME} "${Vulkan_INCLUDE_DIR}")
endif (Vulkan_FOUND)

