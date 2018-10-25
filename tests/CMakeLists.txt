cmake_minimum_required(VERSION 2.8.2)

#[[----Downloading and unpack google test into build directory at configure time.----]]

configure_file(CMakeListsGTest.txt.in ${CMAKE_BINARY_DIR}/googletest-download/CMakeLists.txt)
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

project(PuffinEngineTest)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

if(MSVC)
if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()
elseif(CMAKE_COMPILER_ISGNUCC OR CMAKE_COMPILER_ISGNUCXX)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()


add_executable(${PROJECT_NAME} "BufferTest.cpp" "PuffinEngineTest.cpp" "main.cpp")

target_link_libraries (${PROJECT_NAME} gtest gmock)

if(UNIX)
target_link_libraries (${PROJECT_NAME} pthread)
endif ()

set (gtest_disable_pthreads on)

find_package(Vulkan)
if (Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} "${Vulkan_LIBRARY}")
    include_directories(${PROJECT_NAME} "${Vulkan_INCLUDE_DIR}")
endif (Vulkan_FOUND)


set(GLFW_LIBRARIES ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/include)
include_directories(${PROJECT_NAME} ${GLFW_INCLUDE_DIR})

if(MSVC)
    target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})
endif ()

if(UNIX)
    target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} -lGL -ldl -lX11 -lXxf86vm -lXrandr -lpthread -lXi)
endif()

set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glm)
include_directories(${PROJECT_NAME} ${GLM_INCLUDE_DIR})

set(GLI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/gli)
include_directories(${PROJECT_NAME} ${GLI_INCLUDE_DIR})
