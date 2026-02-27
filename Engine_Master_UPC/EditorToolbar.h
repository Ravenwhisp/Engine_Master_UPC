#pragma once

class EditorModule;

class EditorToolbar 
{
private:
    EditorModule* m_moduleEditor;

    float m_buttonWidth = 100.0f;
    float m_buttonHeight = 28.0f;

public:
    EditorToolbar();
    ~EditorToolbar();

    void DrawCentered(float menuWidth);

private:
    void ManageNavigationButton(int selectedIndex, int selectedNavIndex);
    void ManagePositionButton(int selectedIndex);
    void CreateButton(int selectedIndex, const char* text, int index);
    void DrawGizmoModeButton(float menuWidth, float startX);
};
