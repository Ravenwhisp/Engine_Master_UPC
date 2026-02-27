#include "Globals.h"
#include "UIModule.h"

#include "Application.h"
#include "D3D12Module.h"
#include "FontPass.h"
#include "Logger.h"

bool UIModule::init()
{
    auto device = app->getD3D12Module()->getDevice();
    m_fontPass = std::make_unique<FontPass>(device);
    return true;
}

void UIModule::preRender()
{
    m_textCommands.clear();

    // Remove later, just for test now.
    text(L"UI MODULE OK", 20.0f, 20.0f);
}

void UIModule::renderUI(ID3D12GraphicsCommandList4* commandList, D3D12_VIEWPORT viewport)
{
    if (!m_fontPass) return;

    m_fontPass->setViewport(viewport);

    m_fontPass->begin(commandList);

    for (const auto& cmd : m_textCommands)
    {
        m_fontPass->drawText(cmd.text.c_str(), cmd.x, cmd.y);
    }

    m_fontPass->end();
}

void UIModule::text(const wchar_t* msg, float x, float y)
{
    if (!msg) return;
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
    m_fontPass.reset();
    return true;
}