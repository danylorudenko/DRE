#pragma once

#include <gfx\pass\BasePass.hpp>
#include <gfx\renderer\GlobalGeometryManager.hpp>
#include <engine\data\Geometry.hpp>

namespace EDITOR
{
class ViewportInputManager;
}

namespace GFX
{

class EditorPass : public BasePass
{
public:
    EditorPass(EDITOR::ViewportInputManager* viewportInput);

    virtual PassID  GetID               () const override;

    // Inherited via BasePass
    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;

private:
    EDITOR::ViewportInputManager* m_ViewportInput;

    GlobalGeometry::GeometryGPU m_GizmoVertices;
    Data::Geometry* m_GizmoGeometry = nullptr;
};

}

