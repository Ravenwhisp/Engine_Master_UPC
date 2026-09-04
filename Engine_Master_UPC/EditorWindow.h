#pragma once
#include <imgui.h>

class EditorWindow
{

public:

    virtual ~EditorWindow() = default;

    virtual const char* getWindowName() const = 0;
    virtual void cleanUp() { }

    void setInstanceId(int id)
    {
        m_instanceId = id;
        m_imguiId = std::string(getWindowName()) + "##" + std::to_string(id);
    }

    int getInstanceId() const
    {
        return m_instanceId;
    }

    const char* getImGuiId() const
    {
        return m_imguiId.empty() ? getWindowName() : m_imguiId.c_str();
    }

    bool isOpen() const
    {
        return m_isOpen;
    }

    void setOpen(bool open)
    {
        m_isOpen = open;
    }

    bool* getOpenPtr()
    {
        return &m_isOpen;
    }

    virtual ImGuiWindowFlags getWindowFlags() const
    {
        return ImGuiWindowFlags_None;
    }

    virtual bool isDockable() const
    {
        return true;
    }

    ImVec2 getSize() const
    {
        return m_size;
    }

    void setSize(const ImVec2& size)
    {
        m_size = size;
    }

    float getWindowX() const
    {
        return m_windowX;
    }

    float getWindowY() const
    {
        return m_windowY;
    }

    bool isHovered() const
    {
        return m_isViewportHovered;
    }

    bool isFocused() const
    {
        return m_isViewportFocused;
    }

    void draw()
    {
        if (!m_isOpen)
        {
            if (m_isVisible) // was visible last frame, now closed via X
            {
                m_isVisible = false;
                onBecameHidden();
            }
            return;
        }

        bool isVisible = ImGui::Begin(getImGuiId(), &m_isOpen);

        if (isVisible && !m_isVisible) // became visible (tab switched back, or first open)
        {
            m_isVisible = true;
            onBecameVisible();
        }
        else if (!isVisible && m_isVisible) // became hidden (tab switched away, collapsed)
        {
            m_isVisible = false;
            onBecameHidden();
        }

        if (isVisible)
        {
            drawInternal();
        }

        ImGui::End();
    }

protected:
    virtual void onBecameVisible() { }
    virtual void onBecameHidden() { }
    virtual void drawInternal() = 0;

    float m_windowX = 0;
    float m_windowY = 0;
    bool m_isOpen = true;
    bool m_isVisible = false;
    ImVec2 m_size = { 400, 300 };
    bool m_isViewportHovered = false;
    bool m_isViewportFocused = false;

private:
    int         m_instanceId = 0;
    std::string m_imguiId;

};
