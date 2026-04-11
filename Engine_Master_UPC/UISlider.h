#pragma once
#include "Component.h"
#include "UIImage.h"
#include "UIFill.h"

class UISlider : public Component
{
public:
    UISlider(UID id, GameObject* owner);

    std::unique_ptr<Component> clone(GameObject* newOwner) const override;

    UIImage* getTargetGraphic() const { return m_targetGraphic; }
    void setTargetGraphic(UIImage* img);
    void setTargetImage(UIImage* img) { setTargetGraphic(img); }

    float getFillAmount() const { return m_fillAmount; }
    void setFillAmount(float amount);

    FillMethod getFillMethod() const { return m_fillMethod; }
    void setFillMethod(FillMethod method);

    FillOrigin getFillOrigin() const { return m_fillOrigin; }
    void setFillOrigin(FillOrigin origin);

    void drawUi() override;

    rapidjson::Value getJSON(rapidjson::Document& domTree) override;
    bool deserializeJSON(const rapidjson::Value& componentInfo) override;
    void fixReferences(const SceneReferenceResolver& resolver) override;

private:
    void applyToTarget();

private:
    UIImage* m_targetGraphic = nullptr;
    UID m_targetGraphicUid = 0;

    float m_fillAmount = 1.0f;
    FillMethod m_fillMethod = FillMethod::Horizontal;
    FillOrigin m_fillOrigin = FillOrigin::HorizontalLeft;
};
