cmake_minimum_required(VERSION 3.22...3.24)

project(DRE
		VERSION 1.0
		DESCRIPTION "DR Engine"
		LANGUAGES C CXX)


find_package(Git REQUIRED)
include(CmakeDREHelpers.txt)
update_git_submodule(assimp)
update_git_submodule(glm)
update_git_submodule(shaderc)
update_git_submodule(glTF-Sample-Models)
update_git_submodule(imgui)
update_git_submodule(SPIRV-Cross)
update_git_submodule(stb)
update_git_submodule(Vulkan-Headers)


#override some assimp defaults
#==========
option(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)
option(BUILD_STATIC_LIBS "Build package with shared libraries." ON) # this one is for glm natvis
option(ASSIMP_BUILD_ZLIB "Build your own zlib" ON)
option(ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)
option(ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." ON)
#==========
add_subdirectory(extern/assimp)


#override shaderc defaults
#==========
option(SHADERC_SKIP_INSTALL ON)
option(SHADERC_SKIP_TESTS "Skip building tests" ON)
option(SHADERC_SKIP_EXAMPLES "Skip building examples" ON)
option(SHADERC_SKIP_COPYRIGHT_CHECK "Skip copyright check" ON)
find_package(Python3 COMPONENTS Interpreter REQUIRED)
execute_process(COMMAND python extern/shaderc/utils/git-sync-deps)
#==========
add_subdirectory(extern/shaderc)


add_subdirectory(extern/glm)
add_subdirectory(extern/Vulkan-Headers)
add_subdirectory(extern/SPIRV-Cross)

#imgui and stb have no CMakeLists
#==========
add_library(stb_image INTERFACE "extern/stb/stb_image.h")
target_include_directories(stb_image INTERFACE "extern/stb")

#==========
#disable exceptions and rtti, cmake doesn't have built-in vars for this, so only ugly way
if(MSVC)
	string(REGEX REPLACE "/GR" "/GR-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REGEX REPLACE "/EHsc" "/EHs-c-" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	add_compile_definitions(_HAS_EXCEPTIONS=0)
endif()
#==========
set(IMGUI_SOURCE_LIST 
	"extern/imgui/imgui.cpp" 
	"extern/imgui/imgui_draw.cpp"
	"extern/imgui/imgui_tables.cpp"
	"extern/imgui/imgui_widgets.cpp"
	"extern/imgui/imgui_demo.cpp")
	
set(IMGUI_HEADER_LIST
	"extern/imgui/imgui.h"
	"extern/imgui/imgui_internal.h"
	"extern/imgui/imstb_rectpack.h"
	"extern/imgui/imstb_textedit.h"
	"extern/imgui/imstb_truetype.h"
	"extern/imgui/backends/imgui_impl_vulkan.h"
	"extern/imgui/backends/imgui_impl_win32.h")
	
add_library(imgui STATIC ${IMGUI_HEADER_LIST} ${IMGUI_SOURCE_LIST})
target_include_directories(imgui PUBLIC "extern/imgui")
target_link_libraries(imgui PUBLIC Vulkan-Headers)


add_subdirectory(src)

#==============
add_subdirectory(apps/demo_app)