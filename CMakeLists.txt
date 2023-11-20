cmake_minimum_required (VERSION 2.8.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_ROOT_DIR}/bin)

if(CMAKE_BUILD_TYPE MATCHES Debug)
if(UNIX)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wpedantic -Wextra")
endif()
    set(CMAKE_CONFIGURATION_TYPES Debug)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/PuffinEngine/work/debug)
    add_definitions(-DDEBUG_VERSION)
    message("-- Debug build")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
if(UNIX)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wpedantic -Wextra -O3 -pthread")
endif()
    set(CMAKE_CONFIGURATION_TYPES Release)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/PuffinEngine/work/release)
    add_definitions(-DRELEASE_VERSION)
    message("-- Release build")
endif()

project (PuffinEngine)
set(VS_STARTUP_PROJECT ${PROJECT_NAME})

SET(SOURCE_FILES                "puffinEngine/src/Actor.cpp"
                                "puffinEngine/src/Buffer.cpp"
                                "puffinEngine/src/Camera.cpp"
                                "puffinEngine/src/Character.cpp"
                                "puffinEngine/src/Device.cpp"
                                "puffinEngine/src/ErrorCheck.cpp"
                                "puffinEngine/src/GuiMainUi.cpp"
                                "puffinEngine/src/GuiMainHub.cpp"
                                "puffinEngine/src/GuiTextOverlay.cpp"
                                "puffinEngine/src/Landscape.cpp"
                                "puffinEngine/src/Light.cpp"
                                "puffinEngine/src/LoadFile.cpp"
                                "puffinEngine/src/Log.cpp"
                                "puffinEngine/src/MainCharacter.cpp"
                                "puffinEngine/src/MaterialLibrary.cpp"
                                "puffinEngine/src/MeshLayout.cpp"
                                "puffinEngine/src/MeshLibrary.cpp"
                                "puffinEngine/src/LoadTexture.cpp"
                                "puffinEngine/src/MousePicker.cpp"
                                "puffinEngine/src/PuffinEngine.cpp"
                                "puffinEngine/src/RenderPass.cpp"
                                "puffinEngine/src/Scene.cpp"
                                "puffinEngine/src/SwapChain.cpp"
                                "puffinEngine/src/Texture.cpp"
                                "puffinEngine/src/Threads.cpp"
                                "puffinEngine/src/Ui.cpp"
                                "puffinEngine/src/WorldClock.cpp"
                                "main.cpp")

SET(HEADER_FILES                "puffinEngine/headers/Actor.hpp"
                                "puffinEngine/headers/Buffer.hpp"
                                "puffinEngine/headers/Camera.hpp"
                                "puffinEngine/headers/Character.hpp"
                                "puffinEngine/headers/Device.hpp"
                                "puffinEngine/headers/ErrorCheck.hpp"
                                "puffinEngine/headers/GuiMainUi.hpp"
                                "puffinEngine/headers/GuiMainHub.hpp"
                                "puffinEngine/headers/GuiTextOverlay.hpp"
                                "puffinEngine/headers/Landscape.hpp"
                                "puffinEngine/headers/Light.hpp"
                                "puffinEngine/headers/Log.hpp"
                                "puffinEngine/headers/MainCharacter.hpp"
                                "puffinEngine/headers/MaterialLibrary.hpp"
                                "puffinEngine/headers/MeshLibrary.hpp"
                                "puffinEngine/headers/MousePicker.hpp"
                                "puffinEngine/headers/PuffinEngine.hpp"
                                "puffinEngine/headers/PushConstant.hpp"
                                "puffinEngine/headers/RenderPass.hpp"
                                "puffinEngine/headers/Scene.hpp"
                                "puffinEngine/headers/SwapChain.hpp"
                                "puffinEngine/headers/Texture.hpp"
                                "puffinEngine/headers/Ui.hpp"
                                "puffinEngine/headers/WorldClock.hpp")


add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

add_subdirectory(puffinEngine/imgui)
target_link_libraries(${PROJECT_NAME} imgui)

find_package(Vulkan)
if (Vulkan_FOUND)
    target_link_libraries(${PROJECT_NAME} "${Vulkan_LIBRARY}")
    include_directories(${PROJECT_NAME} "${Vulkan_INCLUDE_DIR}")
else ()
    message("-- ??")
endif ()


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

#[[----Linking GLM libraries----]]

set(glm_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glm/lib/cmake/glm)
find_package(glm)
target_link_libraries(${PROJECT_NAME} glm::glm)
include_directories(${glm_DIR})

if(NOT GLM_INCLUDE_DIR OR GLM_INCLUDE_DIR MATCHES "NOTFOUND")
	set(GLM_FOUND false)
	if(NOT GLM_QUIET)
		message( STATUS "GLM was not found")
	elseif(GLM_REQUIRED)
		message( SEND_ERROR "GLM was not found")
	endif()
else()
	set(GLM_FOUND true)
	set(GLM_INCLUDE_DIRS ${GLM_INCLUDE_DIR})
        message( STATUS "GLM was found")
endif()

#[[----Linking GLI libraries----]]

set(GLI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/gli/include)
include_directories(${GLI_INCLUDE_DIR})

#[[----Downloading and unpack TINYOBJLOADER into libraries directory.----]]

include(ExternalProject)
ExternalProject_Add(tinyobjloader
    GIT_REPOSITORY         https://github.com/syoyo/tinyobjloader
    GIT_TAG                release
    SOURCE_DIR              "${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader"
    CMAKE_ARGS              "-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader"
    UPDATE_COMMAND  ""
)

set(TINYOBJ_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader/include)
include_directories(${PROJECT_NAME} ${TINYOBJ_INCLUDE_PATH})




#[[----Linking STB libraries----]]

set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/3rdParty/stb/stb-prefix/src/stb)
include_directories(${PROJECT_NAME} ${STB_INCLUDE_DIR})



#[[---Downloading and unpack BOOST into libraries directory.---]]

if(WIN32)
	set(CONFIG_COMMAND_VAL b2.exe)
	set(BUILD_COMMAND_VAL bootstrap.bat)
elseif(UNIX)
	set(CONFIG_COMMAND_VAL ./bootstrap.sh)
	set(BUILD_COMMAND_VAL ./b2)
endif()

configure_file(${CMAKE_SOURCE_DIR}/puffinEngine/cmake/CMakeListsBOOST.txt.in ${CMAKE_SOURCE_DIR}/puffinEngine/3rdParty/boost/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
RESULT_VARIABLE result
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/puffinEngine/3rdParty/boost)
if(result)
    message(FATAL_ERROR "CMake step for BOOST failed: ${result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build .
    RESULT_VARIABLE result
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/puffinEngine/3rdParty/boost)
if(result)
    message(FATAL_ERROR "Build step for BOOST failed: ${result}")
endif()

find_package(Boost)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME}  ${Boost_LIBRARIES})
    message("boost lib: ${Boost_LIBRARIES}")
    message("boost inc: ${Boost_INCLUDE_DIR}")
elseif(NOT Boost_FOUND)
    message("-- Can't find boost libraries. Try to add manually.")
    set(Boost_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/3rdParty/boost/boost-prefix/src/boost/)
    set(Boost_LIBRARY_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/3rdParty/boost/boost-prefix/src/boost/libs)
    include_directories(${Boost_INCLUDE_DIR})
endif()

#[[----Include GTest in project.----]]

#enable_testing()
#add_subdirectory(${CMAKE_SOURCE_DIR}/tests)