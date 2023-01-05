set(ENGINE_HEADER_LIST
	"${DRE_SOURCE_DIR}/include/engine/data/Material.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/MaterialLibrary.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/Model.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/ModelMesh.hpp"
	"${DRE_SOURCE_DIR}/include/engine/data/Texture2D.hpp"
	"${DRE_SOURCE_DIR}/include/engine/io/DRFX.hpp"
	"${DRE_SOURCE_DIR}/include/engine/io/IOManager.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/Camera.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/Entity.hpp"
	"${DRE_SOURCE_DIR}/include/engine/scene/Scene.hpp"
	"${DRE_SOURCE_DIR}/include/engine/transform/TransformComponent.hpp"
	"${DRE_SOURCE_DIR}/include/engine/transform/TransformationSystem.hpp"

set(ENGINE_SOURCE_LIST
	"${DRE_SOURCE_DIR}/src/engine/data/Material.cpp"
	"${DRE_SOURCE_DIR}/src/engine/data/MaterialLibrary.cpp"
	"${DRE_SOURCE_DIR}/src/engine/data/Model.cpp"
	"${DRE_SOURCE_DIR}/src/engine/data/Texture2D.cpp"
	"${DRE_SOURCE_DIR}/src/engine/io/DRFX.cpp"
	"${DRE_SOURCE_DIR}/src/engine/io/IOManager.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/Camera.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/Entity.cpp"
	"${DRE_SOURCE_DIR}/src/engine/scene/Scene.cpp"
	"${DRE_SOURCE_DIR}/src/engine/transform/TransformationSystem.cpp")
	
add_library(engine STATIC ${ENGINE_HEADER_LIST} ${ENGINE_SOURCE_LIST})

target_include_directories(engine PUBLIC "${DRE_SOURCE_DIR}/include")

target_compile_features(engine PUBLIC cxx_std_17)
#TODO: disable exceptions

target_link_libraries(engine PUBLIC foundation vk_wrapper)

source_group(
	TREE "${DRE_SOURCE_DIR}/include"
	PREFIX "Header Files"
	FILES ${ENGINE_HEADER_LIST})