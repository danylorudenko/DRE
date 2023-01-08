cmake_minimum_required(VERSION 3.22...3.24)

project(DRE
		VERSION 1.0
		DESCRIPTION "DR Engine"
		LANGUAGES C CXX)


#override some assimp defaults
#==========
option(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)
option(ASSIMP_BUILD_ZLIB "Build your own zlib" ON)
option(ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)
option(ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." ON)
#==========
add_subdirectory(extern/assimp)


add_subdirectory(extern/glm)
add_subdirectory(extern/SPIRV-Cross)

#imgui and stb have no CMakeLists
#==========
add_library(stb_image INTERFACE "extern/stb/stb_image.h")
target_include_directories(stb_image INTERFACE "extern/stb")

#==========
set(IMGUI_SOURCE_LIST 
	"extern/imgui/imgui.cpp" 
	"extern/imgui/imgui_draw.cpp"
	"extern/imgui/imgui_tables.cpp"
	"extern/imgui/imgui_widgets.cpp")
	
set(IMGUI_HEADER_LIST
	"extern/imgui/imgui.h"
	"extern/imgui/imgui_internal.h"
	"extern/imgui/imstb_rectpack.h"
	"extern/imgui/imstb_textedit.h"
	"extern/imgui/imstb_truetype.h")
	
add_library(imgui STATIC ${IMGUI_HEADER_LIST} ${IMGUI_SOURCE_LIST})
target_include_directories(imgui PUBLIC "extern/imgui")

#==========
#copying shaders sources and binaries to build dir
add_custom_target(shaders ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${DRE_SOURCE_DIR}/shaders"
        "${PROJECT_BINARY_DIR}/shaders"
        COMMENT "Copying shaders into binary directory")
#==========


add_subdirectory(src)
add_subdirectory(apps/DemoApp)