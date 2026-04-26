#pragma once

#include <gfx\pass\BasePass.hpp>

namespace GFX
{

class DDGIProbeLightingPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

}