cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(NAME VulkanTest)

project(${NAME})

set(TODO ON CACHE BOOL "Enable compiletime TODO messages")

# Select WSI platform (can use cmake -D)
set(WSI "USE_PLATFORM_GLFW" CACHE STRING "WSI type used by this app")
message("WSI: " ${WSI})

set(RESOURCE_INSTALL_DIR "" CACHE PATH "Path to install resources to (leave empty for running uninstalled)")

# Find Vulkan SDK
if (NOT DEFINED {VULKAN_SDK})
	if (NOT DEFINED ENV{VULKAN_SDK})
		message(FATAL_ERROR "VULKAN_SDK not found!")
	endif()

	if (CYGWIN)
		execute_process(COMMAND cygpath "$ENV{VULKAN_SDK}" OUTPUT_VARIABLE VULKAN_SDK)
		string(STRIP ${VULKAN_SDK} VULKAN_SDK)
	else()
		set(VULKAN_SDK "$ENV{VULKAN_SDK}")
	endif()
endif()
message("Vulkan SDK path: " ${VULKAN_SDK})

# Compile shaders
#set(GLSL_DEBUG_FLAG $<$<CONFIG:Debug>:-g>)
#set(GLSL_COMPILER ${VULKAN_SDK}/bin/glslc -mfmt=num ${GLSL_DEBUG_FLAG})

set(VERT_SHADER "${CMAKE_SOURCE_DIR}/data/shaders/triangle.vert")
#set(VERT_SHADER_INCLUDE ${VERT_SHADER}.spv.inl)
#add_custom_command(
#	COMMENT "Compiling vertex shader"
#	MAIN_DEPENDENCY ${VERT_SHADER}
#	OUTPUT ${VERT_SHADER_INCLUDE}
#	COMMAND ${GLSL_COMPILER} -o ${VERT_SHADER_INCLUDE} ${VERT_SHADER}
#	#VERBATIM -- TODO breaks empty generator-expression
#)

set(FRAG_SHADER "${CMAKE_SOURCE_DIR}/data/shaders/triangle.frag")
#set(FRAG_SHADER_INCLUDE ${FRAG_SHADER}.spv.inl)
#add_custom_command(
#	COMMENT "Compiling fragment shader"
#	MAIN_DEPENDENCY ${FRAG_SHADER}
#	OUTPUT ${FRAG_SHADER_INCLUDE}
#	COMMAND ${GLSL_COMPILER} -o ${FRAG_SHADER_INCLUDE} ${FRAG_SHADER}
#	#VERBATIM -- TODO breaks empty generator-expression
#)

add_custom_target(
	${PROJECT_NAME}_shaders
	COMMENT "Compiling shaders"
        SOURCES ${VERT_SHADER} ${FRAG_SHADER}
	DEPENDS ${VERT_SHADER_INCLUDE} ${FRAG_SHADER_INCLUDE}
)

# Build GLFW
if (${WSI} STREQUAL "USE_PLATFORM_GLFW")
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

	if (CYGWIN)
		# Hack to make GLFW use Win32 WSI instead of X11
		# TODO: Might be cleaner to use cross-compiling
		unset(UNIX)
		set(WIN32 1)
	endif()

	add_subdirectory(external/glfw)
endif()

# glm
include_directories(external/glm)

set(GLM_LIB_NAME "external/glm")
set(GLM_INC_PATH ${GLM_LIB_NAME}/glm)

set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)

add_subdirectory(${GLM_LIB_NAME})


# VulkanTest binary
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# file(GLOB_RECURSE SOURCES "src/*.h" "src/*.cpp" )
file(GLOB SOURCE_HEADERS "src/*.h*")
file(GLOB SOURCE_FILES "src/*.cpp")

add_executable(${PROJECT_NAME} WIN32 ${SOURCE_HEADERS} ${SOURCE_FILES})
add_dependencies(${PROJECT_NAME} ${PROJECT_NAME}_shaders )

if(RESOURCE_INSTALL_DIR)
	add_definitions(-DVK_DATA_DIR=\"${RESOURCE_INSTALL_DIR}/\")
	install(DIRECTORY data/ DESTINATION ${RESOURCE_INSTALL_DIR}/)
else()
	add_definitions(-DVK_DATA_DIR=\"${CMAKE_SOURCE_DIR}/data/\")
endif()


if (NOT TODO)
	add_definitions(-DNO_TODO)
endif()

if (MSVC)
	target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:_CONSOLE>)
        #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
        set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/EHa")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
        set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
        set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
endif()

if ("${CMAKE_VERSION}" VERSION_LESS 3.8.2)
	set_target_properties(
		${PROJECT_NAME}
		PROPERTIES
			CXX_STANDARD 17
			CXX_STANDARD_REQUIRED YES
			CXX_EXTENSIONS NO
	)
else()
	target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
endif()

find_path(
	VULKAN_INCLUDE vulkan/vulkan.h
	PATHS "${VULKAN_SDK}/Include" "${VULKAN_SDK}/include"
	NO_DEFAULT_PATH
)
message("Vulkan include dir: " ${VULKAN_INCLUDE})
include_directories( "${VULKAN_INCLUDE}" "src/" )

if (CYGWIN)
	set(CMAKE_FIND_LIBRARY_PREFIXES "")
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".dll")
endif()

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(VULKAN_LIBRARY_DIRS "${VULKAN_SDK}/lib" "${VULKAN_SDK}/Lib")
elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(VULKAN_LIBRARY_DIRS "${VULKAN_SDK}/Lib32")
endif()

find_library(
	VULKAN_LIBRARY
	NAMES vulkan vulkan-1
	PATHS ${VULKAN_LIBRARY_DIRS}
	NO_DEFAULT_PATH
)
message( "Vulkan libs: " ${VULKAN_LIBRARY} )

if (${WSI} STREQUAL "USE_PLATFORM_GLFW")
	set(WSI_LIBS glfw)
elseif (${WSI} STREQUAL "VK_USE_PLATFORM_WIN32_KHR")
	set( WSI_LIBS )
	add_definitions( -DUNICODE )
elseif (${WSI} STREQUAL "VK_USE_PLATFORM_XLIB_KHR")
	set( WSI_LIBS X11 )
elseif (${WSI} STREQUAL "VK_USE_PLATFORM_XCB_KHR")
	set( WSI_LIBS xcb xcb-keysyms )
elseif (${WSI} STREQUAL "VK_USE_PLATFORM_WAYLAND_KHR")
	set(WSI_LIBS wayland-client xkbcommon)
endif()
add_definitions(-D${WSI})

target_link_libraries(${PROJECT_NAME} "${VULKAN_LIBRARY}" "${WSI_LIBS}" ${GLM_LIBRARIES})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin/)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin"
)

