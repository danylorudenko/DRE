#pragma once

#include <editor\BaseEditor.hpp>

namespace EDITOR
{

class RenderingSettingsEditor : public BaseEditor
{
public:
    RenderingSettingsEditor(BaseEditor* rootEditor, EditorFlags flags);
    RenderingSettingsEditor(RenderingSettingsEditor&& rhs);

    RenderingSettingsEditor& operator=(RenderingSettingsEditor&& rhs);

    virtual ~RenderingSettingsEditor() {}

    virtual BaseEditor::Type GetType() const override { return BaseEditor::Type::RenderingSettings; }

    virtual void Render() override;
};

}
