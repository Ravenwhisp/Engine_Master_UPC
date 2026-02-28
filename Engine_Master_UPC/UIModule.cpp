#include "Globals.h"
#include "UIModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "FontPass.h"
#include "UIImagePass.h"
#include "Logger.h"

#include "SceneModule.h"
#include "ResourcesModule.h"
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

    // Remove later, just for test now.
    text(L"UI MODULE :)", 20.0f, 20.0f);

    const auto& roots = app->getSceneModule()->getAllGameObjects();

    for (GameObject* go : roots)
    {
        if (!go || !go->GetActive())
            continue;

        Canvas* canvas = go->GetComponentAs<Canvas>(ComponentType::CANVAS);
        if (!canvas || !canvas->isActive())
            continue;

        collectUIRecursive(go);
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

void UIModule::collectUIRecursive(GameObject* gameObject)
{
    if (!gameObject || !gameObject->GetActive())
        return;

    Transform2D* t2d = gameObject->GetComponentAs<Transform2D>(ComponentType::TRANSFORM2D);
    UIImage* uiImg = gameObject->GetComponentAs<UIImage>(ComponentType::UIIMAGE);

    if (uiImg && uiImg->isActive() && t2d && t2d->isActive())
    {
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
                auto it = m_uiTextures.find(assetId);
                if (it == m_uiTextures.end())
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
                    uiImg->setTexture(it->second.get());
                }
            }
        }

        if (uiImg->getTexture() != nullptr)
        {
            UIImageCommand command;
            command.texture = uiImg->getTexture();
            command.rect = t2d->getRect();
            m_imageCommands.push_back(command);
        }
    }

    UIText* uiText = gameObject->GetComponentAs<UIText>(ComponentType::UITEXT);

    if (uiText && uiText->isActive() && t2d && t2d->isActive())
    {
        UITextCommand cmd;
        cmd.text = stringToWString(uiText->getText());

        Rect2D r = t2d->getRect();
        cmd.x = t2d->position.x;
        cmd.y = t2d->position.y;

        m_textCommands.push_back(std::move(cmd));
    }

    Transform* transform = gameObject->GetTransform();

    for (GameObject* child : transform->getAllChildren())
        collectUIRecursive(child);
}