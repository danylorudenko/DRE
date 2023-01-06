set(DEMO_HEADER_LIST
	"${DRE_SOURCE_DIR}/apps/DemoApp/Application.hpp"
	"${DRE_SOURCE_DIR}/apps/DemoApp/ImGuiHelper.hpp"
	"${DRE_SOURCE_DIR}/apps/DemoApp/VulkanApplicationDelegate.hpp")
	
set(DEMO_SOURCE_LIST
	"${DRE_SOURCE_DIR}/apps/DemoApp/Application.cpp"
	"${DRE_SOURCE_DIR}/apps/DemoApp/ImGuiHelper.cpp"
	"${DRE_SOURCE_DIR}/apps/DemoApp/main.cpp"
	"${DRE_SOURCE_DIR}/apps/DemoApp/VulkanApplicationDelegate.cpp")
	
add_executable(demo-app ${DEMO_HEADER_LIST} ${DEMO_SOURCE_LIST})

target_compile_features(demo-app PRIVATE cxx_std_17)

target_include_directories(demo-app PRIVATE 
	"${DRE_SOURCE_DIR}/include"
	"${DRE_SOURCE_DIR}/apps")

target_link_libraries(demo-app PRIVATE 
	foundation 
	vk_wrapper
	gfx
	engine)
	
source_group(
	TREE "${DRE_SOURCE_DIR}/apps/DemoApp"
	PREFIX "Header Files"
	FILES ${DEMO_HEADER_LIST})