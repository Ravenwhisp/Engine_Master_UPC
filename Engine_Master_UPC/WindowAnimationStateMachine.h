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
        struct NodeId;
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
    void handleCreateTransitionInteraction();
    void handleLinkContextMenuInteraction();
    void drawLinkContextMenuPopup();
    void handleBackgroundContextMenuInteraction();
    void drawBackgroundContextMenuPopup();
    void handleNodeContextMenuInteraction();
    void drawNodeContextMenuPopup();
    void requestGraphEditorReset(bool clearSavedLayout);
    void applyPendingGraphEditorReset();
    void drawSaveControlsUi();

    bool tryGetStateIndex(ax::NodeEditor::NodeId nodeId, int& outStateIndex) const;
    bool renameStateAndReferences(int stateIndex, const std::string& newStateName);

    bool hasStateWithName(const std::string& stateName) const;
    std::string makeUniqueStateName(const std::string& baseStateName) const;

    void finalizeInitialLayout();
    void finalizeGraphFocus();

    bool tryGetInputStateIndex(ax::NodeEditor::PinId pinId, int& outStateIndex) const;
    bool tryGetOutputStateIndex(ax::NodeEditor::PinId pinId, int& outStateIndex) const;
    bool tryGetTransitionIndex(ax::NodeEditor::LinkId linkId, int& outTransitionIndex) const;
    bool tryResolveTransitionEndpoints(ax::NodeEditor::PinId firstPinId, ax::NodeEditor::PinId secondPinId,int& outSourceStateIndex, int& outTargetStateIndex) const;
    bool hasTransitionBetweenStates(const std::string& sourceStateName, const std::string& targetStateName) const;

    bool deleteStateAndReferences(int stateIndex);
    void resetGraphEditorStateAfterStructuralChange(bool clearSavedLayout);

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
    int m_contextTransitionIndex = -1;
    int m_contextStateIndex = -1;
    int m_pendingNewStatePlacementIndex = -1;
    ImVec2 m_pendingNewStatePosition = ImVec2(40.0f, 40.0f);
    bool m_pendingGraphEditorReset = false;
    bool m_pendingClearSavedLayout = false;
    bool m_hasSaveFeedback = false;
    bool m_lastSaveSucceeded = false;
};
