#include <editor\SceneGraphEditor.hpp>

#include <foundation\Common.hpp>
#include <foundation\string\InplaceString.hpp>

#include <editor\RootEditor.hpp>
#include <engine\data\Material.hpp>
#include <engine\scene\Scene.hpp>
#include <engine\scene\ISceneNodeUser.hpp>
#include <engine\ApplicationContext.hpp>

#include <gfx\renderer\RenderableObject.hpp>
#include <gfx\GraphicsManager.hpp>

#include <glm\gtc\type_ptr.hpp>

#include <imgui.h>

namespace EDITOR
{

SceneGraphEditor::SceneGraphEditor(BaseEditor* rootEditor, EditorFlags flags, WORLD::Scene* scene)
    : BaseEditor{ rootEditor, flags }
    , m_Scene{ scene }
    , m_ShowIDs{ false }
{}

SceneGraphEditor::SceneGraphEditor(SceneGraphEditor&& rhs)
    : BaseEditor{ DRE_MOVE(rhs) }
    , m_Scene{ nullptr }
{
    operator=(DRE_MOVE(rhs));
}

SceneGraphEditor& SceneGraphEditor::operator=(SceneGraphEditor&& rhs)
{
    BaseEditor::operator=(DRE_MOVE(rhs));

    DRE_SWAP_MEMBER(m_Scene);
    DRE_SWAP_MEMBER(m_ShowIDs);

    return *this;
}

void SceneGraphEditor::Render()
{
    ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
    RenderingContext context;

    bool isOpen = true;
    if (ImGui::Begin("Scene Graph Editor", &isOpen, ImGuiWindowFlags_MenuBar))
    {
        if (ImGui::BeginMenuBar())
        {
            ImGui::MenuItem("Show IDs", nullptr, &m_ShowIDs);
        }
        ImGui::EndMenuBar();

        if (ImGui::BeginChild("nodes_tree", ImVec2(150, 0), ImGuiChildFlags_Border | ImGuiChildFlags_ResizeX))
        {

            WORLD::SceneNode* root = m_Scene->GetRootNode();
            DRE::U32 const rootChildCount = root->GetChildrenCount();
            for (DRE::U32 i = 0; i < rootChildCount; ++i)
            {
                RenderSceneNodeRecursive(root->GetChild(i), context);
            }
        };
        ImGui::EndChild();

        ImGui::SameLine();

        if (ImGui::BeginChild("props_window"))
        {
            if (DRE::g_AppContext.m_FocusedObject != nullptr)
            {
                bool transformUpdated = RenderNodeProperties();
                if (DRE::g_AppContext.m_FocusedObject->GetType() == WORLD::Entity::Type::Light)
                {
                    RenderLightProperties(transformUpdated);
                }
                else if (DRE::g_AppContext.m_FocusedObject->GetType() == WORLD::Entity::Type::Entity)
                {
                    RenderEntityProperties(context);
                }
            }
            else
            {
                ImGui::TextDisabled("No Node selected");
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();

    if (!isOpen)
    {
        Close();
    }
}

bool SceneGraphEditor::RenderNodeProperties()
{
    bool wasUpdated = false;

    char const* name = DRE::g_AppContext.m_FocusedObject->GetSceneNode()->GetName();
    glm::vec3 pos = DRE::g_AppContext.m_FocusedObject->GetPosition();
    glm::vec3 rot = DRE::g_AppContext.m_FocusedObject->GetEulerOrientation();
    float scale = DRE::g_AppContext.m_FocusedObject->GetScale();

    ImGui::Text("Selected Object: %s", name);
    ImGui::Separator();

    ImGui::SeparatorText("Transform");

    ImGui::NewLine();
    ImGui::Text("Position:");
    if (ImGui::DragFloat3("##pos", glm::value_ptr(pos), 1.0f))
    {
        DRE::g_AppContext.m_FocusedObject->SetPosition(pos);
        wasUpdated = true;
    }

    ImGui::NewLine();
    ImGui::Text("Orientation:");
    if (ImGui::DragFloat3("##rot", glm::value_ptr(rot), 1.0f))
    {
        DRE::g_AppContext.m_FocusedObject->SetEulerOrientation(rot);
        wasUpdated = true;
    }

    ImGui::NewLine();
    ImGui::Text("Scale:");
    if (ImGui::DragFloat("##scale", &scale, 0.1f))
    {
        DRE::g_AppContext.m_FocusedObject->SetScale(scale);
        wasUpdated = true;
    }

    return wasUpdated;
}

char const* GetMaterialTypeString(Data::Material::RenderingProperties::MaterialType type)
{
    switch (type)
    {
    case Data::Material::RenderingProperties::MaterialType::MATERIAL_TYPE_OPAQUE:
        return "Opaque";
    case Data::Material::RenderingProperties::MaterialType::MATERIAL_TYPE_ALPHA_MASKED:
        return "Alpha Masked";
    case Data::Material::RenderingProperties::MaterialType::MATERIAL_TYPE_WATER:
        return "Water";
    default:
        return "Unknown";
    }
}

char const* GetTextureSlotString(Data::Material::TextureProperty::Slot slot)
{
    switch (slot)
    {
    case Data::Material::TextureProperty::Slot::DIFFUSE:
        return "Diffuse";
    case Data::Material::TextureProperty::Slot::NORMAL:
        return "Normal";
    case Data::Material::TextureProperty::Slot::METALNESS:
        return "Metalness";
    case Data::Material::TextureProperty::Slot::ROUGHNESS:
        return "Roughness";
    case Data::Material::TextureProperty::Slot::OCCLUSION:
        return "Occlusion";
    case Data::Material::TextureProperty::Slot::BENT_NORMAL:
        return "Bent Normal";
    case Data::Material::TextureProperty::Slot::OPACITY:
        return "Opacity";
    case Data::Material::TextureProperty::Slot::GLOSSINESS:
        return "Gloss";
    case Data::Material::TextureProperty::Slot::SPECULAR:
        return "Spec";
    case Data::Material::TextureProperty::Slot::BUMP:
        return "Bump";
    default:
        return "Unknown";
    }
}

void SceneGraphEditor::RenderEntityProperties(RenderingContext& context)
{
    ImGui::NewLine();
    ImGui::SeparatorText("Entity");
    WORLD::Entity* entity = reinterpret_cast<WORLD::Entity*>(DRE::g_AppContext.m_FocusedObject);
    if (GFX::RenderableObject* renderable = entity->GetRenderableObject())
    {
        ImGui::SeparatorText("Renderable Object");
        InstanceFlags flags = renderable->GetInstanceFlags();
        ImGui::Text("Flags hex: 0x%X", flags);
        ImGui::Text("|%d| FLAG_DUMMY", flags & INSTANCE_FLAG_DUMMY);
        ImGui::NewLine();
        
        DRE::U32 layerBits =renderable->GetLayerBits();
        ImGui::Text("Layer Bits hex: 0x%X", layerBits);
        ImGui::Text("|%d| LAYER_FORWARD",   bool(layerBits & GFX::RenderableObject::LayerToBits(GFX::RenderableObject::Layer::LAYER_FORWARD)));
        ImGui::Text("|%d| LAYER_WATER",     bool(layerBits & GFX::RenderableObject::LayerToBits(GFX::RenderableObject::Layer::LAYER_WATER)));
        ImGui::Text("|%d| LAYER_GBUFFER",   bool(layerBits & GFX::RenderableObject::LayerToBits(GFX::RenderableObject::Layer::LAYER_GBUFFER)));
        ImGui::Text("|%d| LAYER_SHADOW",    bool(layerBits & GFX::RenderableObject::LayerToBits(GFX::RenderableObject::Layer::LAYER_SHADOW)));
        ImGui::NewLine();
    }



    Data::Material* material = entity->GetMaterial();
    if (material != nullptr)
    {
        ImGui::SeparatorText("Material");
        ImGui::Text("Name: %s(GUID=%d)", material->GetName(), material->GetGfxMaterial()->GetMaterialGPU().GetID());
        ImGui::Text("Type: %s", GetMaterialTypeString(material->GetRenderingProperties().GetMaterialType()));
        ImGui::Text("Flags hex: 0x%X", material->GetRenderingProperties().GetMaterialFlags());
        ImGui::Text("|%d| USE_PBR", material->GetRenderingProperties().HasUsePBRTextures());
        ImGui::Text("|%d| USE_SPEC_GLOSS", material->GetRenderingProperties().HasUseSpecGloss());
        ImGui::Text("|%d| NORMAL_TEXTURE", material->GetRenderingProperties().HasNormalTexture());
        ImGui::Text("|%d| NORMAL_TEXTURE_INVERT_Y", material->GetRenderingProperties().HasNormalTextureInvertY());
        ImGui::Text("|%d| NORMAL_TBN", material->GetRenderingProperties().HasNormalTBN());
        ImGui::Text("|%d| METALLIC_ROUGNESS_COMBINED", material->GetRenderingProperties().HasMaterialTexturesMetallicRoughnessCombined());
        ImGui::Text("|%d| ALPHA_MASKED", material->GetRenderingProperties().HasAlphaMasked());
        ImGui::Text("|%d| BUMP_TEXTURE", material->GetRenderingProperties().HasBumpTexture());

        ImGui::NewLine();
        ImGui::Text("Textures:");

        if (ImGui::BeginTable("##textures", 2, ImGuiTableFlags_SizingFixedFit))
        {
            ImGui::TableSetupColumn("Slot", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Texture", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for(DRE::U32 i = 0; i < Data::Material::TextureProperty::Slot::MAX; ++i)
            {
                Data::Material::TextureProperty::Slot slot = static_cast<Data::Material::TextureProperty::Slot>(i);
                Data::Texture2D const& texture = material->GetTexture(slot);

                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(GetTextureSlotString(slot));

                ImGui::TableSetColumnIndex(1);
                if (texture.IsInitialized())
                {
                    GFX::Texture* gfxTexture = GFX::g_GraphicsManager->GetTextureBank().FindTexture(texture.GetName());
                    DRE_ASSERT(gfxTexture != nullptr, "Material has a texture that is not loaded in the TextureBank");

                    char const* uniqueViewLabel = context.GetNextUniqueLabel("[view]");
                    if (ImGui::Button(uniqueViewLabel, ImVec2(50, 0)))
                    {
                        DRE::g_AppContext.m_TextureInspectorViewState.m_DrawTexture = true;
                        DRE::g_AppContext.m_TextureInspectorViewState.m_TextureName = texture.GetName();
                    }
                    ImGui::SameLine();
                    ImGui::Text("(GUID=%d)%s", gfxTexture->GetShaderGlobalDescriptor().id_, texture.GetName());
                }
                else
                {
                    ImGui::TextUnformatted("None");
                }
            }

            ImGui::EndTable();
        }
    }
}

bool SceneGraphEditor::RenderLightProperties(bool wasTransformUpdated)
{
    ImGui::NewLine();
    ImGui::SeparatorText("Light");

    WORLD::Light* light = reinterpret_cast<WORLD::Light*>(DRE::g_AppContext.m_FocusedObject);
    bool needsUpdate = wasTransformUpdated;

    int currentType = light->GetLightType();
    glm::vec3 spectrum = light->GetSpectrum();
    float flux = light->GetFlux();


    // go and see "shaders\lights.h" for proper order
    const char* items[] = { "Sun", "Directional", "Point" };
    static_assert(IM_ARRAYSIZE(items) == DRE_LIGHT_TYPE_MAX, "Mismatch in the editor Light type array and engine DEFINES");
    ImGui::Text("Type:");
    if (ImGui::Combo("##type", &currentType, items, IM_ARRAYSIZE(items)))
    {
        light->SetLightType(currentType);
        needsUpdate = true;
    }

    ImGui::NewLine();
    ImGui::Text("Spectrum:");
    if (ImGui::DragFloat3("##spectrum", glm::value_ptr(spectrum), 1.0f, 0.0f, 100.0f))
    {
        light->SetSpectrum(spectrum);
        needsUpdate = true;
    }

    ImGui::NewLine();
    ImGui::Text("Flux:");
    if (ImGui::DragFloat("##flux", &flux, 1.0f, 0.0f, 100.0f))
    {
        light->SetFlux(flux);
        needsUpdate = true;
    }

    if (needsUpdate)
    {
        light->ScheduleUpdateGPUData();
    }

    return needsUpdate;
}

SceneGraphEditor::RenderingContext::RenderingContext()
    : m_CurrentID{ 0 }
{
    DRE::MemZero(m_UniqueLabel, sizeof(m_UniqueLabel));
}

char const* SceneGraphEditor::RenderingContext::GetNextUniqueLabel(char const* displayLabel)
{
    DRE::MemZero(m_UniqueLabel, sizeof(m_UniqueLabel));
    std::sprintf(m_UniqueLabel, "%s##%u", displayLabel, m_CurrentID++);
    return m_UniqueLabel;
}

DRE::String128 SceneGraphEditor::GetUniqueSceneNodeLabel(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context)
{
    char const* typeStr = "Node";
    if (node->GetNodeUser() != nullptr) // not a structural node
    {
        switch (node->GetNodeUser()->GetType())
        {
        case WORLD::ISceneNodeUser::Type::Entity:
            typeStr = "Entity";
            break;
        case WORLD::ISceneNodeUser::Type::Camera:
            typeStr = "Camera";
            break;
        case WORLD::ISceneNodeUser::Type::Light:
            typeStr = "Light";
        }
    }

    char displayLabelBuffer[128];
    char const* label = std::strlen(node->GetName()) > 0 ? node->GetName() : typeStr;

    if (m_ShowIDs)
        std::sprintf(displayLabelBuffer, "%s (%u)", label, node->GetGlobalID());
    else
        std::sprintf(displayLabelBuffer, "%s", label);

    char const* uniqueStr = context.GetNextUniqueLabel(displayLabelBuffer);
    return DRE::String64{ uniqueStr };
}

void SceneGraphEditor::RenderSceneNodeRecursive(WORLD::SceneNode* node, SceneGraphEditor::RenderingContext& context)
{
    DRE::U32 const childCount = node->GetChildrenCount();
    DRE::String128 label = GetUniqueSceneNodeLabel(node, context);

    ImGuiTreeNodeFlags const flags = childCount > 0 ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf;
    if (ImGui::TreeNodeEx(label.GetData(), flags))
    {
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        {
            DRE::g_AppContext.m_FocusedObject = node->GetNodeUser();
        }

        for (DRE::U32 i = 0; i < childCount; ++i)
        {
            RenderSceneNodeRecursive(node->GetChild(i), context);
        }

        ImGui::TreePop();
    }
}

}
