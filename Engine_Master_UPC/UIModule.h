#pragma once

#include "Module.h"
#include <unordered_set>
#include "UID.h"
#include "UICommands.h"

class FontPass;
class GameObject;

class UIModule : public Module
{
public:
    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    void renderUI(ID3D12GraphicsCommandList4* commandList, D3D12_VIEWPORT viewport);

    void text(const wchar_t* msg, float x, float y);
    void text(const std::wstring& msg, float x, float y);

private:
    FontPass* m_fontPass = nullptr;
    std::vector<UITextCommand> m_textCommands;

private:
    void logCanvasTree(GameObject* canvasGO);
    void logChildrenRecursive(GameObject* go, int depth);

    std::unordered_set<UID> m_loggedCanvases;
};
