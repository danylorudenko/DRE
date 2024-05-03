#pragma once

#include <foundation\class_features\NonCopyable.hpp>

#include <glm\vec2.hpp>

namespace VKW
{
class Context;
}

namespace EDITOR
{

enum EditorFlags
{
    EDITOR_FLAGS_NONE       = 0,
    EDITOR_FLAGS_STATIC     = 1
};


class BaseEditor : public NonCopyable
{
public:
    enum class Type
    {
        Root,
        Camera,
        SceneGraph,
        LightProperties,
        Transform,
        MAX
    };

    BaseEditor(BaseEditor* rootEditor, EditorFlags flags = EDITOR_FLAGS_NONE);
    BaseEditor(BaseEditor&& rhs);

    BaseEditor& operator=(BaseEditor&& rhs);

    virtual ~BaseEditor() {};


    //inline void SetPosition(glm::uvec2 pos) { m_Position = pos; }
    //inline void SetPosition(std::uint32_t xPos, std::uint32_t yPos) { m_Position.x = xPos; m_Position.y = yPos; }
    //
    //inline void SetSize(glm::uvec2 size) { m_Size = size; }
    //inline void SetSize(std::uint32_t xSize, std::uint32_t ySize) { m_Size.x = xSize; m_Size.y = ySize; }
    //
    //inline glm::uvec2 GetPosition() const { return m_Position; }
    //inline glm::uvec2 GetSize() const { return m_Size; }

    virtual Type GetType() const = 0;

    virtual void Render() = 0;
    virtual void Close();


protected:
    BaseEditor* m_RootEditor;

    glm::uvec2  m_Position;
    glm::uvec2  m_Size;
    EditorFlags m_Flags;
};

}

