#pragma once
#include "Component.h"
#include "Delegates.h"

class UIButton;

class ExitApplication final : public Component
{
public:
    ExitApplication(UID id, GameObject* gameObject);
    ~ExitApplication();

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    bool init() override;
    void drawUi() override;

    void onTransformChange() override {}
    void onExitApplication();

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentValue) override;

private:
    UIButton* m_uiButton = nullptr;
    DelegateHandle m_onClickHandle;
};