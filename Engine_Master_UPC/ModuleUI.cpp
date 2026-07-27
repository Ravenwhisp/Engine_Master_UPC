#include "Globals.h"
#include "ModuleUI.h"

#include "Application.h"
#include "ModuleD3D12.h"
#include "FontPass.h"
#include "UIImagePass.h"
#include "WindowLogger.h"

#include "ModuleScene.h"
#include "ModuleResources.h"
#include "ModuleEditor.h"
#include "Texture.h"

#include "Scene.h"
#include "GameObject.h"
#include "Transform.h"
#include "Canvas.h"
#include "UIImage.h"
#include "Transform2D.h"
#include "UIText.h"
#include <unordered_map>
#include "WindowSceneEditor.h"
#include "AssetReference.h"

#include "UILayoutUtils.h"

void ModuleUI::preRender()
{
    m_textCommands.clear();
    m_imageCommands.clear();
}

void ModuleUI::buildCommandsForViewport(float width, float height)
{
    m_textCommands.clear();
    m_imageCommands.clear();

    if (width <= 0.0f || height <= 0.0f)
    {
        return;
    }

    m_rootScreenRect = { 0.0f, 0.0f, width, height };

    for (GameObject* go : app->getModuleScene()->getScene()->getAllGameObjects())
    {
        if (!go || !go->GetActive())
        {
            continue;
        }

        Canvas* canvas = go->GetComponentAs<Canvas>(ComponentType::CANVAS);
        if (!canvas || !canvas->isActive())
        {
            continue;
        }

        const bool isScreenSpace = canvas->renderMode == CanvasRenderMode::SCREEN_SPACE;

        Vector2 uiScale(1.0f, 1.0f);
        Rect2D rootRect = m_rootScreenRect;

        if (isScreenSpace)
        {
            uiScale = UILayoutUtils::CalculateScreenSpaceScale(width, height);
        }
        else
        {
            rootRect = { -0.5f, -0.5f, 1.0f, 1.0f };
        }

        if (Transform2D* canvasTransform = go->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D))
        {
            if (canvasTransform->isActive())
            {
                rootRect = canvasTransform->getRect(rootRect, { 1.0f, 1.0f });
            }
        }

        buildUIDrawCommands(go, rootRect, canvas->renderMode, go->GetTransform()->getGlobalMatrix(), canvas->zTest, uiScale);
    }
}


void ModuleUI::text(const wchar_t* msg, float x, float y)
{
    if (!msg)
    {
        return;
    }
    UITextCommand cmd;
    cmd.text = msg;
    cmd.x = x;
    cmd.y = y;
    m_textCommands.push_back(std::move(cmd));
}

void ModuleUI::text(const std::wstring& msg, float x, float y)
{
    UITextCommand cmd;
    cmd.text = msg;
    cmd.x = x;
    cmd.y = y;
    m_textCommands.push_back(std::move(cmd));
}

bool ModuleUI::cleanUp()
{
    m_textCommands.clear();
    m_imageCommands.clear();

    m_uiTextures.clear();

    delete m_fontPass;     
    m_fontPass = nullptr;

    delete m_imagePass;
    m_imagePass = nullptr;

    return true;
}

static std::wstring stringToWString(const std::string& string)
{
    if (string.empty())
    {
        return L"";
    }
    int len = MultiByteToWideChar(CP_UTF8, 0, string.c_str(), (int)string.size(), nullptr, 0);
    if (len <= 0)
    {
        return L"";
    }
    std::wstring wstring;
    wstring.resize(len);
    MultiByteToWideChar(CP_UTF8, 0, string.c_str(), (int)string.size(), wstring.data(), len);
    return wstring;
}

void ModuleUI::buildUIDrawCommands(GameObject* gameObject, const Rect2D& parentRect, CanvasRenderMode renderMode, const Matrix& canvasWorld, bool zTest,
    const Vector2& inheritedScale, float parentAlpha)
{
    if (!gameObject || !gameObject->GetActive())
    {
        return;
    }

    Transform2D* t2d = gameObject->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D);

    Rect2D myRect = parentRect;
    Vector2 scale = inheritedScale;
    float alpha = parentAlpha;

    if (t2d && t2d->isActive())
    {
        myRect = t2d->getRect(parentRect, scale);
        scale = { t2d->scale.x * inheritedScale.x, t2d->scale.y * inheritedScale.y };
        alpha = t2d->getInheritedAlpha(parentAlpha);

        buildUIImage(gameObject, myRect, renderMode, canvasWorld, zTest, alpha);
        buildUIText(gameObject, myRect);
    }

    Transform* transform = gameObject->GetTransform();

    for (GameObject* child : transform->getAllChildren())
    {
        buildUIDrawCommands(child, myRect, renderMode, canvasWorld, zTest, scale, alpha);
    }
}

