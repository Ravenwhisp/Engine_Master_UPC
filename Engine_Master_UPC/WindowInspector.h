#pragma once
#include "EditorWindow.h"
#include "UID.h"

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
    void lockInspector(GameObject* go);
    void unlockInspector();
    bool isLocked() const;

private:
    bool m_isLocked = false;
    UID m_lockedGameObjectUID = 0;
};
