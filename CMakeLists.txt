cmake_minimum_required(VERSION 3.14.7 FATAL_ERROR)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
set(NAME VulkanTest)

project(${NAME})

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(TODO ON CACHE BOOL "Enable compiletime TODO messages")

# Select WSI platform (can use cmake -D)
set(WSI "USE_PLATFORM_GLFW" CACHE STRING "WSI type used by this app")
message("WSI: " ${WSI})

set(RESOURCE_INSTALL_DIR "" CACHE PATH "Path to install resources to (leave empty for running uninstalled)")

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib/static")
endif()

if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
endif()

if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
endif()

# set output directories for all builds (Debug, Release, etc.)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/lib )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_SOURCE_DIR}/bin )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

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

# Build GLFW
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

# Compile shaders
if (${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL "AMD64")
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin/glslangValidator.exe")
else()
  set(GLSL_VALIDATOR "$ENV{VULKAN_SDK}/Bin32/glslangValidator.exe")
endif()

file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/data/shaders/*.frag"
    "${PROJECT_SOURCE_DIR}/data/shaders/*.vert"
    )

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/data/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target( 
	${PROJECT_NAME}_shaders
	DEPENDS ${SPIRV_BINARY_FILES} 
	SOURCES ${GLSL_SOURCE_FILES}
	)

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
        set_target_properties( ${PROJECT_NAME} PROPERTIES COMPILE_FLAGS "/EHa")
        set_target_properties( ${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
        set_target_properties( ${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
        set_target_properties( ${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
        set_target_properties( ${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
        set_target_properties( ${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
        set_target_properties( ${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
endif()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

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

set(WSI_LIBS glfw)
add_definitions(-D${WSI})

target_link_libraries(${PROJECT_NAME} "${VULKAN_LIBRARY}" "${WSI_LIBS}" ${GLM_LIBRARIES})

#set_target_properties(${PROJECT_NAME}
#    PROPERTIES
#    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/$<0:>"
#    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/$<0:>"
#    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/$<0:>"
#)

