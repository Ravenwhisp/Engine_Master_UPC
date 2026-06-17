#pragma once
#include "Component.h"
#include "CanvasRenderMode.h"

class Canvas : public Component
{
public:
    Canvas(UID id, GameObject* owner);
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    CanvasRenderMode renderMode = CanvasRenderMode::SCREEN_SPACE;
    bool zTest = false;

    void drawUi() override;
    void serialize(IArchive& archive) override;

private:
};