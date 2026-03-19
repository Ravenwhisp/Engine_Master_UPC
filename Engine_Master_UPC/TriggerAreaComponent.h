#pragma once
#include "Component.h"


class TriggerAreaComponent final : public Component {

public:
	TriggerAreaComponent(UID id, GameObject* gameObject);

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;

	void update() override;

	//void preRender() override;

	void onTransformChange() override {}
	void onChangeScene();
	void printArea();

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	virtual bool deserializeJSON(const rapidjson::Value& componentInfo) override;
private:

	float m_xWidth, m_zWidth;

	std::string m_sceneToLoad;
	UID m_object1;
	UID m_object2;

};

