#include "DREApplicationDelegate.hpp"

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
#include <engine\ApplicationContext.hpp>


////////////////
constexpr bool C_COMPILE_GLSL_SOURCES_ON_START = true;
////////////////

//////////////////////////////////////////
DREApplicationDelegate::DREApplicationDelegate(HINSTANCE instance, char const* title, std::uint32_t windowWidth, std::uint32_t windowHeight, std::uint32_t buffering, bool vkDebug, bool imguiEnabled)
    : m_MainWindow {
        instance,
        title,
        windowWidth,
        windowHeight,
        "VulkanRenderWindow",
        DREApplicationDelegate::WinProc,
        this }
    , m_InputSystem{ m_MainWindow.NativeHandle() }
    , m_MaterialLibrary{ &DRE::g_MainAllocator }
    , m_GeometryLibrary{ &DRE::g_MainAllocator }
    , m_IOManager{ &DRE::g_MainAllocator, &m_MaterialLibrary, &m_GeometryLibrary }
    , m_GraphicsManager{ instance, &m_MainWindow, &m_IOManager, vkDebug }
    , m_ImGuiEnabled{ imguiEnabled }
    , m_MainScene{ &DRE::g_MainAllocator }
    , m_RootEditor{ &m_MainScene }
    , m_WaterGeometry{ sizeof(Data::DREVertex), 4 }
    , m_WaterMaterial{ "water_mat" }
    , m_BeachMaterial{ "beach_mat" }
{
    WORLD::g_MainScene = &m_MainScene;
}

