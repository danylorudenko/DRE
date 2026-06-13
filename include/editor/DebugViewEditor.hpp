#pragma once

#include <editor\BaseEditor.hpp>

namespace EDITOR
{

class DebugViewEditor : public BaseEditor
{
public:
    DebugViewEditor(BaseEditor* rootEditor, EditorFlags flags);
    DebugViewEditor(DebugViewEditor&& rhs);

    DebugViewEditor& operator=(DebugViewEditor&& rhs);

    virtual ~DebugViewEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::DebugView; }

    virtual void Render() override;
};

}
