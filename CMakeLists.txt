cmake_minimum_required( VERSION 3.1 )

project( Tracey )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only" FORCE)
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW lib only" FORCE)
set( GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only" FORCE)
set( GLFW_INSTALL OFF CACHE BOOL "GLFW lib only" FORCE)

set(ASSIMP_BUILD_TESTS OFF)
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
set(ASSIMP_BUILD_OBJ_IMPORTER TRUE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
# Needed for compilation?
set(ASSIMP_BUILD_ASSBIN_EXPORTER TRUE)
set(ASSIMP_BUILD_ASSXML_EXPORTER TRUE)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
add_subdirectory( external/glfw )
add_subdirectory( external/assimp )

include_directories(
    external/glfw/include
    external/glm
    external/glad/include
    external/imgui
    external/imgui/backends
    external/tinyobjloader
    src/
    )

if( MSVC )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

set( SRC
	external/imgui/imgui.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/imgui_widgets.cpp
	external/imgui/backends/imgui_impl_glfw.cpp
	external/imgui/backends/imgui_impl_opengl3.cpp
	external/glad/src/glad.c
	src/hittables/triangle.cpp
	src/hittables/triangle_mesh.cpp
	src/textures/image_texture.cpp
	src/tracey.cpp
	src/core.cpp
	src/renderer.cpp
	src/camera.cpp
	src/scene.cpp
	src/scene_parser.cpp
	src/options_manager.cpp
	src/input_manager.cpp
	src/thread_pool.cpp
	src/quartic.cpp
	src/bvh.cpp
	src/transform.cpp
	src/animation.cpp
	)

if (UNIX)
	set (
		LIBS
		dl
		pthread
		glfw
		assimp
		)
else()
	set (
		LIBS
		glfw
		assimp
		)
endif()

set(CXX_OPTIONS -ffast-math)
add_executable( Tracey ${SRC} src/tracey.cpp)
target_link_libraries( Tracey ${LIBS})
target_compile_options( Tracey PRIVATE ${CXX_OPTIONS})
set_property(TARGET Tracey PROPERTY CXX_STANDARD 17)

if( UNIX )
    add_custom_command(
        TARGET Tracey
        POST_BUILD COMMAND ${CMAKE_COMMAND} -E
        create_symlink ${CMAKE_CURRENT_SOURCE_DIR}/src/kernels ${CMAKE_BINARY_DIR}/kernels)
else()
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_BINARY_DIR}/kernels" _dstDir)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/kernels" _srcDir)
    execute_process(COMMAND cmd.exe /c mklink /J "${_dstDir}" "${_srcDir}")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.txt ${CMAKE_CURRENT_BINARY_DIR}/config.txt COPYONLY)

if( MSVC )
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
        message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'GLFW-CMake-starter' as StartUp Project in Visual Studio.\n" )
    else()
        set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GLFW-CMake-starter )
    endif()
endif()

