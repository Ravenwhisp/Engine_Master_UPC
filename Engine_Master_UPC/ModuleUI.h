#pragma once

#include "Module.h"
#include <unordered_map>
#include "UICommands.h"
#include "MD5Fwd.h"

class FontPass;
class GameObject;
class UIImagePass;
class Texture;
class Transform2D;

class ModuleUI : public Module
{
public:
    void preRender() override;
    bool cleanUp() override;

    const std::vector<UITextCommand>& getTextCommands()  const { return m_textCommands; }
    const std::vector<UIImageCommand>& getImageCommands() const { return m_imageCommands; }

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
