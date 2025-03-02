set(DEMO_HEADER_LIST
	"${DRE_SOURCE_DIR}/apps/unit_tests/BuddyAllocatorTests.hpp"
	)
	
set(DEMO_SOURCE_LIST
	"${DRE_SOURCE_DIR}/apps/unit_tests/unit_main.cpp"
	)
	
add_executable(unit_tests ${DEMO_HEADER_LIST} ${DEMO_SOURCE_LIST} "${DRE_SOURCE_DIR}/NatvisFile.natvis")

target_compile_features(unit_tests PRIVATE cxx_std_20)
target_compile_definitions(unit_tests PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)

target_include_directories(unit_tests PRIVATE 
	"${DRE_SOURCE_DIR}/include"
	"${DRE_SOURCE_DIR}/apps")

target_link_libraries(unit_tests PRIVATE 
	foundation 
	vk_wrapper
	gfx
	engine
	editor
	imgui)
	
set_property(
	TARGET unit_tests 
	PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
	$<TARGET_FILE_DIR:unit_tests>)

source_group(
	PREFIX "Header Files"
	TREE "${DRE_SOURCE_DIR}/apps/unit_tests"
	FILES ${DEMO_HEADER_LIST})
	
add_custom_command(TARGET unit_tests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	"${DRE_SOURCE_DIR}/shaders"
	"$<TARGET_FILE_DIR:unit_tests>/shaders"
	COMMENT "Creating symlink to shaders in binary directory"
)


add_custom_command(TARGET unit_tests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	"${DRE_SOURCE_DIR}/textures"
	"$<TARGET_FILE_DIR:unit_tests>/textures"
	COMMENT "Creating symlink to textures in binary directory"
)

add_custom_command(TARGET unit_tests POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E create_symlink
	"${DRE_SOURCE_DIR}/extern/glTF-Sample-Models/2.0"
	"$<TARGET_FILE_DIR:unit_tests>/data"
	COMMENT "Creating symlink to data in binary directory"
)
