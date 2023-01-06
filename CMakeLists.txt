cmake_minimum_required(VERSION 3.24)

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

add_subdirectory(src)
add_subdirectory(apps/DemoApp)