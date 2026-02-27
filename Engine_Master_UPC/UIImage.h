#pragma once
#include "Component.h"
#include <string>

class UIImage : public Component
{
public:
    UIImage(UID id, GameObject* owner);

    void setPath(const std::string& p) { m_path = p; m_dirty = true; }
    const std::string& getPath() const { return m_path; }

    class Texture* getTexture() const { return m_texture; }
    void setTexture(Texture* tex) { m_texture = tex; m_dirty = false; }

    bool isDirty() const { return m_dirty; }

    void drawUi() override;

private:
    std::string m_path = "Assets/Textures/KlinKlang.png";
    Texture* m_texture = nullptr;
    bool m_dirty = true;
};
