cmake_minimum_required (VERSION 2.8.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

project (PuffinEngine)

add_executable(${PROJECT_NAME}  "puffinEngine/src/Buffer.cpp"
                                "puffinEngine/src/Device.cpp"
                                "puffinEngine/src/ErrorCheck.cpp"
                                "puffinEngine/src/LoadMesh.cpp"
                                "puffinEngine/src/LoadTexture.cpp"
                                "puffinEngine/src/PuffinEngine.cpp"
                                "puffinEngine/src/StatusOverlay.cpp"
                                "puffinEngine/src/Ui.cpp"
                                "main.cpp")

find_package(Vulkan)
if (Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} "${Vulkan_LIBRARY}")
    include_directories(${PROJECT_NAME} "${Vulkan_INCLUDE_DIR}")
endif (Vulkan_FOUND)

#[[----Downloading and unpack GLFW3, GLI, GLM into liblaries directory.----]]

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

set(GLFW_LIBRARIES ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/lib/${CMAKE_STATIC_LIBRARY_PREFIX}glfw3${CMAKE_STATIC_LIBRARY_SUFFIX})
set(GLFW_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glfw/include)
include_directories(${PROJECT_NAME} ${GLFW_INCLUDE_DIR})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})
endif ()

if(UNIX)
    target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES})
    target_link_libraries(${PROJECT_NAME} -lGL -ldl -lX11 -lXxf86vm -lXrandr -lpthread -lXi)
endif()

set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glm)
include_directories(${PROJECT_NAME} ${GLM_INCLUDE_DIR})

set(GLI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/gli/include)
include_directories(${GLI_INCLUDE_DIR})

#[[----Downloading and unpack IMGUI into libraries directory.----]]

include(ExternalProject)
ExternalProject_Add(imgui
	GIT_REPOSITORY         https://github.com/ocornut/imgui
    GIT_TAG                         master
    SOURCE_DIR              "${CMAKE_SOURCE_DIR}/puffinEngine/lib/imgui"
	CMAKE_ARGS              "-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/puffinEngine/lib/imgui"
	UPDATE_COMMAND  ""
    CONFIGURE_COMMAND  ""
    BUILD_COMMAND  ""
    INSTALL_COMMAND  ""
)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/imgui)
include_directories(${PROJECT_NAME} ${IMGUI_DIR})

#[[----Downloading and unpack TINYOBJLOADER into libraries directory.----]]

include(ExternalProject)
ExternalProject_Add(tinyobjloader
	GIT_REPOSITORY         https://github.com/syoyo/tinyobjloader
    GIT_TAG                         master
    SOURCE_DIR              "${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader"
	CMAKE_ARGS              "-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader"
	UPDATE_COMMAND  ""
    CONFIGURE_COMMAND  ""
    BUILD_COMMAND  ""
    INSTALL_COMMAND  ""
)

set(TINYOBJLOADER_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/imgui/include)
include_directories(${PROJECT_NAME} ${TINYOBJLOADER_DIR})

#[[----Include GTest in project.----]]

#enable_testing()
#add_subdirectory(${CMAKE_SOURCE_DIR}/tests)