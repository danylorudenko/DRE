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

private:

    static DRE::U32 ObjectIDFromBuffer  (void* ptr, DRE::U32 x, DRE::U32 y);
};

}

