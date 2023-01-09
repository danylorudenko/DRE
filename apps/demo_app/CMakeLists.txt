set(DEMO_HEADER_LIST
	"${DRE_SOURCE_DIR}/apps/demo_app/Application.hpp"
	"${DRE_SOURCE_DIR}/apps/demo_app/ImGuiHelper.hpp"
	"${DRE_SOURCE_DIR}/apps/demo_app/VulkanApplicationDelegate.hpp")
	
set(DEMO_SOURCE_LIST
	"${DRE_SOURCE_DIR}/apps/demo_app/Application.cpp"
	"${DRE_SOURCE_DIR}/apps/demo_app/ImGuiHelper.cpp"
	"${DRE_SOURCE_DIR}/apps/demo_app/main.cpp"
	"${DRE_SOURCE_DIR}/apps/demo_app/VulkanApplicationDelegate.cpp")
	
add_executable(demo_app ${DEMO_HEADER_LIST} ${DEMO_SOURCE_LIST})

target_compile_features(demo_app PRIVATE cxx_std_17)
target_compile_definitions(demo_app PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)

target_include_directories(demo_app PRIVATE 
	"${DRE_SOURCE_DIR}/include"
	"${DRE_SOURCE_DIR}/apps")

target_link_libraries(demo_app PRIVATE 
	foundation 
	vk_wrapper
	gfx
	engine
	imgui)
	
set_property(
	TARGET demo_app 
	PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
	$<TARGET_FILE_DIR:demo_app>)

source_group(
	TREE "${DRE_SOURCE_DIR}/apps/demo_app"
	PREFIX "Header Files"
	FILES ${DEMO_HEADER_LIST})