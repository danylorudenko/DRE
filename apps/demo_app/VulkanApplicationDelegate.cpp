#include "VulkanApplicationDelegate.hpp"

#include "AppUtils.hpp"

#include <assimp\Importer.hpp>
#include <glm\geometric.hpp>
#include <glm\ext\matrix_transform.hpp>
#include <imgui.h>

#include <utility>
#include <cstdio>

#include <vk_wrapper\Tools.hpp>
#include <vk_wrapper\Helper.hpp>
#include <vk_wrapper\resources\Framebuffer.hpp>
#include <vk_wrapper\pipeline\ShaderModule.hpp>

#include <engine\data\Geometry.hpp>


////////////////
constexpr bool C_COMPILE_GLSL_SOURCES_ON_START = true;
////////////////

//////////////////////////////////////////
VulkanApplicationDelegate::VulkanApplicationDelegate(HINSTANCE instance, char const* title, std::uint32_t windowWidth, std::uint32_t windowHeight, std::uint32_t buffering, bool vkDebug, bool imguiEnabled)
    : m_MainWindow {
        instance,
        title,
        windowWidth,
        windowHeight,
        "VulkanRenderWindow",
        VulkanApplicationDelegate::WinProc,
        this }
    , m_InputSystem{ m_MainWindow.NativeHandle() }
    , m_MaterialLibrary{ &DRE::g_MainAllocator }
    , m_GeometryLibrary{ &DRE::g_MainAllocator }
    , m_IOManager{ &DRE::g_MainAllocator, &m_MaterialLibrary, &m_GeometryLibrary }
    , m_GraphicsManager{ instance, &m_MainWindow, &m_IOManager, vkDebug }
    , m_ImGuiEnabled{ imguiEnabled }
    , m_DeltaMicroseconds{ 0 }
    , m_EngineFrame{ 0 }
    , m_MainScene{ &DRE::g_MainAllocator }
    , m_RotateSun{ false }
    , m_SunElevation{ 70.0f }
    , m_RotateCamera{ false }
    , m_PauseTime{ false }
    , m_TimeOffset{ 0.0f }
    , m_WaterGeometry{ sizeof(Data::DREVertex), 4 }
    , m_WaterMaterial{ "water_mat" }
    , m_BeachMaterial{ "beach_mat" }
{
    WORLD::g_MainScene = &m_MainScene;
}

//////////////////////////////////////////
VulkanApplicationDelegate::~VulkanApplicationDelegate()
{

}

//////////////////////////////////////////
LRESULT VulkanApplicationDelegate::WinProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    auto* appDelegate = reinterpret_cast<VulkanApplicationDelegate*>(::GetWindowLongPtr(handle, GWLP_USERDATA));
    
    switch (message)
    {
    case WM_INPUT:
    {
        UINT code = GET_RAWINPUT_CODE_WPARAM(wparam);
        LRESULT result{};
        if (code == RIM_INPUTSINK || code == RIM_INPUT)
            result = ::DefWindowProc(handle, message, wparam, lparam);

        appDelegate->GetInputSystem().ProcessSystemInput(handle, wparam, lparam); 
        return result;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        break;
    }
    
    return ::DefWindowProc(handle, message, wparam, lparam);
}

//////////////////////////////////////////
InputSystem& VulkanApplicationDelegate::GetInputSystem()
{
    return m_InputSystem;
}

WORLD::Scene& VulkanApplicationDelegate::GetMainScene()
{
    return m_MainScene;
}

