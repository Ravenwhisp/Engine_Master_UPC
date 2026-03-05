#pragma once
#include "Component.h"

class Canvas : public Component
{
public:
    Canvas(UID id, GameObject* owner);
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool isScreenSpace = true;

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
};