#pragma once
#include "Component.h"
#include <string>

class UIImage : public Component
{
public:
    UIImage(UID id, GameObject* owner);

    void setPath(const std::string& path) { m_path = path; }
    const std::string& getPath() const { return m_path; }

    void requestLoad() { m_loadRequested = true; }              
    bool consumeLoadRequest();

    class Texture* getTexture() const { return m_texture; }
    void setTexture(Texture* texture) { m_texture = texture; }

    bool hasTexture() const { return m_texture != nullptr; }

    void drawUi() override;

private:
    std::string m_path = "Assets/Textures/Klinklang.png";
    Texture* m_texture = nullptr;
    bool m_loadRequested = false;
};
