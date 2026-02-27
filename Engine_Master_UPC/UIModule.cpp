#include "Globals.h"
#include "UIModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "FontPass.h"
#include "Logger.h"

#include "SceneModule.h"
#include "GameObject.h"
#include "Transform.h"
#include "Canvas.h"

bool UIModule::init()
{
    auto device = app->getD3D12Module()->getDevice();
    m_fontPass = new FontPass(device);
    return true;
}

void UIModule::preRender()
{
    m_textCommands.clear();

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

        if (m_loggedCanvases.insert(go->GetID()).second)
        {
            logCanvasTree(go);
        }
    }
}

void UIModule::renderUI(ID3D12GraphicsCommandList4* commandList, D3D12_VIEWPORT viewport)
{
    if (!m_fontPass)
    {
        return;
    }

    m_fontPass->setViewport(viewport);

    m_fontPass->setTextCommands(&m_textCommands);

    m_fontPass->apply(commandList);

    m_fontPass->setTextCommands(nullptr);
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

    delete m_fontPass;     
    m_fontPass = nullptr;

    return true;
}

void UIModule::logCanvasTree(GameObject* canvasGO)
{
    DEBUG_LOG("Canvas found: %s (UID: %llu)",
        canvasGO->GetName().c_str(),
        (unsigned long long)canvasGO->GetID());

    logChildrenRecursive(canvasGO, 0);
}

void UIModule::logChildrenRecursive(GameObject* go, int depth)
{
    if (!go) return;

    std::string indent(depth * 2, ' ');
    DEBUG_LOG("%s- %s (UID: %llu)",
        indent.c_str(),
        go->GetName().c_str(),
        (unsigned long long)go->GetID());

    Transform* t = go->GetTransform();
    if (!t) return;

    for (GameObject* child : t->getAllChildren())
    {
        if (child && child->GetActive())
            logChildrenRecursive(child, depth + 1);
    }
}