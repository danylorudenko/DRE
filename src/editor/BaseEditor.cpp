#include <editor\BaseEditor.hpp>

#include <foundation\Common.hpp>

namespace EDITOR
{

BaseEditor::BaseEditor(EditorFlags flags)
    : m_Position{ 0, 0 }
    , m_Size{ 100, 100 }
    , m_Flags{ flags }
{}

BaseEditor::BaseEditor(BaseEditor&& rhs)
    : m_Position{ 0, 0 }
    , m_Size{ 0, 0 }
    , m_Flags{ EDITOR_FLAGS_NONE }
{
    operator=(DRE_MOVE(rhs));
}

BaseEditor& BaseEditor::operator=(BaseEditor&& rhs)
{
    DRE_SWAP_MEMBER(m_Position);
    DRE_SWAP_MEMBER(m_Size);
    DRE_SWAP_MEMBER(m_Flags);

    return *this;
}

}