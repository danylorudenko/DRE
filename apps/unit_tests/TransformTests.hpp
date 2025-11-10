#pragma once

#include <cassert>
#include <cstdio>
#include <cstdlib>  // for rand, srand
#include <ctime>    // for time()

#include <foundation\Common.hpp>

#include <common\global_uniform.h>

#include <gfx\view\RenderView.hpp>

#include <glm/glm.hpp>

inline float ConvertDeviceZToViewZ(float deviceZ, glm::mat4 cameraProjM)
{
    // Using the standard mapping: z_ndc = A + B / z_view  ->  z_view = B / (z_ndc - A)
    // To avoid layout assumptions, read A from the diagonal (safe)
    const glm::mat4 P = cameraProjM;
    const float A = P[2][2];         // diagonal, safe in GLM and HLSL
    const float B = P[3][2];         // note: would be fragile if indexed directly; formula depends on your projection build
    return B / (deviceZ - A);
}

inline glm::vec3 ConvertScreenToView(glm::ivec2 pixel, float deviceZ, glm::mat4 cameraProjM, glm::mat4 cameraInvProjM, glm::vec2 viewportSize)
{
    const float zView = ConvertDeviceZToViewZ(deviceZ, cameraProjM);

    // Compute zClip robustly via matrix multiply to avoid row/column layout pitfalls:
    // clip = P * [0, 0, zView, 1]; then take .z
    const glm::mat4 P = cameraProjM;
    const float zClip = (P * glm::vec4(0.0f, 0.0f, zView, 1.0f)).z;

    const glm::vec2 pixelNDC = ((glm::vec2(pixel) + 0.5f) / viewportSize) * 2.0f - 1.0f;

    // In clip space, x/y = ndc * z_view (matching your HLSL), z = zClip, w = zView
    const glm::vec4 pixelClip(pixelNDC * zView, zClip, zView);

    const glm::mat4 iP = cameraInvProjM;
    const glm::vec4 viewH = iP * pixelClip;
    return glm::vec3(viewH) / viewH.w;
}

inline glm::vec3 ConvertScreenToWorld(glm::ivec2 pixel, float deviceZ, glm::mat4 cameraProjM, glm::mat4 cameraInvProjM, glm::vec2 viewportSize)
{
    const float zView = ConvertDeviceZToViewZ(deviceZ, cameraProjM);

    const glm::mat4 P = cameraProjM;
    const float zClip = (P * glm::vec4(0.0f, 0.0f, zView, 1.0f)).z;

    const glm::vec2 pixelNDC = ((glm::vec2(pixel) + 0.5f) / viewportSize) * 2.0f - 1.0f;
    const glm::vec4 pixelClip(pixelNDC * zView, zClip, zView);

    const glm::mat4 iVP = cameraInvProjM;
    const glm::vec4 worldH = iVP * pixelClip;
    return glm::vec3(worldH) / worldH.w;
}



bool UnprojectionTests()
{
    GlobalUniforms globalUniform;
    GFX::RenderView view;

    glm::vec3 cameraPos{};
    glm::vec3 cameraDir{};

    view.UpdateViewport(glm::uvec2{ 0, 0 }, glm::uvec2{ 1600, 900 });
    view.UpdatePlacement(cameraPos, cameraDir, glm::vec3{ 0, 1, 0 });
    view.UpdateProjection(60.0f, 0.1f, 1000.0f);


    //globalUniform.main_CameraPos_GenericScalar = glm::vec4{ scene.GetMainCamera().GetPosition(), GetGraphicsSettings().m_GenericScalar };
    //globalUniform.main_CameraDir = glm::vec4{ scene.GetMainCamera().GetForward(), 0.0f };
    //globalUniform.main_Jitter = glm::vec4{ taaJitter, 0.0f, 0.0f };

    globalUniform.main_ViewM = view.GetViewM();
    globalUniform.main_iViewM = view.GetInvViewM();
    globalUniform.main_ProjM = view.GetProjectionM();
    globalUniform.main_ViewProjM = view.GetViewProjectionM();
    globalUniform.main_iViewProjM = view.GetInvViewProjectionM();

    globalUniform.main_PrevViewM = view.GetPrevViewM();
    globalUniform.main_PreviViewM = view.GetPrevInvViewM();
    globalUniform.main_PrevProjM = view.GetPrevProjectionM();
    globalUniform.main_PrevViewProjM = view.GetPrevViewProjectionM();
    globalUniform.main_PreviViewProjM = view.GetPrevInvViewProjectionM();

    return true;
}
