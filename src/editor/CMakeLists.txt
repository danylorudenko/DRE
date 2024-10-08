set(EDITOR_HEADER_LIST
	"${DRE_SOURCE_DIR}/include/editor/ViewportInputManager.hpp"
	"${DRE_SOURCE_DIR}/include/editor/BaseEditor.hpp"
	"${DRE_SOURCE_DIR}/include/editor/CameraEditor.hpp"
	"${DRE_SOURCE_DIR}/include/editor/OceanEditor.hpp"
	"${DRE_SOURCE_DIR}/include/editor/RootEditor.hpp"
	"${DRE_SOURCE_DIR}/include/editor/SceneGraphEditor.hpp"
	"${DRE_SOURCE_DIR}/include/editor/StatsEditor.hpp"
	"${DRE_SOURCE_DIR}/include/editor/TextureInspector.hpp")

set(EDITOR_SOURCE_LIST
	"${DRE_SOURCE_DIR}/src/editor/ViewportInputManager.cpp"
	"${DRE_SOURCE_DIR}/src/editor/BaseEditor.cpp"
	"${DRE_SOURCE_DIR}/src/editor/CameraEditor.cpp"
	"${DRE_SOURCE_DIR}/src/editor/OceanEditor.cpp"
	"${DRE_SOURCE_DIR}/src/editor/RootEditor.cpp"
	"${DRE_SOURCE_DIR}/src/editor/SceneGraphEditor.cpp"
	"${DRE_SOURCE_DIR}/src/editor/StatsEditor.cpp"
	"${DRE_SOURCE_DIR}/src/editor/TextureInspector.cpp")
	
add_library(editor STATIC ${EDITOR_HEADER_LIST} ${EDITOR_SOURCE_LIST})

target_include_directories(editor PUBLIC 
	"${DRE_SOURCE_DIR}/include"
	)

target_compile_features(editor PUBLIC cxx_std_20)
target_compile_definitions(editor PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
#TODO: disable exceptions

target_link_libraries(editor PUBLIC engine foundation assimp glm_static imgui stb_image)

source_group(
	TREE "${DRE_SOURCE_DIR}/include/editor"
	PREFIX "Header Files"
	FILES ${ENGINE_HEADER_LIST})