void VulkanApplicationDelegate::start()
{
    if (m_ImGuiEnabled)
        InitImGui();

    if (C_COMPILE_GLSL_SOURCES_ON_START)
    {  
        m_IOManager.CompileGLSLSources();
    }
    m_IOManager.LoadShaderBinaries();

    m_MainScene.GetMainCamera().SetFOV(60.0f);
    //m_MainScene.GetMainCamera().SetPosition(glm::vec3{ 7.28f, 5.57f, -1.07f });
    //m_MainScene.GetMainCamera().SetEulerOrientation(glm::vec3{ -17.26f, 107.37f, 0.0f });

    m_MainScene.GetMainCamera().SetPosition(glm::vec3{ 0.81f, 2.82f, 2.41f });
    m_MainScene.GetMainCamera().SetCameraEuler(glm::vec3{ -24.32f, 13.83f, 0.0f });

    m_MainScene.GetMainSunLight().SetEulerOrientation(glm::vec3{ m_SunElevation, 45.0f + 180.0f, 0.0f });

    m_GraphicsManager.Initialize();



    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    glm::mat spheresTransform = glm::rotate(glm::identity<glm::mat4>(), glm::radians(180.0f), glm::vec3{ 1.0f, 0.0, 0.0f });
    //spheresTransform[3][3] += 5.0f;
    //m_IOManager.ParseModelFile("data\\Sponza\\glTF\\Sponza.gltf", m_MainScene, "default_pbr");
    //m_IOManager.ParseModelFile("data\\MetalRoughSpheres\\glTF\\MetalRoughSpheres.gltf", m_MainScene, "gltf_spheres", spheresTransform, Data::TEXTURE_VARIATION_RGBA);

    m_GraphicsManager.GetMainContext().FlushAll();


    ////////////
    DRE::ByteBuffer planeVertices;
    DRE::ByteBuffer planeIndicies;
    GeneratePlaneMesh(C_WATER_VERTEX_X, C_WATER_VERTEX_Z, planeVertices, planeIndicies);

    m_WaterGeometry.SetVertexData(DRE_MOVE(planeVertices));
    m_WaterGeometry.SetIndexData(DRE_MOVE(planeIndicies));
    Data::Texture2D waterNormalMap = m_IOManager.ReadTexture2D("textures\\water_normal0.jpg", Data::TEXTURE_VARIATION_RGBA);
    
    m_WaterMaterial.AssignTextureToSlot(Data::Material::TextureProperty::NORMAL, DRE_MOVE(waterNormalMap));
    m_WaterMaterial.GetRenderingProperties().SetMaterialType(Data::Material::RenderingProperties::MATERIAL_TYPE_WATER);
    m_WaterMaterial.GetRenderingProperties().SetShader("water");



    glm::mat4 waterTransform = glm::identity<glm::mat4>();
    //wTrans.model = glm::rotate(wTrans.model, glm::radians(180.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    waterTransform[3][1] += 1.5f;
    //wTrans.model[3][2] -= 0.4f;
    waterTransform = glm::scale(waterTransform, glm::vec3{ 0.1f });
    WORLD::Entity* waterEntity = m_MainScene.CreateOpaqueEntity(m_GraphicsManager.GetMainContext(), &m_WaterGeometry, &m_WaterMaterial);
    waterEntity->SetMatrix(waterTransform);

    ////////////
    m_BeachMaterial.GetRenderingProperties().SetMaterialType(Data::Material::RenderingProperties::MATERIAL_TYPE_OPAQUE);
    m_BeachMaterial.GetRenderingProperties().SetShader("sand_beach");
    m_BeachMaterial.AssignTextureToSlot(Data::Material::TextureProperty::DIFFUSE, m_IOManager.ReadTexture2D("textures\\wavy-sand_albedo.png", Data::TEXTURE_VARIATION_RGBA));
    m_BeachMaterial.AssignTextureToSlot(Data::Material::TextureProperty::NORMAL, m_IOManager.ReadTexture2D("textures\\wavy-sand_normal-dx.png", Data::TEXTURE_VARIATION_RGBA));
    m_BeachMaterial.AssignTextureToSlot(Data::Material::TextureProperty::METALNESS, m_IOManager.ReadTexture2D("textures\\wavy-sand_metallic.png", Data::TEXTURE_VARIATION_GRAY));
    m_BeachMaterial.AssignTextureToSlot(Data::Material::TextureProperty::ROUGHNESS, m_IOManager.ReadTexture2D("textures\\wavy-sand_roughness.png", Data::TEXTURE_VARIATION_GRAY));

    glm::mat4 beachTransform = waterTransform; // beach transform
    //bTrans.model = glm::scale(bTrans.model, glm::vec3(1.5f));
    beachTransform = glm::rotate(beachTransform, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    beachTransform[3][2] -= 6.0f;
    WORLD::Entity* beachEntity = m_MainScene.CreateOpaqueEntity(m_GraphicsManager.GetMainContext(), &m_WaterGeometry, &m_BeachMaterial); // reuse water geometry
    beachEntity->SetMatrix(beachTransform);

    Data::Texture2D blueNoise256 = m_IOManager.ReadTexture2D("textures\\blue_noise_rgba.png", Data::TEXTURE_VARIATION_RGBA);
    m_GraphicsManager.GetTextureBank().LoadTexture2DSync("blue_noise_256", 256, 256, VKW::FORMAT_R8G8B8A8_UNORM, blueNoise256.GetBuffer());


    ////////////
    m_GraphicsManager.GetMainContext().FlushAll();
}

constexpr float C_SUN_ROTATOR_MUL = 1.0f / 100000.0f;

//////////////////////////////////////////
void VulkanApplicationDelegate::update()
{
    ////////////////////////////////////////////////////
    // Frame time measurement
    m_DeltaMicroseconds = m_FrameStopwatch.CurrentMicroseconds();
    m_FrameStopwatch.Reset();
    ////////////////////////////////////////////////////

    DRE::g_FrameScratchAllocator.Reset();

    m_InputSystem.Update();

    if (m_ImGuiEnabled)
    {
        m_ImGuiHelper->BeginFrame();
        ImGuiUser();
        m_ImGuiHelper->EndFrame();
    }

    if (m_InputSystem.GetKeyboardButtonJustReleased(Keys::R))
    {
        if (m_IOManager.NewShadersPending())
        {
            m_GraphicsManager.WaitIdle();
            m_GraphicsManager.ReloadShaders();
        }
    }

    glm::vec3 const sun_orient = m_MainScene.GetMainSunLight().GetEulerOrientation();
    m_MainScene.GetMainSunLight().SetEulerOrientation(glm::vec3{ -m_SunElevation, sun_orient.y, sun_orient.z });
    if (m_RotateSun)
        m_MainScene.GetMainSunLight().Rotate(glm::vec3{ 0.0f, C_SUN_ROTATOR_MUL * m_DeltaMicroseconds, 0.0f });

    if (m_RotateCamera)
        m_MainScene.GetMainCamera().Rotate(glm::vec3{ 0.0f, C_SUN_ROTATOR_MUL * m_DeltaMicroseconds * 15.0f, 0.0f});

    if (m_PauseTime != m_GlobalStopwatch.IsPaused())
    {
        if (m_PauseTime)
            m_GlobalStopwatch.Pause();
        else
            m_GlobalStopwatch.Unpause();
    }

    float const globalTimeSeconds = static_cast<float>(m_GlobalStopwatch.CurrentMilliseconds()) * 0.001f + m_TimeOffset;
    m_GraphicsManager.RenderFrame(m_EngineFrame, m_DeltaMicroseconds, globalTimeSeconds);

    m_EngineFrame++;
}

//////////////////////////////////////////
void VulkanApplicationDelegate::shutdown()
{
}

//////////////////////////////////////////
void VulkanApplicationDelegate::InitImGui()
{
    m_ImGuiHelper = std::make_unique<ImGuiHelper>(&m_MainWindow, &m_InputSystem);
}

//////////////////////////////////////////
void VulkanApplicationDelegate::ImGuiUser()
{
    if (m_ImGuiEnabled) {
        IM_ASSERT(ImGui::GetCurrentContext() != NULL && "Missing dear imgui context. Refer to examples app!");

        ImGui::SetNextWindowContentSize(ImVec2{ 150.0f, 0.0f});
        ImGui::SetNextWindowPos(ImVec2(1100.0f, 0.0f), ImGuiCond_Once);

        ImGuiWindowFlags frameDataWindowFlags = 
            //ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoScrollbar;// | 
            //ImGuiWindowFlags_NoCollapse;
        static bool frameDataOpened = false;
        if (ImGui::Begin("Stats", nullptr, frameDataWindowFlags))
        {
            ImGui::Text("DT: %f ms", static_cast<double>(m_DeltaMicroseconds) / 1000);
            ImGui::Text("FPS: %f", 1.0 / (static_cast<double>(m_DeltaMicroseconds) / 1000000));
            ImGui::Text("Global T(s): %f", static_cast<float>(m_GlobalStopwatch.CurrentMilliseconds()) * 0.001f);
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(1100.0f, 100.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

        if (ImGui::Begin("Camera Controls", nullptr, ImGuiWindowFlags_NoResize))
        {
            WORLD::Camera& camera = m_MainScene.GetMainCamera();
            glm::vec3 const& cameraEuler = camera.GetEulerOrientation();
            glm::vec3 const& cameraForward = camera.GetForward();
            glm::vec3 const& cameraRight = camera.GetRight();

            if (ImGui::Button("Reset", ImVec2(50.0, 20.0f)))
            {
                camera.SetPosition(glm::vec3{ 0.0f, 0.0f, 0.0f });
                camera.SetEulerOrientation(glm::vec3{ 0.0f, 0.0f, 0.0f });
            }

            float const cameraMod = (static_cast<float>(m_DeltaMicroseconds) / 10000);
            float const moveMul = 0.1f;
            float const rotMul = 1.0f;

            ImGui::PushButtonRepeat(true);

            if (ImGui::ArrowButton("fwd", ImGuiDir_Up))
                camera.Move(cameraForward * moveMul * cameraMod);
            ImGui::SameLine();
            if (ImGui::ArrowButton("left", ImGuiDir_Left))
                camera.Move(-cameraRight * moveMul * cameraMod);
            ImGui::SameLine(0.0f, 20.0f);
            if (ImGui::ArrowButton("upr", ImGuiDir_Up))
                camera.RotateCamera(glm::vec3{ rotMul * cameraMod, 0.0f, 0.0f });
            ImGui::SameLine();
            if (ImGui::ArrowButton("leftr", ImGuiDir_Left))
                camera.RotateCamera(glm::vec3{ 0.0f, rotMul * cameraMod, 0.0f });
            ImGui::SameLine();
            if(ImGui::Button("UP"))
                camera.Move(glm::vec3{ 0.0f, moveMul * cameraMod, 0.0f });

            // new line
            if (ImGui::ArrowButton("back", ImGuiDir_Down))
                camera.Move(-cameraForward * moveMul * cameraMod);
            ImGui::SameLine();
            if (ImGui::ArrowButton("right", ImGuiDir_Right))
                camera.Move(cameraRight * moveMul * cameraMod);
            ImGui::SameLine(0.0f, 20.0f);
            if (ImGui::ArrowButton("downr", ImGuiDir_Down))
                camera.RotateCamera(glm::vec3{ -rotMul * cameraMod, 0.0f, 0.0f });
            ImGui::SameLine();
            if (ImGui::ArrowButton("rightr", ImGuiDir_Right))
                camera.RotateCamera(glm::vec3{ 0.0f, -rotMul * cameraMod, 0.0f });
            ImGui::SameLine();
            if (ImGui::Button("DOWN"))
                camera.Move(glm::vec3{ 0.0f, -moveMul * cameraMod, 0.0f });

            ImGui::PopButtonRepeat();

            ImGui::Text("Camera pos: %.2f, %.2f, %.2f", camera.GetPosition()[0], camera.GetPosition()[1], camera.GetPosition()[2]);
            ImGui::Text("Camera rot: %.2f, %.2f, %.2f", camera.GetEulerOrientation()[0], camera.GetEulerOrientation()[1], camera.GetEulerOrientation()[2]);

            ImGui::Checkbox("Rotate sun", &m_RotateSun);
            ImGui::SliderFloat("Sun Elevation", &m_SunElevation, 0.0f, 90.0f);
            //ImGui::SliderAngle("Sun Elevation", &m_SunElevation, 0.0f, 90.0f);


            ImGui::Checkbox("Rotate cam", &m_RotateCamera);
            if (ImGui::BeginCombo("Water settings", nullptr, ImGuiComboFlags_None))
            {
                /*
    float           m_WaterSpeed            = 1.0f;
    float           m_WaterSizeMeters       = 10.0f;
    float           m_WindDirectionX        = 0.0f;
    float           m_WindSpeed             = 1.0f;*/
                ImGui::Checkbox("Water wireframe", &m_GraphicsManager.GetGraphicsSettings().m_WaterWireframe);
                ImGui::Checkbox("FFT Water", &m_GraphicsManager.GetGraphicsSettings().m_UseFFTWater);
                ImGui::SliderFloat("Water speed", &m_GraphicsManager.GetGraphicsSettings().m_WaterSpeed, 0.0f, 3.0f);
                ImGui::SliderFloat("Water size meters", &m_GraphicsManager.GetGraphicsSettings().m_WaterSizeMeters, 1.0f, 100.0f);
                ImGui::SliderFloat("Water amplitude", &m_GraphicsManager.GetGraphicsSettings().m_WaterAmplitude, 0.0f, 1000.0f);
                ImGui::SliderFloat("Water wind dir X", &m_GraphicsManager.GetGraphicsSettings().m_WindDirectionX, -1.0f, 1.0f);
                ImGui::SliderFloat("Water wind speed", &m_GraphicsManager.GetGraphicsSettings().m_WindSpeed, 0.0f, 100.0f);
                ImGui::SliderFloat("Water wind dir factor", &m_GraphicsManager.GetGraphicsSettings().m_WindDirFactor, 0.0f, 100.0f);
                ImGui::EndCombo();
            }
            ImGui::Checkbox("Use ACES", &m_GraphicsManager.GetGraphicsSettings().m_UseACESEncoding);
            ImGui::SliderFloat("Exposure target EV", &m_GraphicsManager.GetGraphicsSettings().m_ExposureEV, -3.0f, 5.0f);

            if (ImGui::Button("Enable TAA"))
            {
                m_GraphicsManager.GetGraphicsSettings().m_AlphaTAA = 0.9f;
                m_GraphicsManager.GetGraphicsSettings().m_JitterScale = 0.35f;
            }
            ImGui::SameLine();
            if (ImGui::Button("Disable TAA"))
            {
                m_GraphicsManager.GetGraphicsSettings().m_AlphaTAA = 0.0f;
                m_GraphicsManager.GetGraphicsSettings().m_JitterScale = 0.0f;
            }

            ImGui::SliderFloat("TAA Alpha", &m_GraphicsManager.GetGraphicsSettings().m_AlphaTAA, 0.0f, 1.0f);
            ImGui::SliderFloat("TAA Jitter Scale", &m_GraphicsManager.GetGraphicsSettings().m_JitterScale, 0.0f, 2.0f);
            ImGui::SliderFloat("TAA Variance Gamma", &m_GraphicsManager.GetGraphicsSettings().m_VarianceGammaTAA, 0.0f, 5.0f);
            ImGui::Checkbox("Pause time", &m_PauseTime);
            if(ImGui::Button("Reset global time"))
            {
                m_GlobalStopwatch.Reset();
            }
            ImGui::SliderFloat("Time offset", &m_TimeOffset, -10.0f, 10.0);
            ImGui::SliderFloat("Generic Scalar", &m_GraphicsManager.GetGraphicsSettings().m_GenericScalar, -2.0f, 2.0f);

        }
        ImGui::End();
    }
}
