cmake_minimum_required (VERSION 2.8.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

project (PuffinEngine)

add_executable(${PROJECT_NAME}  "puffinEngine/src/Buffer.cpp"
                                "puffinEngine/src/Device.cpp"
                                "puffinEngine/src/PuffinEngine.cpp"
                                "main.cpp")

find_package(Vulkan)
if (Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} "${Vulkan_LIBRARY}")
    include_directories(${PROJECT_NAME} "${Vulkan_INCLUDE_DIR}")
endif (Vulkan_FOUND)

#[[----Downloading and unpack GLFW3 into liblaries directory.----]]

configure_file(CMakeListsLib.txt.in ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
RESULT_VARIABLE result
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw)
if(result)
    message(FATAL_ERROR "CMake step for glfw3 failed: ${result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw)
if(result)
    message(FATAL_ERROR "Build step for glfw3 failed: ${result}")
endif()

set(GLFW3_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GLFW3_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/include)
include_directories(${PROJECT_NAME} ${GLFW3_INCLUDE_DIR})

if(MSVC)
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/lib/libglfw3.lib")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set_target_properties(${WINDOWS_BINARIES} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif ()

if(UNIX)
    target_link_libraries(${PROJECT_NAME} "${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/lib/libglfw3.a")
    target_link_libraries(${PROJECT_NAME} -lGL -ldl -lX11 -lXxf86vm -lXrandr -lpthread -lXi)
endif()

#[[----Include GTest in project.----]]

enable_testing()
add_subdirectory(${CMAKE_SOURCE_DIR}/tests)