#pragma once

#include "Module.h"
#include "UICommands.h"
#include "UID.h"

class FontPass;
class GameObject;
class UIImagePass;
class Texture;
class Transform2D;
struct AssetId;

class ModuleUI : public Module
{
public:
    void preRender() override;
    bool cleanUp() override;

    void buildCommandsForViewport(float width, float height);

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

    std::unordered_map<UID, std::shared_ptr<Texture>> m_uiTextures;

private:
void buildUIDrawCommands(GameObject* go, const Rect2D& parentRect, CanvasRenderMode renderMode, const Matrix& canvasWorld, bool zTest,
        const Vector2& inheritedScale, float parentAlpha = 1.0f);

void buildUIImage(GameObject* go, const Rect2D& parentRect, CanvasRenderMode renderMode, const Matrix& canvasWorld, bool zTest, float alpha);
void buildUIText(GameObject* go, const Rect2D& parentRect);
};
