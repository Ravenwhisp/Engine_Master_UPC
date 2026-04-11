#pragma once

#include "Component.h"
#include "IPointerEventHandler.h"

#include <vector>
#include <string>

#include "Delegates.h"
#include "ScriptMethodInfo.h"
#include "SimpleMath.h"

using Vector3 = DirectX::SimpleMath::Vector3;

class UIImage;
class ScriptComponent;
class Script;
class PointerEventData;
class SceneReferenceResolver;

class UIButton : public Component, public IPointerEventHandler
{
private:
	struct ButtonEventBinding
	{
		UID gameObjectUid = 0;
		UID componentUid = 0;

		std::string methodName;

		ScriptComponent* component = nullptr;

		using MethodPtr = void(*)(Script*);
		MethodPtr function = nullptr;
		ScriptMethodParamFunc paramFunc = nullptr;
		ScriptMethodParamType paramType = ScriptMethodParamType::None;
		std::string paramName;
		float paramFloat = 0.0f;
		int paramInt = 0;
		bool paramBool = false;
		Vector3 paramVec3 = Vector3(0.0f, 0.0f, 0.0f);
		std::string paramString;
	};

public:
	UIButton(UID id, GameObject* owner);
	std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	DECLARE_MULTICAST_DELEGATE(OnClick);
	OnClick onClick;

#pragma region UI API
	UIImage* getTargetGraphic() const { return m_targetGraphic; }
	void setTargetGraphic(UIImage* img);
#pragma endregion

#pragma region Events
	void onPointerEnter(PointerEventData& data) override;
	void onPointerExit(PointerEventData& data) override;
	void onPointerDown(PointerEventData& data) override;
	void onPointerUp(PointerEventData& data) override;
	void onPointerClick(PointerEventData& data) override;

	void press();
	void executeBindings(std::vector<ButtonEventBinding>& bindings);
#pragma endregion

#pragma region Editor
	void drawUi() override;
	void drawBindingsUI(const char* label, std::vector<ButtonEventBinding>& bindings);
#pragma endregion

#pragma region Serialization
	void SerializeBindings(const std::vector<UIButton::ButtonEventBinding>& bindings, rapidjson::Value& array, rapidjson::Document& doc);
	rapidjson::Value getJSON(rapidjson::Document& domTree) override;
	void DeserializeBindings(const rapidjson::Value& array, std::vector<UIButton::ButtonEventBinding>& outBindings);
	bool deserializeJSON(const rapidjson::Value& componentInfo) override;
	void ResolveBinding(UIButton::ButtonEventBinding& b, const SceneReferenceResolver& resolver);
	void fixReferences(const SceneReferenceResolver& resolver) override;
#pragma endregion

private:

#pragma region Data
	UIImage* m_targetGraphic = nullptr;
	UID m_targetGraphicUid = 0;

	bool m_isPressed = false;
#pragma endregion

#pragma region EventBindings
	std::vector<ButtonEventBinding> m_bindingsOnHover;
	std::vector<ButtonEventBinding> m_bindingsOnPress;
	std::vector<ButtonEventBinding> m_bindingsOnRelease;
#pragma endregion
};