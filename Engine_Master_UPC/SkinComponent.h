#pragma once

#include "Component.h"
#include "MD5Fwd.h"

class SkinComponent final : public Component
{
public:
    SkinComponent(UID id, GameObject* owner);
    ~SkinComponent() override = default;

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool init() override;
    void lateUpdate() override;
    bool cleanUp() override;

    void setSkinReference(const MD5Hash& skinUID);
    const MD5Hash& getSkinReference() const { return m_skinAsset; }

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

private:
    MD5Hash m_skinAsset = INVALID_ASSET_ID;
};