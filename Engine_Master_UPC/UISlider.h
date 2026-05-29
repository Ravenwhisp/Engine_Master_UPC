#pragma once
#include "Component.h"
#include "UIFill.h"

class UIImage;

class UISlider : public Component
{
public:
    UISlider(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    float getFillAmount() const { return m_fillAmount; }
    void setFillAmount(float amount);

    FillMethod getFillMethod() const { return m_fillMethod; }
    void setFillMethod(FillMethod method);

    FillOrigin getFillOrigin() const { return m_fillOrigin; }
    void setFillOrigin(FillOrigin origin);

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;

private:
    void applyToImage();

private:
    float m_fillAmount = 1.0f;
    FillMethod m_fillMethod = FillMethod::Horizontal;
    FillOrigin m_fillOrigin = FillOrigin::HorizontalLeft;
};
