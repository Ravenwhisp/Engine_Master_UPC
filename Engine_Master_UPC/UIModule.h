#pragma once

#include "Module.h"
#include <unordered_set>
#include "UID.h"
#include "UICommands.h"

class FontPass;
class GameObject;
class UIImagePass;
class Texture;

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
    UIImagePass* m_imagePass = nullptr;

    std::vector<UITextCommand> m_textCommands;
    std::vector<UIImageCommand> m_imageCommands;

    std::unordered_map<std::string, std::unique_ptr<Texture>> m_uiTextures;

private:
    void collectUIRecursive(GameObject* go);

};
