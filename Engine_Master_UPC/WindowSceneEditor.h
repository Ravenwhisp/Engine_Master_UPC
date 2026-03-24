#pragma once
#include "EditorWindow.h"
#include "IDebugDrawable.h"
#include "Delegates.h"

class ModuleInput;
class ModuleCamera;

class Settings;
class Quadtree;

class EditorToolbar;
class PlayToolbar;

class DebugDrawPass;
class GameObject;
class RenderSurface;


DECLARE_EVENT(OnResize, WindowSceneEditor);


class WindowSceneEditor: public EditorWindow, public IDebugDrawable
{
private:
    std::unique_ptr<RenderSurface> m_surface;

    ModuleCamera* m_moduleCamera;

    Settings* m_settings;

    EditorToolbar* m_editorToolbar;
	PlayToolbar* m_playToolbar;

    ImVec2 m_viewportPos = ImVec2(0.0f, 0.0f);
    float m_viewportX = 0.0f;
    float m_viewportY = 0.0f;

public:
    WindowSceneEditor();
    ~WindowSceneEditor();

    const char* getWindowName() const override { return "Scene Editor"; }
    void        update() override;
    void        render() override;
    bool        resize(ImVec2 contentRegion);

    void debugDraw() override;

    float  getViewportX()      const { return m_viewportX; }
    float  getViewportY()      const { return m_viewportY; }

};
