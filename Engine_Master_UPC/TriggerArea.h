#pragma once
#include "Component.h"


class TriggerArea final : public Component {

public:
	TriggerArea(UID id, GameObject* gameObject);

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;

	void update() override;

	void onTransformChange() override {}
	void onChangeScene();

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	virtual bool deserializeJSON(const rapidjson::Value& componentInfo) override;
private:

	float xWidth, zWidth;

	std::string m_sceneToLoad;
	UID m_object1;
	UID m_object2;

};

