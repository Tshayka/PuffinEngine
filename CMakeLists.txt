cmake_minimum_required (VERSION 2.8.2)

#[[----Downloading and unpack google test into build directory at configure time.----]]

configure_file(CMakeLists.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
execute_process(COMMAND "${CMAKE_COMMAND}" -G "${CMAKE_GENERATOR}" .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/googletest-download )
if(result)
    message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

add_subdirectory(
    ${CMAKE_BINARY_DIR}/googletest-src
    ${CMAKE_BINARY_DIR}/googletest-build)


#[[----Configuration of the project.----]]

project (PuffinEngine)

add_executable(${PROJECT_NAME}  "puffinEngine/src/Buffer.cpp"
                                "puffinEngine/src/Device.cpp"
                                "puffinEngine/src/PuffinEngine.cpp"
                                "main.cpp")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

enable_testing()
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)

find_package(Vulkan)
if (Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} "${Vulkan_LIBRARY}")
    include_directories(${PROJECT_NAME} "${Vulkan_INCLUDE_DIR}")
endif (Vulkan_FOUND)

include_directories(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/include")
target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/lib/libglfw3.a")

set(GLFW_LIB ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GLFW_INCLUDE ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/include)
include_directories(${PROJECT_NAME} ${GLFW_INCLUDE})
target_link_libraries(${PROJECT_NAME} ${GLFW_LIB} -lGL -ldl -lX11 -lXxf86vm -lXrandr -lpthread -lXi)

