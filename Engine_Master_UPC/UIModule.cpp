#include "Globals.h"
#include "UIModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "FontPass.h"
#include "UIImagePass.h"
#include "Logger.h"

#include "SceneModule.h"
#include "ResourcesModule.h"
#include "EditorModule.h"
#include "Texture.h"

#include "GameObject.h"
#include "Transform.h"
#include "Canvas.h"
#include "UIImage.h"
#include "Transform2D.h"
#include "UIText.h"

bool UIModule::init()
{
    auto device = app->getD3D12Module()->getDevice();
    m_fontPass = new FontPass(device);
    m_imagePass = new UIImagePass(device);
    return true;
}

void UIModule::preRender()
{
    m_textCommands.clear();
    m_imageCommands.clear();

    const ImVec2 screenSize = app->getEditorModule()->getSceneEditorSize();

    if (screenSize.x <= 0.0f || screenSize.y <= 0.0f) {
        return;
    }

    m_rootScreenRect.x = 0.0f;
    m_rootScreenRect.y = 0.0f;
    m_rootScreenRect.w = screenSize.x;
    m_rootScreenRect.h = screenSize.y;

    const auto& roots = app->getSceneModule()->getAllGameObjects();

    for (GameObject* go : roots)
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

        buildUIDrawCommands(go, m_rootScreenRect);
    }
}

void UIModule::renderUI(ID3D12GraphicsCommandList4* commandList, D3D12_VIEWPORT viewport)
{
    if (m_imagePass)
    {
        m_imagePass->setViewport(viewport);
        m_imagePass->setImageCommands(&m_imageCommands);
        m_imagePass->apply(commandList);
        m_imagePass->setImageCommands(nullptr);
    }

    if (m_fontPass)
    {
        m_fontPass->setViewport(viewport);
        m_fontPass->setTextCommands(&m_textCommands);
        m_fontPass->apply(commandList);
        m_fontPass->setTextCommands(nullptr);
    }

}

void UIModule::text(const wchar_t* msg, float x, float y)
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

void UIModule::text(const std::wstring& msg, float x, float y)
{
    UITextCommand cmd;
    cmd.text = msg;
    cmd.x = x;
    cmd.y = y;
    m_textCommands.push_back(std::move(cmd));
}

bool UIModule::cleanUp()
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

void UIModule::buildUIDrawCommands(GameObject* gameObject, const Rect2D& parentRect) 
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

void UIModule::buildUIImage(GameObject* gameObject, const Rect2D& myRect)
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
                auto texture = app->getResourcesModule()->createTexture2D(*asset);
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

void UIModule::buildUIText(GameObject* gameObject, const Rect2D& myRect) 
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