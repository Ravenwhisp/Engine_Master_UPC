#pragma once

#include "Component.h"
#include "IPointerEventHandler.h"

#include <vector>
#include <string>

#include "Delegates.h"
#include "ScriptMethodInfo.h"
#include "SimpleMath.h"

#include "AssetReference.h"

using Vector3 = DirectX::SimpleMath::Vector3;

class UIImage;
class ScriptComponent;
class Script;
struct PointerEventData;
class SceneReferenceResolver;

class UIButton : public Component, public IPointerEventHandler
{
private:
	struct ButtonEventBinding
	{
		UID gameObjectUid = 0;
		GameObject* targetGameObject = nullptr;

		UID componentUid = 0;
		Component* targetComponent = nullptr;

		std::string methodName;

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

	UIButton* getNavUp() const { return m_navUp; }
	UIButton* getNavDown() const { return m_navDown; }
	UIButton* getNavLeft() const { return m_navLeft; }
	UIButton* getNavRight() const { return m_navRight; }

	void onSelect();
	void onDeselect();

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
	void applyTargetTexture(const AssetReference& assetId);
	void applyCurrentStateTexture();
	const AssetReference& getDefaultTextureAssetId();

#pragma region Data
	UIImage* m_targetGraphic = nullptr;
	UID m_targetGraphicUid = 0;
	AssetReference m_defaultTextureAssetId = {};
	AssetReference m_hoverTextureAssetId = {};
	AssetReference m_pressedTextureAssetId = {};

	bool m_isPressed = false;
	bool m_isHovered = false;
	bool m_isSelected = false;

	UIButton* m_navUp = nullptr;
	UIButton* m_navDown = nullptr;
	UIButton* m_navLeft = nullptr;
	UIButton* m_navRight = nullptr;

	UID m_navUpUid = 0;
	UID m_navDownUid = 0;
	UID m_navLeftUid = 0;
	UID m_navRightUid = 0;
#pragma endregion

#pragma region EventBindings
	std::vector<ButtonEventBinding> m_bindingsOnHover;
	std::vector<ButtonEventBinding> m_bindingsOnPress;
	std::vector<ButtonEventBinding> m_bindingsOnRelease;
#pragma endregion
};