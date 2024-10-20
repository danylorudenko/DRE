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
	"${DRE_SOURCE_DIR}/include/gfx/pass/BasePass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/AntiAliasingPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/CausticPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/ColorEncodingPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/DebugPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/EditorPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/ForwardOpaquePass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/ImGuiRenderPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/ShadowPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/WaterPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/FFTWaterPass.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pass/PassID.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/pipeline/PipelineDB.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/renderer/DrawBatcher.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/renderer/LightsManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/renderer/RenderableObject.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/renderer/TransformsManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/DependencyManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/GraphDescriptorManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/GraphResource.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/GraphResourcesManager.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/scheduling/RenderGraph.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/texture/TextureBank.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/texture/Texture.hpp"
	"${DRE_SOURCE_DIR}/include/gfx/view/RenderView.hpp")
	
set(GFX_SOURCE_LIST
	"${DRE_SOURCE_DIR}/src/gfx/DeviceChild.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/GraphicsManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/BufferBase.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/PersistentStorage.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/ReadbackProxy.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/StorageBuffer.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/buffer/UniformProxy.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/BasePass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/AntiAliasingPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/CausticPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/ColorEncodingPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/DebugPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/EditorPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/ForwardOpaquePass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/ImGuiRenderPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/ShadowPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/WaterPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pass/FFTWaterPass.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/pipeline/PipelineDB.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/renderer/DrawBatcher.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/renderer/LightsManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/renderer/RenderableObject.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/renderer/TransformsManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/DependencyManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/GraphDescriptorManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/GraphResourcesManager.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/scheduling/RenderGraph.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/texture/TextureBank.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/texture/Texture.cpp"
	"${DRE_SOURCE_DIR}/src/gfx/view/RenderView.cpp")
	
add_library(gfx STATIC ${GFX_HEADER_LIST} ${GFX_SOURCE_LIST})

target_include_directories(gfx PUBLIC "${DRE_SOURCE_DIR}/include" "${DRE_SOURCE_DIR}/shaders")

target_compile_features(gfx PUBLIC cxx_std_20)
target_compile_definitions(gfx PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX)
#TODO: disable exceptions

target_link_libraries(gfx PUBLIC foundation engine vk_wrapper glm_static imgui)

source_group(
	TREE "${DRE_SOURCE_DIR}/include/gfx"
	PREFIX "Header Files"
	FILES ${GFX_HEADER_LIST})