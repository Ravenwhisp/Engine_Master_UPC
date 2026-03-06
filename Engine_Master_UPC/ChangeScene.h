#pragma once
#include "Component.h"
#include "Delegates.h"

class UIButton;

class ChangeScene final : public Component {

public:
	ChangeScene(UID id, GameObject* gameObject);
	~ChangeScene();

	bool init() override;
	void drawUi() override;

	void onTransformChange() override {}
	void onChangeScene();

	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	virtual bool deserializeJSON(const rapidjson::Value& componentValue) override;
private:

	std::string m_sceneToLoad;
	UIButton* m_uiButton = nullptr;
	DelegateHandle m_onClickHandle;
};