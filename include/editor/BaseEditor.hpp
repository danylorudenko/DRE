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
        Stats,
        TextureInspector,
        RenderingSettings,
        DebugView,
        MAX
    };

    BaseEditor(BaseEditor* rootEditor, EditorFlags flags = EDITOR_FLAGS_NONE);
    BaseEditor(BaseEditor&& rhs);

    BaseEditor& operator=(BaseEditor&& rhs);

    virtual ~BaseEditor() {};

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

