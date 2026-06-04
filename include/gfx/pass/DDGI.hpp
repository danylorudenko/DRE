#pragma once

#include <foundation\Common.hpp>
#include <common\global_illumination\ddgi_common.slang>
#include <glm\fwd.hpp>

#include <gfx\pass\BasePass.hpp>
#include <gfx\renderer\GlobalGeometryManager.hpp>

namespace Data
{
class GeometryLibrary;
}

namespace GFX::DDGI
{
    glm::uvec3  GetProbeCount3D();
    DRE::U32    GetProbeTotalCount();
    DRE::U32    GetProbeDataBufferSize();

    DDGIConstantBuffer GetConstantBuffer(DRE::U32 probeSphereVertexCount, DRE::U32 probeSphereIndexCount);

    inline char const* GetProbeDebugSphereGeometryName() { return "dre_sphere"; }
}

namespace GFX
{

class DDGIProbeTracePass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DDGIProbeBlendPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DDGIProbeLightingPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DDGIProbeScatterPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

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