void ModuleUI::buildUIImage(GameObject* gameObject, const Rect2D& myRect, CanvasRenderMode renderMode, const Matrix& canvasWorld, bool zTest, float alpha)
{
    UIImage* uiImg = gameObject->GetComponentAs<UIImage>(ComponentType::UIIMAGE);
    Transform2D* t2d = gameObject->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D);

    if (!uiImg || !uiImg->isActive())
    {
        return;
    }

        if (uiImg->consumeLoadRequest())
        {
            TextureAsset* asset = uiImg->getTextureAsset();
            const AssetId& assetId = uiImg->getTextureAssetId();

            if (!asset || !assetId.isValid())
            {
                uiImg->setTexture(nullptr);
            }
            else
            {
                auto textureIteration = m_uiTextures.find(assetId.m_uid);
                if (textureIteration == m_uiTextures.end())
                {
                    auto texture = app->getModuleResources()->createTexture(*asset, true);
                    if (texture)
                    {
                        Texture* raw = texture.get();
                        m_uiTextures.emplace(assetId.m_uid, std::move(texture));
                        uiImg->setTexture(raw);
                    }
                    else
                    {
                        uiImg->setTexture(nullptr);
                    }
                }
                else
                {
                    uiImg->setTexture(textureIteration->second.get());
                }
            }
        }

    if (uiImg->getTexture() != nullptr)
    {
        UIImageCommand command;
        command.texture = uiImg->getTexture();
        command.rect = myRect;
        command.alpha = alpha;
        command.fillAmount = uiImg->getFillAmount();
        command.fillMethod = uiImg->getFillMethod();
        command.fillOrigin = uiImg->getFillOrigin();

        command.uvScale = { 1.0f, 1.0f };
        command.uvScale.x /= uiImg->getSheetColumns();
        command.uvScale.y /= uiImg->getSheetRows();
        command.sheetOffset = Vector2(0.5f, 0.5f) * command.uvScale + uiImg->getSheetOffset();

        const bool hasSheet = uiImg->getSheetColumns() > 1 || uiImg->getSheetRows() > 1;
        const UIImage::StretchDrawMode drawMode = uiImg->getStretchDrawMode();

        if (drawMode == UIImage::StretchDrawMode::Cover && !hasSheet)
        {
            TextureAsset* textureAsset = uiImg->getTextureAsset();

            if (textureAsset)
            {
                const float textureWidth = static_cast<float>(textureAsset->getWidth());
                const float textureHeight = static_cast<float>(textureAsset->getHeight());

                if (textureWidth > 0.0f && textureHeight > 0.0f && myRect.w > 0.0f && myRect.h > 0.0f)
                {
                    const float textureAspect = textureWidth / textureHeight;
                    const float rectAspect = myRect.w / myRect.h;

                    if (rectAspect > textureAspect)
                    {
                        // if the rectAspect is wider than the texture, fill width and crop top/bottom.
                        const float visibleV = textureAspect / rectAspect;

                        command.uvScale = { 1.0f, visibleV };
                        command.sheetOffset = { 0.5f, 0.5f };
                    }
                    else
                    {
                        // int he other hand, if the rect is taller/narrower than the texture, fill height and crop left/right.
                        const float visibleU = rectAspect / textureAspect;

                        command.uvScale = { visibleU, 1.0f };
                        command.sheetOffset = { 0.5f, 0.5f };
                    }
                }
            }
        }
        else if (drawMode == UIImage::StretchDrawMode::Tile && !hasSheet)
        {
            if (t2d->getStretchMode() == StretchMode::HORIZONTAL)
            {
                command.uvScale.y *= t2d->getScale().y;
            }
            else if (t2d->getStretchMode() == StretchMode::VERTICAL)
            {
                command.uvScale.x *= t2d->getScale().x;
            }
            else if (t2d->getStretchMode() == StretchMode::BOTH)
            {
                command.uvScale *= Vector2(myRect.w / t2d->getBaseSize().x, myRect.h / t2d->getBaseSize().y);
            }
            else
            {
                command.uvScale *= t2d->getScale();
            }

            command.sheetOffset = Vector2(0.5f, 0.5f);
        }

        command.renderMode = renderMode;
        command.world = (renderMode == CanvasRenderMode::SCREEN_SPACE)
            ? Matrix::Identity
            : gameObject->GetTransform()->getGlobalMatrix();
        command.zTest = zTest;
        m_imageCommands.push_back(command);
    }
}

void ModuleUI::buildUIText(GameObject* gameObject, const Rect2D& myRect) 
{
    UIText* uiText = gameObject->GetComponentAs<UIText>(ComponentType::UITEXT);

    if (!uiText || !uiText->isActive())
    {
        return;
    }

    UITextCommand command;
    command.text = stringToWString(uiText->getText());
    command.x = myRect.x;
    command.y = myRect.y;
    command.color = uiText->getColor();
    command.scale = uiText->getFontScale();

    m_textCommands.push_back(std::move(command));
}