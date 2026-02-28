#pragma once
#include "Component.h"
#include <UIImage.h>
#include <PointerEventData.h>
#include "IPointerEventHandler.h"
#include "Delegates.h"

class UIButton : public Component, public IPointerEventHandler {
public:
	UIButton(UID id, GameObject* owner);

	DECLARE_MULTICAST_DELEGATE(OnClick);
	OnClick onClick;

	UIImage*	getTargetGraphic() const { return m_targetGraphic; }
	void        setTargetGraphic(UIImage* img) { m_targetGraphic = img; }

	void onPointerUp(PointerEventData& data) override;
	void onPointerClick(PointerEventData& data) override;

	void press();

	void drawUi() override;
private:
	UIImage*		m_targetGraphic;
	bool			m_isPressed = false;
	//Event
};