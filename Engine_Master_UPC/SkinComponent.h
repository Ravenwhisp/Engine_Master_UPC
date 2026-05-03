#pragma once

#include "Component.h"

class SkinComponent final : public Component
{
public:
    SkinComponent(UID id, GameObject* owner);
    ~SkinComponent() override = default;

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool init() override;
    void lateUpdate() override;
    bool cleanUp() override;

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;
};