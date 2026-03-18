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

void ModuleUI::preRender()
{
    m_textCommands.clear();
    m_imageCommands.clear();

#ifdef GAME_RELEASE
    auto viewport = app->getModuleD3D12()->getSwapChain()->getViewport();
    const ImVec2 screenSize(viewport.Width, viewport.Height);
#else
    const ImVec2 screenSize = app->getModuleEditor()->getWindowSceneEditorSize();
#endif

    if (screenSize.x <= 0.0f || screenSize.y <= 0.0f)
        return;

    m_rootScreenRect = { 0.0f, 0.0f, screenSize.x, screenSize.y };

    for (GameObject* go : app->getModuleScene()->getScene()->getRootObjects())
    {
        if (!go || !go->GetActive())
            continue;

        Canvas* canvas = go->GetComponentAs<Canvas>(ComponentType::CANVAS);
        if (!canvas || !canvas->isActive())
            continue;

        buildUIDrawCommands(go, m_rootScreenRect);
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

void ModuleUI::buildUIDrawCommands(GameObject* gameObject, const Rect2D& parentRect) 
{
    if (!gameObject || !gameObject->GetActive())
    {
        return;
    }

    Transform2D* t2d = gameObject->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D);
    
    Rect2D myRect = parentRect;

    if (t2d && t2d->isActive())
    {
        myRect = t2d->getRect(parentRect);

        buildUIImage(gameObject, myRect);
        buildUIText(gameObject, myRect);
    }

    Transform* transform = gameObject->GetTransform();

    for (GameObject* child : transform->getAllChildren())
    {
        buildUIDrawCommands(child, myRect);
    }
}

void ModuleUI::buildUIImage(GameObject* gameObject, const Rect2D& myRect)
{
    UIImage* uiImg = gameObject->GetComponentAs<UIImage>(ComponentType::UIIMAGE);

    if (!uiImg || !uiImg->isActive())
    {
        return;
    }

    if (uiImg->consumeLoadRequest())
    {
        TextureAsset* asset = uiImg->getTextureAsset();
        UID assetId = uiImg->getTextureAssetId();

        if (!asset || assetId == 0)
        {
            uiImg->setTexture(nullptr);
        }
        else
        {
            auto textureIteration = m_uiTextures.find(assetId);
            if (textureIteration == m_uiTextures.end())
            {
                auto texture = app->getModuleResources()->createTexture2D(*asset);
                if (texture)
                {
                    Texture* raw = texture.get();
                    m_uiTextures.emplace(assetId, std::move(texture));
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