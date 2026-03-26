#pragma once
#include "EditorWindow.h"

class GameObject;

class WindowInspector : public EditorWindow
{

public:

    WindowInspector();

    const char* getWindowName() const override
    {
        return "WindowInspector";
    }

    void drawInternal() override;
};
