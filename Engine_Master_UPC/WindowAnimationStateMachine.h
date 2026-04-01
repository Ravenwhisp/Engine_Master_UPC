#pragma once
#include "EditorWindow.h"
#include "MD5Fwd.h"

#include <memory>
#include <string>

class AnimationStateMachineAsset;

namespace ax
{
    namespace NodeEditor
    {
        struct EditorContext;
        struct PinId;
        struct LinkId;
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

    bool tryGetInputStateIndex(ax::NodeEditor::PinId pinId, int& outStateIndex) const;
    bool tryGetOutputStateIndex(ax::NodeEditor::PinId pinId, int& outStateIndex) const;
    bool tryGetTransitionIndex(ax::NodeEditor::LinkId linkId, int& outTransitionIndex) const;

    bool tryResolveTransitionEndpoints(ax::NodeEditor::PinId firstPinId, ax::NodeEditor::PinId secondPinId,int& outSourceStateIndex, int& outTargetStateIndex) const;

    bool hasTransitionBetweenStates(const std::string& sourceStateName,
        const std::string& targetStateName) const;

    void markDirty();
    void sanitizeAssetAfterEdit();
    bool saveAsset();

private:
    MD5Hash m_targetStateMachineUID = INVALID_ASSET_ID;
    std::shared_ptr<AnimationStateMachineAsset> m_asset;
    ax::NodeEditor::EditorContext* m_editorContext = nullptr;
    bool m_needsInitialNodeLayout = true;
    std::string m_editorSettingsFile;
    bool m_focusContentNextFrame = false;
    bool m_isDirty = false;
};
