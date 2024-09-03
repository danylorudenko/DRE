#include <editor\CameraEditor.hpp>

#include <editor\RootEditor.hpp>
#include <foundation\Common.hpp>
#include <engine\scene\Camera.hpp>
#include <engine\ApplicationContext.hpp>

#include <imgui.h>

namespace EDITOR
{

CameraEditor::CameraEditor(BaseEditor* rootEditor, EditorFlags flags, WORLD::Camera* camera)
    : BaseEditor{ rootEditor, flags }
    , m_Camera{ camera }
{}

CameraEditor::CameraEditor(CameraEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_Camera{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

CameraEditor& CameraEditor::operator=(CameraEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_Camera);

    return *this;
}

void CameraEditor::Render()
{
    bool isOpen = true;
    if (ImGui::Begin("Camera Controls", &isOpen))
    {
        WORLD::Camera& camera = *m_Camera;
        glm::vec3 const& cameraEuler = camera.GetEulerOrientation();
        glm::vec3 const& cameraForward = camera.GetForward();
        glm::vec3 const& cameraRight = camera.GetRight();

        if (ImGui::Button("Reset", ImVec2(50.0, 20.0f)))
        {
            camera.SetPosition(glm::vec3{ 0.0f, 0.0f, 0.0f });
            camera.SetCameraEuler(glm::vec3{ 0.0f, 0.0f, 0.0f });
        }

        float const cameraMod = (static_cast<float>(DRE::g_AppContext.m_DeltaTimeUS) / 10000);
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
        if (ImGui::Button("UP"))
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
    }

    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

}