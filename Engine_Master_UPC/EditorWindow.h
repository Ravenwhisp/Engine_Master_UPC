#pragma once
class EditorWindow
{
public:
    EditorWindow(){}
    virtual ~EditorWindow() = default;

    virtual const char* GetWindowName() const = 0;
    virtual void Update() { }
    virtual void Render() = 0;
    virtual void CleanUp() { }

    bool IsOpen() const { return m_IsOpen; }
    void SetOpen(bool open) { m_IsOpen = open; }
    bool* GetOpenPtr() { return &m_IsOpen; }

    virtual ImGuiWindowFlags GetWindowFlags() const { return ImGuiWindowFlags_None; }
    virtual bool IsDockable() const { return true; }

    //TODO: Changes names for a intuitive API
    ImVec2 GetSize() const { return m_Size; }
    void SetSize(const ImVec2& size) { m_Size = size; }
    float GetWindowX() const { return windowX; }
    float GetWindowY() const { return windowY; }


    bool IsHovered() { return _isViewportHovered; }
    bool IsFocused() { return _isViewportFocused; }
protected:
    float windowX, windowY = 0;
    bool m_IsOpen = true;
    ImVec2 m_Size = { 400, 300 };
    bool _isViewportHovered = false;
    bool _isViewportFocused = false;
};

