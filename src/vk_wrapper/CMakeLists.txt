set(VK_WRAPPER_HEADER_LIST
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Constant.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Context.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Device.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Format.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Helper.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/ImportTable.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Instance.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/LogicalDevice.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Surface.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/Tools.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/descriptor/Descriptor.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/descriptor/DescriptorLayout.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/descriptor/DescriptorManager.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/descriptor/StandaloneDescriptorSet.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/memory/MemoryController.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/memory/MemoryPage.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/pipeline/Dependency.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/pipeline/Pipeline.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/pipeline/RenderPass.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/pipeline/ShaderModule.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/queue/Queue.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/queue/QueueProvider.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/resources/Framebuffer.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/resources/Resource.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/resources/ResourcesController.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/swapchain/PresentationController.hpp"
	"${DRE_SOURCE_DIR}/include/vk_wrapper/swapchain/Swapchain.hpp")
	
set(VK_WRAPPER_SOURCE_LIST
	"${DRE_SOURCE_DIR}/src/vk_wrapper/Constant.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/Context.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/Device.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/Format.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/Helper.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/ImportTable.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/Instance.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/LogicalDevice.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/Surface.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/descriptor/DescriptorLayout.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/descriptor/DescriptorManager.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/descriptor/StandaloneDescriptorSet.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/memory/MemoryController.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/memory/MemoryPage.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/pipeline/Dependency.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/pipeline/Pipeline.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/pipeline/RenderPass.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/pipeline/ShaderModule.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/queue/Queue.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/queue/QueueProvider.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/resources/Framebuffer.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/resources/Resource.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/resources/ResourcesController.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/swapchain/PresentationController.cpp"
	"${DRE_SOURCE_DIR}/src/vk_wrapper/swapchain/Swapchain.cpp")
	
add_library(vk_wrapper STATIC 
	${VK_WRAPPER_HEADER_LIST} 
	${VK_WRAPPER_SOURCE_LIST})

find_package(Vulkan REQUIRED)

target_include_directories(vk_wrapper PUBLIC 
	"${DRE_SOURCE_DIR}/include"
	"${Vulkan_INCLUDE_DIR}")

target_compile_features(vk_wrapper PUBLIC cxx_std_17)
# TODO: disable exceptions

target_compile_definitions(vk_wrapper PUBLIC 
	VK_USE_PLATFORM_WIN32_KHR 
	WIN32_LEAN_AND_MEAN 
	NOMINMAX)

target_link_libraries(vk_wrapper PUBLIC foundation)

source_group(
	TREE "${DRE_SOURCE_DIR}/include"
	PREFIX "Header Files"
	FILES ${VK_WRAPPER_HEADER_LIST})
	
	
	