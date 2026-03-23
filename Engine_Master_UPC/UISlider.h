#pragma once
#include "Component.h"
#include "UIImage.h"

enum class FillMethod
{
    Horizontal,
    Vertical,
    Radial90,
    Radial180,
    Radial360
};

class UISlider : public Component
{
public:
    UISlider(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    UIImage* getTargetGraphic() const { return m_targetGraphic; }
    void setTargetGraphic(UIImage* img);

    float getFillAmount() const { return m_fillAmount; }
    void setFillAmount(float amount) { m_fillAmount = amount; }

    FillMethod getFillMethod() const { return m_fillMethod; }
    void setFillMethod(FillMethod method) { m_fillMethod = method; }

    bool getClockwise() const { return m_clockwise; }
    void setClockwise(bool clockwise) { m_clockwise = clockwise; }

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;
    void fixReferences(const SceneReferenceResolver& resolver) override;

private:
    UIImage* m_targetGraphic = nullptr;
    UID m_targetGraphicUid = 0;

    float m_fillAmount = 1.0f;
    FillMethod m_fillMethod = FillMethod::Horizontal;
    bool m_clockwise = true;
};
