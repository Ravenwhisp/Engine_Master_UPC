#pragma once
#include "Component.h"
#include "IPointerEventHandler.h"

#include "SimpleMath.h"
using DirectX::SimpleMath::Vector2;

#include "UIFill.h"

class UIImage;
class Transform2D;

class UISlider : public Component, public IPointerEventHandler
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

#pragma region Events
    void onPointerEnter(PointerEventData& data) override;
    void onPointerExit(PointerEventData& data) override;
    void onPointerDown(PointerEventData& data) override;
    void onPointerDrag(PointerEventData& data) override;
    void onPointerUp(PointerEventData& data) override;
    void onPointerClick(PointerEventData& data) override;

	void updateFillAmountFromPointerPosition(const Vector2& mousePos);
#pragma endregion

    FillMethod getFillMethod() const { return m_fillMethod; }
    void setFillMethod(FillMethod method);

    FillOrigin getFillOrigin() const { return m_fillOrigin; }
    void setFillOrigin(FillOrigin origin);

    void drawUi() override;

    void serialize(IArchive& archive) override;

    void fixReferences(const SceneReferenceResolver& resolver) override;

private:
    void applyToImage();

	void updateThumbPosition();

private:
    Vector2 m_fillAmount = Vector2(0.0f, 1.0f);
    FillMethod m_fillMethod = FillMethod::Horizontal;
    FillOrigin m_fillOrigin = FillOrigin::HorizontalLeft;

    Transform2D* m_thumbTransform = nullptr;
    UID m_thumbComponentUid = 0;
};
