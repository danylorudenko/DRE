set(GFX_HEADER_LIST
	"${DRE_SOURCE_DIR}/include/gfx/DeviceChild.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/FrameID.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/GraphicsManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/buffer/BufferBase.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/buffer/PersistentStorage.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/buffer/ReadbackProxy.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/buffer/StorageBuffer.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/buffer/TransientArena.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/buffer/UniformProxy.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/buffer/VertexBuffer.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/BasePass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/ForwardOpaquePass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/ImGuiRenderPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/PassID.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pipeline/PipelineDB.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/renderer/DrawBatcher.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/renderer/RenderableObject.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/DependencyManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/GraphDescriptorManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/GraphResource.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/GraphResourcesManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/RenderGraph.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/texture/ReadOnlyTexture.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/texture/StorageTexture.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/texture/TextureBank.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/texture/TextureBase.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/view/RenderView.hpp")
	
set(GFX_SOURCE_LIST
	"${DRE_SOURCE_DIR}/src/gfx/DeviceChild.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/GraphicsManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/BufferBase.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/PersistentStorage.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/ReadbackProxy.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/StorageBuffer.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/UniformProxy.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/VertexBuffer.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/BasePass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/ForwardOpaquePass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/ImGuiRenderPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/ImGuiRenderPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pipeline/PipelineDB.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/renderer/DrawBatcher.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/renderer/RenderableObject.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/DependencyManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/GraphDescriptorManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/GraphResourcesManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/RenderGraph.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/texture/ReadOnlyTexture.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/texture/StorageTexture.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/texture/TextureBank.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/texture/TextureBase.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/view/RenderView.cpp")
	
add_library(gfx STATIC ${GFX_HEADER_LIST} ${GFX_SOURCE_LIST})

target_include_directories(gfx PUBLIC "${DRE_SOURCE_DIR}/include" "${DRE_SOURCE_DIR}/shaders")

target_compile_features(gfx PUBLIC cxx_std_17)
target_compile_definitions(gfx PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
#TODO: disable exceptions

target_link_libraries(gfx PUBLIC foundation vk_wrapper glm::glm imgui)

add_dependencies(gfx shaders)

source_group(
	TREE "${DRE_SOURCE_DIR}/include/gfx"
	PREFIX "Header Files"
	FILES ${GFX_HEADER_LIST})