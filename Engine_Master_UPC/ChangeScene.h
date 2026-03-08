#pragma once
#include "Component.h"
#include "Delegates.h"

class UIButton;

class ChangeScene final : public Component 
{

public:
	ChangeScene(UID id, GameObject* gameObject);
	~ChangeScene();

	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	bool init() override;
	void drawUi() override;

	void onTransformChange() override {}
	void onChangeScene();

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	virtual bool deserializeJSON(const rapidjson::Value& componentValue) override;
	void fixReferences(const std::unordered_map<UID, Component*>& referenceMap) override;

private:

	std::string m_sceneToLoad;
	UIButton* m_uiButton = nullptr;
	UID m_uiButtonUid = 0;
	DelegateHandle m_onClickHandle;
};