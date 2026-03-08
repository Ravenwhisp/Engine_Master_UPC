#pragma once
#include "Component.h"
#include <UIImage.h>
#include <PointerEventData.h>
#include "IPointerEventHandler.h"
#include "Delegates.h"

class UIButton : public Component, public IPointerEventHandler 
{
public:
	UIButton(UID id, GameObject* owner);

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	DECLARE_MULTICAST_DELEGATE(OnClick);
	OnClick onClick;

	UIImage* getTargetGraphic() const { return m_targetGraphic; }
	void setTargetGraphic(UIImage* img) { m_targetGraphic = img; m_targetGraphicUid = img ? img->getID() : 0; }

	void onPointerUp(PointerEventData& data) override;
	void onPointerClick(PointerEventData& data) override;

	void press();

	void drawUi() override;

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;
	void fixReferences(const std::unordered_map<UID, Component*>& referenceMap) override;

private:
	UIImage*		m_targetGraphic = nullptr;
	UID				m_targetGraphicUid = 0;
	bool			m_isPressed = false;

	//Event
};