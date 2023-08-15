set(ENGINE_HEADER_LIST
	"${DRE_SOURCE_DIR}/include/engine/data/Geometry.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/GeometryLibrary.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/Material.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/MaterialLibrary.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/Texture2D.hpp"
	"${DRE_SOURCE_DIR}/include/engine/io/DRFX.hpp"
	"${DRE_SOURCE_DIR}/include/engine/io/IOManager.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/Camera.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/Entity.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/Light.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/SceneNode.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/Scene.hpp")

set(ENGINE_SOURCE_LIST
	"${DRE_SOURCE_DIR}/src/engine/data/Geometry.cpp"
	"${DRE_SOURCE_DIR}/src/engine/data/GeometryLibrary.cpp"
	"${DRE_SOURCE_DIR}/src/engine/data/Material.cpp"
	"${DRE_SOURCE_DIR}/src/engine/data/MaterialLibrary.cpp"
	"${DRE_SOURCE_DIR}/src/engine/data/Texture2D.cpp"
	"${DRE_SOURCE_DIR}/src/engine/io/DRFX.cpp"
	"${DRE_SOURCE_DIR}/src/engine/io/IOManager.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/Camera.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/Entity.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/Light.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/SceneNode.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/Scene.cpp")
	
add_library(engine STATIC ${ENGINE_HEADER_LIST} ${ENGINE_SOURCE_LIST})

target_include_directories(engine PUBLIC 
	"${DRE_SOURCE_DIR}/include"
	)

target_compile_features(engine PUBLIC cxx_std_20)
target_compile_definitions(gfx PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
#TODO: disable exceptions

target_link_libraries(engine PUBLIC foundation vk_wrapper assimp glm::glm imgui stb_image spirv-cross-cpp spirv-cross-reflect shaderc)

source_group(
	TREE "${DRE_SOURCE_DIR}/include/engine"
	PREFIX "Header Files"
	FILES ${ENGINE_HEADER_LIST})