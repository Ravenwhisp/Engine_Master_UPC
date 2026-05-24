#pragma once
#include "EditorWindow.h"

class ModuleMusic;

class WindowMusicDebug : public EditorWindow
{
private:
    ModuleMusic* m_moduleMusic;

public:
    WindowMusicDebug();

    const char* getWindowName() const override
    {
        return "Music Library";
    }

    void drawInternal() override;
};

