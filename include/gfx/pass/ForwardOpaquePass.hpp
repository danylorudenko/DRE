#pragma once

#include <gfx\pass\BasePass.hpp>
#include <gfx\buffer\ReadbackProxy.hpp>

namespace GFX
{

class ForwardOpaquePass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    // Inherited via BasePass
    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;

};

}

