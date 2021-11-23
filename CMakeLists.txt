cmake_minimum_required( VERSION 3.1 )

project( Tracey )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only" FORCE)
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW lib only" FORCE)
set( GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW lib only" FORCE)
set( GLFW_INSTALL OFF CACHE BOOL "GLFW lib only" FORCE)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
add_subdirectory( external/glfw )

include_directories(
    external/glfw/include
    external/glm
    external/glad/include
    src/
    )

if( MSVC )
    SET( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ENTRY:mainCRTStartup" )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
endif()

set( SRC
     external/glad/src/glad.c
	 src/tracey.cpp
	 src/renderer.cpp
	 src/camera.cpp
	 src/scene.cpp
	 src/options_manager.cpp
	 src/input_manager.cpp
)

if (UNIX)
	set (
		LIBS
		dl
		pthread
		glfw
		)
else()
	set (
		LIBS
		glfw
		)
endif()

set(CXX_OPTIONS -g )
add_executable( Tracey WIN32 ${SRC} src/tracey.cpp)
target_link_libraries( Tracey ${LIBS})
target_compile_options( Tracey PRIVATE ${CXX_OPTIONS})
set_property(TARGET Tracey PROPERTY CXX_STANDARD 17)

if( MSVC )
    if(${CMAKE_VERSION} VERSION_LESS "3.6.0")
        message( "\n\t[ WARNING ]\n\n\tCMake version lower than 3.6.\n\n\t - Please update CMake and rerun; OR\n\t - Manually set 'GLFW-CMake-starter' as StartUp Project in Visual Studio.\n" )
    else()
        set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT GLFW-CMake-starter )
    endif()
endif()

