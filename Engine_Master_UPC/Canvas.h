#pragma once
#include "Component.h"

class Canvas : public Component
{
public:
    Canvas(UID id, GameObject* owner);

    bool isScreenSpace = true;

    void drawUi() override;

private:
};