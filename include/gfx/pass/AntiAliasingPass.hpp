#pragma once

#include <gfx\pass\BasePass.hpp>

namespace GFX
{

class AntiAliasingPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    // Inherited via BasePass
    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;

};

}

