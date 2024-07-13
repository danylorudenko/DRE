#include <vk_wrapper\pipeline\Pipeline.hpp>

#include <gfx\pass\BasePass.hpp>

namespace GFX
{

class Texture;

class ImGuiRenderPass
    : public BasePass
{
public:
    /////////////////////////
    ImGuiRenderPass();
    virtual ~ImGuiRenderPass();


    /////////////////////////
    virtual PassID GetID() const override;

    /////////////////////////
    virtual void RegisterResources  (RenderGraph& graph) override;
    virtual void Initialize         (RenderGraph& graph) override;
    virtual void Render             (RenderGraph& graph, VKW::Context& context) override;

private:
    Texture*            m_ImGuiAtlas;
    VKW::Pipeline*      m_GraphicsPipeline;
};

}

