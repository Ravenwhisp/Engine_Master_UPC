#pragma once

#include "Module.h"

class FontPass;

struct UITextCommand
{
    std::wstring text;
    float x = 0.0f;
    float y = 0.0f;
};

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
    std::vector<UITextCommand> m_textCommands;
};
