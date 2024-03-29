#include <engine\transform\TransformationSystem.hpp>

#include <engine\transform\TransformComponent.hpp>
#include <utility>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\quaternion.hpp>
#include <glm\gtc\type_ptr.hpp>

namespace Transform
{

TransformationSystem::TransformationSystem() = default;

TransformationSystem::TransformationSystem(TransformationSystem&& rhs)
{
    operator=(std::move(rhs));
}

TransformationSystem& TransformationSystem::operator=(TransformationSystem&& rhs)
{
    std::swap(components_, rhs.components_);

    return *this;
}

TransformationSystem::~TransformationSystem() = default;

void TransformationSystem::Update(std::uint32_t context, TransformSystemCameraData& cameraData)
{
    glm::mat4 view_mat = glm::translate(glm::mat4(1.0f), cameraData.cameraPos);
    view_mat = glm::rotate(view_mat, cameraData.cameraEuler.x, glm::vec3(1.0f, 0.0f, 0.0f));
    view_mat = glm::rotate(view_mat, cameraData.cameraEuler.z, glm::vec3(0.0f, 0.0f, 1.0f));

    glm::vec4 view_dir = view_mat * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);

    glm::mat4 perspective_mat = glm::perspective(glm::radians(cameraData.cameraFowDegrees), cameraData.width / cameraData.height, 0.1f, 1000.0f);
    
    std::uint64_t const componentsCount = static_cast<std::uint64_t>(components_.size());
    for (std::uint64_t i = 0; i < componentsCount; ++i) {
        TransformComponent* component = components_[i];
        
        glm::mat4 scale_mat = glm::scale(glm::mat4(1.0f), component->scale_);

        glm::quat orientation_q{ glm::radians(component->orientation_) };
        glm::mat4 orientation_mat = glm::mat4_cast(orientation_q);
        glm::mat4 translate_mat = glm::translate(glm::mat4(1.0f), component->position_);

        glm::mat4 model_mat = translate_mat * orientation_mat * scale_mat;

        glm::mat4 mvp_mat = perspective_mat * view_mat * model_mat;

        //std::uint8_t* dst = reinterpret_cast<std::uint8_t*>(component->uniformProxy_.MappedPtr(context));
        std::uint8_t* dst = nullptr;
        std::memcpy(dst, glm::value_ptr(mvp_mat), sizeof(mvp_mat));
        dst += sizeof(mvp_mat);

        std::memcpy(dst, glm::value_ptr(model_mat), sizeof(model_mat));
        dst += sizeof(model_mat);

        std::memcpy(dst, glm::value_ptr(view_dir), sizeof(view_dir));
        dst += sizeof(view_dir);

        glm::vec4 const cameraPos(cameraData.cameraPos, 1.0f);
        std::memcpy(dst, glm::value_ptr(cameraData.cameraPos), sizeof(cameraPos));
        dst += sizeof(cameraPos);

        //component->uniformProxy_.Flush(context);
    }
}

TransformComponent* TransformationSystem::CreateTransformComponent(TransformComponent* parent, Render::UniformBufferWriterProxy* uniformProxy)
{
    TransformComponent* component = new TransformComponent;
    component->parent_ = parent;
    component->position_ = glm::vec3(0.0f, 0.0f, 0.0f);
    component->orientation_ = glm::vec3(0.0f, 0.0f, 0.0f);
    component->scale_ = glm::vec3(1.0f, 1.0f, 1.0f);
    if (uniformProxy != nullptr) {
        //component->uniformProxy_ = *uniformProxy;
    }

    components_.emplace_back(component);

    return component;
}

}