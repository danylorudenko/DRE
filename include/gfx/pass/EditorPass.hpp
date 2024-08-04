#pragma once

#include <gfx\pass\BasePass.hpp>
#include <gfx\buffer\BufferBase.hpp>
#include <engine\data\Geometry.hpp>

namespace GFX
{

class EditorPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    // Inherited via BasePass
    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;

private:
    VKW::BufferResource* m_GizmoVertices = nullptr;
    Data::Geometry* m_GizmoGeometry = nullptr;
};

}

