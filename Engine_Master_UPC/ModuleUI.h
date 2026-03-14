#pragma once

#include "Module.h"
#include <unordered_set>
#include "MD5.h"
#include "UICommands.h"

class FontPass;
class GameObject;
class UIImagePass;
class Texture;
class Transform2D;

class ModuleUI : public Module
{
public:
    bool init() override;
    void preRender() override;
    bool cleanUp() override;

    void renderUI(ID3D12GraphicsCommandList4* commandList, D3D12_VIEWPORT viewport);

    void text(const wchar_t* msg, float x, float y);
    void text(const std::wstring& msg, float x, float y);

private:
    Rect2D m_rootScreenRect;

    FontPass* m_fontPass = nullptr;
    UIImagePass* m_imagePass = nullptr;

    std::vector<UITextCommand> m_textCommands;
    std::vector<UIImageCommand> m_imageCommands;

    std::unordered_map<MD5Hash, std::shared_ptr<Texture>> m_uiTextures;

private:
    void buildUIDrawCommands(GameObject* go, const Rect2D& parentRect);

    void buildUIImage(GameObject* go, const Rect2D& parentRect);
    void buildUIText(GameObject* go, const Rect2D& parentRect);

};