//////////////////////////////////////////
DREApplicationDelegate::~DREApplicationDelegate()
{
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
//////////////////////////////////////////
LRESULT DREApplicationDelegate::WinProc(HWND handle, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(handle, message, wparam, lparam))
        return true;

    auto* appDelegate = reinterpret_cast<DREApplicationDelegate*>(::GetWindowLongPtr(handle, GWLP_USERDATA));
    
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
InputSystem& DREApplicationDelegate::GetInputSystem()
{
    return m_InputSystem;
}

WORLD::Scene& DREApplicationDelegate::GetMainScene()
{
    return m_MainScene;
}

void DREApplicationDelegate::start()
{
    if (C_COMPILE_GLSL_SOURCES_ON_START)
    {  
        m_IOManager.CompileGLSLSources();
    }
    m_IOManager.LoadShaderBinaries();

    m_MainScene.GetMainCamera().SetFOV(60.0f);
    //m_MainScene.GetMainCamera().SetPosition(glm::vec3{ 7.28f, 5.57f, -1.07f });
    //m_MainScene.GetMainCamera().SetEulerOrientation(glm::vec3{ -17.26f, 107.37f, 0.0f });

    m_MainScene.GetMainCamera().SetPosition(glm::vec3{ -0.23f, 10.41f, 14.70f });
    m_MainScene.GetMainCamera().SetCameraEuler(glm::vec3{ -13.32f, -43.83f, 0.0f });

    WORLD::Light* sunLight = m_MainScene.CreateSunLight(m_GraphicsManager.GetMainContext());
    m_MainScene.SetMainSunLight(sunLight);

    sunLight->SetEulerOrientation(glm::vec3{ -70.0f, 110.0f, 0.0f });
    sunLight->ScheduleUpdateGPUData();


    if (m_ImGuiEnabled)
        InitImGui();


    m_GraphicsManager.LoadDefaultData();



    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    glm::mat spheresTransform = glm::rotate(glm::identity<glm::mat4>(), glm::radians(180.0f), glm::vec3{ 1.0f, 0.0, 0.0f });
    //spheresTransform[3][3] += 5.0f;
    WORLD::SceneNode* sponzaNode = m_IOManager.ParseModelFile("data\\Sponza\\glTF\\Sponza.gltf", m_MainScene, "default_pbr");
    sponzaNode->SetScale(0.1f);


    m_IOManager.ParseModelFile("data\\MetalRoughSpheres\\glTF\\MetalRoughSpheres.gltf", m_MainScene, "gltf_spheres", spheresTransform, Data::TEXTURE_VARIATION_RGBA);

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
    waterEntity->GetSceneNode()->SetName("water");

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
    beachEntity->GetSceneNode()->SetName("beach_plane");

    Data::Texture2D blueNoise256 = m_IOManager.ReadTexture2D("textures\\blue_noise_rgba.png", Data::TEXTURE_VARIATION_RGBA);
    m_GraphicsManager.GetTextureBank().LoadTexture2DSync("blue_noise_256", 256, 256, VKW::FORMAT_R8G8B8A8_UNORM, blueNoise256.GetBuffer());


    ////////////
    m_GraphicsManager.GetMainContext().FlushAll();
}

//////////////////////////////////////////
void DREApplicationDelegate::update()
{
    ////////////////////////////////////////////////////
    // Frame time
    DRE::g_AppContext.m_DeltaTimeUS = m_FrameStopwatch.CurrentMicroseconds();
    DRE::g_AppContext.m_TimeSinceStartUS = m_GlobalStopwatch.CurrentMicroseconds();
    DRE::g_AppContext.m_SystemTimeUS = DRE::Stopwatch::GlobalTimeMicroseconds();
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

    if (DRE::g_AppContext.m_PauseTime != m_GlobalStopwatch.IsPaused())
    {
        if (DRE::g_AppContext.m_PauseTime)
        {
            m_GlobalStopwatch.Pause();
        }
        else
        {
            m_GlobalStopwatch.Unpause();
        }
    }

    m_GraphicsManager.RenderFrame(DRE::g_AppContext.m_EngineFrame, DRE::g_AppContext.m_DeltaTimeUS, m_GlobalStopwatch.CurrentSeconds());

    DRE::g_AppContext.m_EngineFrame++;
}

//////////////////////////////////////////
void DREApplicationDelegate::shutdown()
{
    m_GraphicsManager.WaitIdle();
    m_GraphicsManager.GetTextureBank().UnloadAllTextures();
    DestroyImGui();
}

//////////////////////////////////////////
void DREApplicationDelegate::InitImGui()
{
    // Window* window, InputSystem* input, VKW::Instance& instance, VKW::Swapchain& swapchain, VKW::Device& device, VKW::Context& context
    m_ImGuiHelper = std::make_unique<ImGuiHelper>(&m_MainWindow, &m_InputSystem, *m_GraphicsManager.GetInstance(), *m_GraphicsManager.GetSwapchain(), *m_GraphicsManager.GetMainDevice(), m_GraphicsManager.GetMainContext());
}

void DREApplicationDelegate::DestroyImGui()
{
    m_ImGuiHelper.reset();
}

//////////////////////////////////////////
void DREApplicationDelegate::ImGuiUser()
{
    m_RootEditor.Render();
    //ImGui::ShowDemoWindow();

    if (/*m_ImGuiEnabled*/false) {
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
            ImGui::Text("DT: %f ms", static_cast<double>(DRE::g_AppContext.m_DeltaTimeUS) / 1000);
            ImGui::Text("FPS: %f", 1.0 / (static_cast<double>(DRE::g_AppContext.m_DeltaTimeUS) / 1000000));
            ImGui::Text("Global T(s): %f", m_GlobalStopwatch.CurrentSeconds());
        }
        ImGui::End();

        ImGui::SetNextWindowPos(ImVec2(1100.0f, 100.0f), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(0.0f, 0.0f), ImGuiCond_Always);

        if (ImGui::Begin("Camera Controls", nullptr, ImGuiWindowFlags_NoResize))
        {
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
            ImGui::Checkbox("Pause time", &DRE::g_AppContext.m_PauseTime);
            if(ImGui::Button("Reset global time"))
            {
                m_GlobalStopwatch.Reset();
            }
            ImGui::SliderFloat("Generic Scalar", &m_GraphicsManager.GetGraphicsSettings().m_GenericScalar, -2.0f, 2.0f);

        }
        ImGui::End();
    }
}
