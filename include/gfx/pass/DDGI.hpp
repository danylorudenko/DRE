#pragma once

#include <foundation\Common.hpp>
#include <foundation\class_features\NonCopyable.hpp>
#include <foundation\class_features\NonMovable.hpp>
#include <common\global_illumination\ddgi_common.slang>
#include <glm\fwd.hpp>

#include <gfx\pass\BasePass.hpp>
#include <gfx\renderer\GlobalGeometryManager.hpp>

namespace Data
{
class GeometryLibrary;
}

namespace GFX
{

struct GraphicsSettings;

class DDGI final
    : public NonCopyable
    , public NonMovable
{
public:
    DDGI();

    void Initialize(Data::GeometryLibrary* geometryLibrary);

    glm::uvec3          GetProbeCount3D          () const;
    DRE::U32            GetProbeTotalCount       () const;
    DRE::U32            GetProbeDataBufferSize   () const;

    DRE::U32            GetProbeResolutionIrradiance() const;
    glm::uvec2          GetProbeAtlasIrradianceDimentions() const;

    DRE::U32            GetProbeResolutionVisibility() const;
    glm::uvec2          GetProbeAtlasVisibilityDimentions() const;

    GlobalGeometry::GeometryGPU* GetSphereGeometry() const { return m_ProbeDebugSphereGPU; }

    DDGIConstantBuffer  GetConstantBuffer        (RenderGraph& graph) const;

    static char const*  GetProbeDebugSphereGeometryName() { return "dre_sphere"; }

private:
    GlobalGeometry::GeometryGPU* m_ProbeDebugSphereGPU;
};

class DDGIProbeTracePass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override {};
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DDGIProbeBorderFillPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override {};
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DDGIProbeLightingPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override {};
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DDGIProbeVisibilityPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override {};
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DDGIProbeScatterPass : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override {};
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

class DebugPassDDGIProbeDisplay : public BasePass
{
public:
    virtual PassID  GetID               () const override;

    virtual void    RegisterResources   (RenderGraph& graph) override;
    virtual void    Initialize          (RenderGraph& graph) override;
    virtual void    Render              (RenderGraph& graph, VKW::Context& context) override;
};

}
