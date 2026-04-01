#pragma once
#include "EditorWindow.h"
#include "MD5Fwd.h"

#include <memory>

class AnimationStateMachineAsset;

namespace ax
{
    namespace NodeEditor
    {
        struct EditorContext;
    }
}

class WindowAnimationStateMachine final : public EditorWindow
{
public:
    WindowAnimationStateMachine();
    ~WindowAnimationStateMachine() override = default;

    const char* getWindowName() const override
    {
        return "Animation State Machine";
    }

    void cleanUp() override;

    void setTargetStateMachineUID(const MD5Hash& uid);
    const MD5Hash& getTargetStateMachineUID() const
    {
        return m_targetStateMachineUID;
    }

protected:
    void drawInternal() override;

private:
    bool ensureAssetLoaded();
    bool ensureEditorContext();

    void destroyEditorContext();

    void drawHeaderUi();
    void drawAssetSummaryUi() const;

    void drawGraphContent();
    void drawUnavailableGraphMessage(const char* message);

    void drawStateNodes();
    void drawTransitionLinks();

    void finalizeInitialLayout();
    void finalizeGraphFocus();

private:
    MD5Hash m_targetStateMachineUID = INVALID_ASSET_ID;
    std::shared_ptr<AnimationStateMachineAsset> m_asset;
    ax::NodeEditor::EditorContext* m_editorContext = nullptr;
    bool m_needsInitialNodeLayout = true;
    std::string m_editorSettingsFile;
    bool m_focusContentNextFrame = false;
};
