#pragma once

#include <gfx\pass\BasePass.hpp>
#include <gfx\renderer\GlobalGeometryManager.hpp>

namespace Data
{
class GeometryLibrary;
}

namespace GFX
{

class DebugPassDDGIProbeDisplay : public BasePass
{
public:
    DebugPassDDGIProbeDisplay(Data::GeometryLibrary* geometryLibrary);

    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;

private:
    Data::GeometryLibrary* m_GeometryLibrary;

    GlobalGeometry::GeometryGPU* m_ProbeDebugSphereGPU;
};

}
