#pragma once
#include "Component.h"
#include "UIFill.h"

class UIImage;

class UISlider : public Component
{
public:
    UISlider(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    Vector2 getFillAmount() const { return m_fillAmount; }
    void setFillAmount(const Vector2& amount);
    float getFillStart() const { return m_fillAmount.x; }
    float getFillEnd() const { return m_fillAmount.y; }
    void setFillStart(float start);
    void setFillEnd(float end);

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
    Vector2 m_fillAmount = Vector2(0.0f, 1.0f);
    FillMethod m_fillMethod = FillMethod::Horizontal;
    FillOrigin m_fillOrigin = FillOrigin::HorizontalLeft;
};
