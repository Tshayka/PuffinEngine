cmake_minimum_required (VERSION 2.8.2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_ROOT_DIR}/bin)


if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wpedantic -Wextra")
    set(CMAKE_CONFIGURATION_TYPES Debug)
    message("-- Debug build")
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wpedantic -Wextra -O3")
    set(CMAKE_CONFIGURATION_TYPES Release)
    message("-- Release build")
endif()

project (PuffinEngine)

add_executable(${PROJECT_NAME}  "puffinEngine/src/Actor.cpp"
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
                                "puffinEngine/src/MainCharacter.cpp"
                                "puffinEngine/src/MaterialLibrary.cpp"
                                "puffinEngine/src/MeshLayout.cpp"
                                "puffinEngine/src/MeshLibrary.cpp"
                                "puffinEngine/src/LoadTexture.cpp"
                                "puffinEngine/src/MousePicker.cpp"
                                "puffinEngine/src/PuffinEngine.cpp"
                                "puffinEngine/src/Scene.cpp"
                                "puffinEngine/src/Texture.cpp"
                                "puffinEngine/src/Threads.cpp"
                                "puffinEngine/src/Ui.cpp"
                                "puffinEngine/src/WorldClock.cpp"
                                "main.cpp")

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

set(GLM_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/glm)
include_directories(${PROJECT_NAME} ${GLM_INCLUDE_DIR})

set(GLI_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/gli/include)
include_directories(${GLI_INCLUDE_DIR})

#[[----Downloading and unpack TINYOBJLOADER into libraries directory.----]]

include(ExternalProject)
ExternalProject_Add(tinyobjloader
	GIT_REPOSITORY         https://github.com/syoyo/tinyobjloader
    GIT_TAG                master
    SOURCE_DIR              "${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader"
	CMAKE_ARGS              "-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader"
	UPDATE_COMMAND  ""
)

set(TINYOBJ_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/puffinEngine/lib/tinyobjloader/include)
include_directories(${PROJECT_NAME} ${TINYOBJ_INCLUDE_PATH})

#[[----Downloading and unpack STB into libraries directory.----]]

include(ExternalProject)
ExternalProject_Add(stb
	GIT_REPOSITORY         https://github.com/nothings/stb
    GIT_TAG                master
    SOURCE_DIR              "${CMAKE_SOURCE_DIR}/puffinEngine/lib/stb"
	CMAKE_ARGS              "-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/puffinEngine/lib/stb"
	UPDATE_COMMAND  ""
    CONFIGURE_COMMAND  ""
    BUILD_COMMAND  ""
    INSTALL_COMMAND  ""
)

set(STB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/stb)
target_include_directories(${PROJECT_NAME} PUBLIC ${STB_INCLUDE_DIR})

add_definitions(-DSTB_IMAGE_IMPLEMENTATION)

#[[---Downloading and unpack BOOST into libraries directory.---]]

include(ExternalProject)
ExternalProject_Add(BOOST
	URL                     "https://dl.bintray.com/boostorg/release/1.69.0/source/boost_1_69_0.tar.bz2"
    URL_MD5                 "a1332494397bf48332cb152abfefcec2"
    SOURCE_DIR              "${CMAKE_SOURCE_DIR}/puffinEngine/lib/boost"
	CMAKE_ARGS              "-DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/puffinEngine/lib/boost"
    CONFIGURE_COMMAND       ./bootstrap.sh --with-libraries=atomic,date_time,filesystem,program_options,system,thread
    BUILD_COMMAND           ./b2 link=static
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
	UPDATE_COMMAND  ""
)

set(BOOST_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/puffinEngine/lib/boost)
target_include_directories(${PROJECT_NAME} PUBLIC ${BOOST_INCLUDE_DIR})

#[[----Include GTest in project.----]]

#enable_testing()
#add_subdirectory(${CMAKE_SOURCE_DIR}/tests)