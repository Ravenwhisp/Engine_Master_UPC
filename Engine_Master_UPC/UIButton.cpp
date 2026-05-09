#include "Globals.h"
#include "UIButton.h"

#include "SceneReferenceResolver.h"
#include "Script.h"
#include "GameObject.h"
#include "UIImage.h"
#include "ScriptComponent.h"
#include "Transform2D.h"

#include <imgui.h>
#include <format>
#include <cstring>


UIButton::UIButton(UID id, GameObject* owner) : Component(id, ComponentType::UIBUTTON, owner) { }

std::unique_ptr<Component> UIButton::clone(GameObject* newOwner) const
{
	auto cloned = std::make_unique<UIButton>(m_uuid, newOwner);

	cloned->setActive(isActive());
	cloned->m_targetGraphic = m_targetGraphic;
	cloned->m_targetGraphicUid = m_targetGraphicUid;
	cloned->m_defaultTextureAssetId = m_defaultTextureAssetId;
	cloned->m_hoverTextureAssetId = m_hoverTextureAssetId;
	cloned->m_pressedTextureAssetId = m_pressedTextureAssetId;
	cloned->m_isHovered = m_isHovered;
	cloned->m_isPressed = m_isPressed;
	cloned->m_isSelected = m_isSelected;

	cloned->m_navUp = m_navUp;
	cloned->m_navDown = m_navDown;
	cloned->m_navLeft = m_navLeft;
	cloned->m_navRight = m_navRight;
	cloned->m_navUpUid = m_navUpUid;
	cloned->m_navDownUid = m_navDownUid;
	cloned->m_navLeftUid = m_navLeftUid;
	cloned->m_navRightUid = m_navRightUid;
	cloned->m_bindingsOnHover = m_bindingsOnHover;
	cloned->m_bindingsOnPress = m_bindingsOnPress;
	cloned->m_bindingsOnRelease = m_bindingsOnRelease;

	return cloned;
}

void UIButton::onSelect()
{
	if (!isActive()) return;
	if (m_isSelected) return;
	m_isSelected = true;
	m_isHovered = true;
	applyCurrentStateTexture();
}

void UIButton::onDeselect()
{
	if (!m_isSelected) return;
	m_isSelected = false;
	m_isHovered = false;
	m_isPressed = false;
	applyCurrentStateTexture();
}

void UIButton::setTargetGraphic(UIImage* img)
{
	m_targetGraphic = img;
	m_targetGraphicUid = img ? img->getID() : 0;
	m_defaultTextureAssetId = img->getTextureAssetId();
	applyCurrentStateTexture();
}

#pragma region Events
void UIButton::applyTargetTexture(AssetReference& assetId)
{
	if (!m_targetGraphic)
	{
		return;
	}

	if (!assetId.isValid())
	{
		return;
	}

	m_targetGraphic->setTextureAssetId(assetId);
}

AssetReference* UIButton::getDefaultTextureAssetId() const
{
	if (m_defaultTextureAssetId->isValid())
	{
		return m_defaultTextureAssetId;
	}

	if (m_targetGraphic)
	{
		return m_targetGraphic->getTextureAssetId();
	}

	return new AssetReference();
}

void UIButton::applyCurrentStateTexture()
{
	AssetReference* targetAsset = getDefaultTextureAssetId();

	if (m_isPressed && m_isHovered)
	{
		if (m_pressedTextureAssetId->isValid())
		{
			targetAsset = m_pressedTextureAssetId;
		}
		else if (m_hoverTextureAssetId->isValid())
		{
			targetAsset = m_hoverTextureAssetId;
		}
	}
	else if (m_isHovered)
	{
		if (m_hoverTextureAssetId->isValid())
		{
			targetAsset = m_hoverTextureAssetId;
		}
	}

	applyTargetTexture(*targetAsset);
}

void UIButton::onPointerEnter(PointerEventData&)
{
	if (!isActive()) return;
	m_isHovered = true;
	applyCurrentStateTexture();

	executeBindings(m_bindingsOnHover);
}

void UIButton::onPointerExit(PointerEventData&)
{
	m_isHovered = false;
	applyCurrentStateTexture();
}

void UIButton::onPointerDown(PointerEventData&)
{
	if (!isActive()) return;

	m_isPressed = true;
	applyCurrentStateTexture();

	executeBindings(m_bindingsOnPress);
}

void UIButton::onPointerUp(PointerEventData&)
{
	m_isPressed = false;
	applyCurrentStateTexture();
	if (m_isSelected)
	{
		executeBindings(m_bindingsOnRelease);
	}
}

void UIButton::executeBindings(std::vector<ButtonEventBinding>& bindings)
{
	for (auto& binding : bindings)
	{
		if (!binding.targetGameObject)
			continue;

		if (binding.methodName == "GameObject.SetActive")
		{
			binding.targetGameObject->SetActive(binding.paramBool);
			continue;
		}

		if (!binding.targetComponent)
			continue;

		if (binding.targetComponent->getType() == ComponentType::SCRIPT)
		{
			if (!binding.function && !binding.paramFunc)
				continue;

			ScriptComponent* scriptComponent = static_cast<ScriptComponent*>(binding.targetComponent);
			Script* script = scriptComponent->getScript();
			if (!script)
				continue;

			switch (binding.paramType)
			{
			case ScriptMethodParamType::None:
				if (binding.function)
				{
					binding.function(script);
				}
				break;
			case ScriptMethodParamType::Float:
				if (binding.paramFunc)
				{
					binding.paramFunc(script, &binding.paramFloat);
				}
				break;
			case ScriptMethodParamType::Int:
				if (binding.paramFunc)
				{
					binding.paramFunc(script, &binding.paramInt);
				}
				break;
			case ScriptMethodParamType::Bool:
				if (binding.paramFunc)
				{
					binding.paramFunc(script, &binding.paramBool);
				}
				break;
			case ScriptMethodParamType::Vec3:
				if (binding.paramFunc)
				{
					binding.paramFunc(script, &binding.paramVec3);
				}
				break;
			case ScriptMethodParamType::String:
				if (binding.paramFunc)
				{
					binding.paramFunc(script, &binding.paramString);
				}
				break;
			case ScriptMethodParamType::Unsupported:
				break;
			}
		}
	}
}
#pragma endregion

#pragma region Editor

void UIButton::drawUi()
{
	ImGui::Text("UIButton");
	ImGui::Separator();


	ImGui::Button("TargetGraphic reference");
	if (ImGui::BeginDragDropTarget())
	{
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
		{
			Component* comp = *(Component**)payload->Data;

			if (comp && comp->getType() == ComponentType::UIIMAGE)
			{
				setTargetGraphic(static_cast<UIImage*>(comp));
			}
		}
		ImGui::EndDragDropTarget();
	}

	ImGui::Text("Target Graphic: %s", m_targetGraphic ? "Assigned" : "None");
	if (m_targetGraphic)
	{
		ImGui::SeparatorText("Button Textures");

		ImGui::Button("Hover Texture");
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
			{
				AssetReference* data = static_cast<AssetReference*>(payload->Data);
				m_hoverTextureAssetId = data;
				applyCurrentStateTexture();
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();
		ImGui::Text("%s", m_hoverTextureAssetId->isValid() ? "Assigned" : "Default");
		ImGui::SameLine();
		if (ImGui::Button("Clear##HoverTexture"))
		{
			m_hoverTextureAssetId = new AssetReference();
			applyCurrentStateTexture();
		}

		ImGui::Button("Pressed Texture");
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
			{
				AssetReference* data = static_cast<AssetReference*>(payload->Data);
				m_pressedTextureAssetId = data;
				applyCurrentStateTexture();
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();
		ImGui::Text("%s", m_pressedTextureAssetId->isValid() ? "Assigned" : "Default");
		ImGui::SameLine();
		if (ImGui::Button("Clear##PressedTexture"))
		{
			m_pressedTextureAssetId = new AssetReference();
			applyCurrentStateTexture();
		}
	}

	ImGui::Separator();
	ImGui::TextUnformatted("Navigation (Explicit)");

	auto drawNavSlot = [&](const char* label, UIButton*& ref, UID& refUid)
	{
		ImGui::Button(label);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
			{
				Component* comp = *(Component**)payload->Data;
				if (comp && comp->getType() == ComponentType::UIBUTTON)
				{
					ref = static_cast<UIButton*>(comp);
					refUid = ref ? ref->getID() : 0;
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();
		ImGui::Text("%s", ref ? ref->getOwner()->GetName().c_str() : "None");
		ImGui::SameLine();
		std::string clearLabel = std::string("Clear##") + label;
		if (ImGui::SmallButton(clearLabel.c_str()))
		{
			ref = nullptr;
			refUid = 0;
		}
	};

	drawNavSlot("Up", m_navUp, m_navUpUid);
	drawNavSlot("Down", m_navDown, m_navDownUid);
	drawNavSlot("Left", m_navLeft, m_navLeftUid);
	drawNavSlot("Right", m_navRight, m_navRightUid);

	ImGui::Separator();

	drawBindingsUI("On Hover", m_bindingsOnHover);
	drawBindingsUI("On Press", m_bindingsOnPress);
	drawBindingsUI("On Release", m_bindingsOnRelease);
}

void UIButton::drawBindingsUI(const char* label, std::vector<ButtonEventBinding>& bindings)
{
	std::string headerLabel = std::format("{} Events", label);
	if (!ImGui::CollapsingHeader(headerLabel.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	std::string addLabel = std::format("Add {}###Add{}", label, label);
	if (ImGui::Button(addLabel.c_str()))
	{
		bindings.emplace_back();
	}

	ImGui::PushID(label);
	
	ImGui::BeginGroup();
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 6));
	ImGui::PushStyleColor(ImGuiCol_Border, ImGui::GetStyleColorVec4(ImGuiCol_Border));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 0));

	ImGui::BeginGroup();
	const ImVec2 groupPadding(4.0f, 4.0f);
	ImGui::Dummy(ImVec2(groupPadding.x, groupPadding.y));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::BeginGroup();

	for (int i = 0; i < bindings.size(); ++i)
	{
		auto& binding = bindings[i];

		ImGui::PushID(i);

		if (ImGui::BeginTable("BindingTable", 2, ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Binding", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Remove", ImGuiTableColumnFlags_WidthFixed, 24.0f);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);

			ImGui::Text("Script:");
			std::string scriptLabel;
			if (binding.targetGameObject)
			{
				scriptLabel = binding.targetGameObject->GetName();
			}
			else
			{
				scriptLabel = "Drop GameObject";
			}

			ImGui::Button(scriptLabel.c_str());

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GAME_OBJECT"))
				{
					GameObject* go = *(GameObject**)payload->Data;
					if (go)
					{
						binding.targetGameObject = go;
						binding.gameObjectUid = go->GetID();
						
						binding.targetComponent = nullptr;
						binding.componentUid = 0;
						binding.methodName.clear();
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (binding.targetGameObject)
			{
				const char* preview = binding.methodName.empty() ? "Select Method" : binding.methodName.c_str();
				std::string comboLabel = std::format("Method###Method{}_{}", label, i);
				if (ImGui::BeginCombo(comboLabel.c_str(), preview))
				{
					if (ImGui::BeginMenu("GameObject"))
					{
						bool selected = (binding.methodName == "GameObject.SetActive");
						if (ImGui::Selectable("SetActive", selected))
						{
							binding.targetComponent = nullptr;
							binding.componentUid = 0;
							binding.methodName = "GameObject.SetActive";
							binding.paramType = ScriptMethodParamType::Bool;
							binding.paramName = "Active";
							binding.paramBool = false;
						}
						ImGui::EndMenu();
					}

					std::vector<Component*> components = binding.targetGameObject->GetAllComponents();
					for (Component* comp : components)
					{
						if (comp->getType() == ComponentType::SCRIPT)
						{
							ScriptComponent* scriptComp = static_cast<ScriptComponent*>(comp);
							if (ImGui::BeginMenu(scriptComp->getScriptName().c_str()))
							{
								Script* script = scriptComp->getScript();
								if (script)
								{
									ScriptMethodList methods = script->getExposedMethods();
									for (size_t j = 0; j < methods.count; ++j)
									{
										const auto& method = methods.methods[j];
										if (method.paramType == ScriptMethodParamType::Unsupported)
											continue;
										bool selected = (binding.targetComponent == comp && binding.methodName == method.name);
										if (ImGui::Selectable(method.name, selected))
										{
											binding.targetComponent = comp;
											binding.componentUid = comp->getID();
											binding.methodName = method.name;
											const ScriptMethodParamType previousType = binding.paramType;
											binding.function = method.func;
											binding.paramFunc = method.paramFunc;
											binding.paramType = method.paramType;
											binding.paramName = method.paramName ? method.paramName : "";
											if (binding.paramType != previousType)
											{
												binding.paramFloat = 0.0f;
												binding.paramInt = 0;
												binding.paramBool = false;
												binding.paramVec3 = Vector3(0.0f, 0.0f, 0.0f);
												binding.paramString.clear();
											}
										}
										if (selected)
											ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndMenu();
							}
						}
					}
					ImGui::EndCombo();
				}

				if (!binding.methodName.empty())
				{
					const char* paramLabel = binding.paramName.empty() ? "Parameter" : binding.paramName.c_str();
					switch (binding.paramType)
					{
					case ScriptMethodParamType::None:
						break;
					case ScriptMethodParamType::Float:
						ImGui::DragFloat(paramLabel, &binding.paramFloat, 0.1f);
						break;
					case ScriptMethodParamType::Int:
						ImGui::DragInt(paramLabel, &binding.paramInt);
						break;
					case ScriptMethodParamType::Bool:
						ImGui::Checkbox(paramLabel, &binding.paramBool);
						break;
					case ScriptMethodParamType::Vec3:
						ImGui::DragFloat3(paramLabel, &binding.paramVec3.x, 0.1f);
						break;
					case ScriptMethodParamType::String:
					{
						char buffer[256];
						std::strncpy(buffer, binding.paramString.c_str(), sizeof(buffer));
						buffer[sizeof(buffer) - 1] = '\0';
						if (ImGui::InputText(paramLabel, buffer, sizeof(buffer)))
						{
							binding.paramString = buffer;
						}
						break;
					}
					case ScriptMethodParamType::Unsupported:
						ImGui::TextDisabled("Parameters not supported");
						break;
					}
				}
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
			std::string removeLabel = std::format("X###Remove{}_{}", label, i);
			if (ImGui::SmallButton(removeLabel.c_str()))
			{
				bindings.erase(bindings.begin() + i);
				ImGui::PopStyleColor(3);
				ImGui::EndTable();
				ImGui::PopID();
				break;
			}
			ImGui::PopStyleColor(3);
			ImGui::EndTable();
		}

#pragma endregion

		ImGui::PopID();
	}

	ImGui::PopID();
	ImGui::EndGroup();
	ImGui::Dummy(ImVec2(groupPadding.x, groupPadding.y));
	ImGui::EndGroup();

	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar();

	ImVec2 min = ImGui::GetItemRectMin();
	ImVec2 max = ImGui::GetItemRectMax();
	ImGui::GetWindowDrawList()->AddRect(min, max, ImGui::GetColorU32(ImGuiCol_Border));
	ImGui::EndGroup();
}

#pragma endregion

#pragma region Serialization
void UIButton::SerializeBindings(const std::vector<UIButton::ButtonEventBinding>& bindings, rapidjson::Value& array, rapidjson::Document& doc)
{
	for (const auto& b : bindings)
	{
		rapidjson::Value obj(rapidjson::kObjectType);

		obj.AddMember("GameObjectUID", (uint64_t)b.gameObjectUid, doc.GetAllocator());
		obj.AddMember("ComponentUID", (uint64_t)b.componentUid, doc.GetAllocator());
		obj.AddMember("Method", rapidjson::Value(b.methodName.c_str(), doc.GetAllocator()), doc.GetAllocator());
		obj.AddMember("ParamType", static_cast<int>(b.paramType), doc.GetAllocator());
		switch (b.paramType)
		{
		case ScriptMethodParamType::Float:
			obj.AddMember("ParamValue", b.paramFloat, doc.GetAllocator());
			break;
		case ScriptMethodParamType::Int:
			obj.AddMember("ParamValue", b.paramInt, doc.GetAllocator());
			break;
		case ScriptMethodParamType::Bool:
			obj.AddMember("ParamValue", b.paramBool, doc.GetAllocator());
			break;
		case ScriptMethodParamType::Vec3:
		{
			rapidjson::Value array(rapidjson::kArrayType);
			array.PushBack(b.paramVec3.x, doc.GetAllocator());
			array.PushBack(b.paramVec3.y, doc.GetAllocator());
			array.PushBack(b.paramVec3.z, doc.GetAllocator());
			obj.AddMember("ParamValue", array, doc.GetAllocator());
			break;
		}
		case ScriptMethodParamType::String:
			obj.AddMember("ParamValue", rapidjson::Value(b.paramString.c_str(), doc.GetAllocator()), doc.GetAllocator());
			break;
		case ScriptMethodParamType::None:
		case ScriptMethodParamType::Unsupported:
			break;
		}

		array.PushBack(obj, doc.GetAllocator());
	}
}

rapidjson::Value UIButton::getJSON(rapidjson::Document& domTree)
{
	rapidjson::Value json(rapidjson::kObjectType);

	json.AddMember("UID", m_uuid, domTree.GetAllocator());
	json.AddMember("ComponentType", int(ComponentType::UIBUTTON), domTree.GetAllocator());
	json.AddMember("Active", isActive(), domTree.GetAllocator());
	json.AddMember("TargetGraphicUID", (uint64_t)m_targetGraphicUid, domTree.GetAllocator());
	json.AddMember("DefaultTextureAssetId", m_defaultTextureAssetId->getJson(domTree.GetAllocator()), domTree.GetAllocator());
	json.AddMember("HoverTextureAssetId", m_hoverTextureAssetId->getJson(domTree.GetAllocator()), domTree.GetAllocator());
	json.AddMember("PressedTextureAssetId", m_pressedTextureAssetId->getJson(domTree.GetAllocator()), domTree.GetAllocator());

	json.AddMember("NavUpUID", (uint64_t)m_navUpUid, domTree.GetAllocator());
	json.AddMember("NavDownUID", (uint64_t)m_navDownUid, domTree.GetAllocator());
	json.AddMember("NavLeftUID", (uint64_t)m_navLeftUid, domTree.GetAllocator());
	json.AddMember("NavRightUID", (uint64_t)m_navRightUid, domTree.GetAllocator());

	rapidjson::Value hoverArray(rapidjson::kArrayType);
	rapidjson::Value pressArray(rapidjson::kArrayType);
	rapidjson::Value releaseArray(rapidjson::kArrayType);

	SerializeBindings(m_bindingsOnHover, hoverArray, domTree);
	SerializeBindings(m_bindingsOnPress, pressArray, domTree);
	SerializeBindings(m_bindingsOnRelease, releaseArray, domTree);

	json.AddMember("OnHover", hoverArray, domTree.GetAllocator());
	json.AddMember("OnPress", pressArray, domTree.GetAllocator());
	json.AddMember("OnRelease", releaseArray, domTree.GetAllocator());

	return json;
}

void UIButton::DeserializeBindings(const rapidjson::Value& array, std::vector<UIButton::ButtonEventBinding>& outBindings)
{
	outBindings.clear();

	for (auto& v : array.GetArray())
	{
		UIButton::ButtonEventBinding b;

		b.gameObjectUid = (UID)v["GameObjectUID"].GetUint64();
		b.componentUid = (UID)v["ComponentUID"].GetUint64();
		b.methodName = v["Method"].GetString();
		b.paramType = ScriptMethodParamType::None;
		b.paramFloat = 0.0f;
		b.paramInt = 0;
		b.paramBool = false;
		b.paramVec3 = Vector3(0.0f, 0.0f, 0.0f);
		b.paramString.clear();
		if (v.HasMember("ParamType") && v["ParamType"].IsInt())
		{
			b.paramType = static_cast<ScriptMethodParamType>(v["ParamType"].GetInt());
		}
		if (v.HasMember("ParamValue"))
		{
			const auto& paramValue = v["ParamValue"];
			switch (b.paramType)
			{
			case ScriptMethodParamType::Float:
				if (paramValue.IsNumber())
					b.paramFloat = paramValue.GetFloat();
				break;
			case ScriptMethodParamType::Int:
				if (paramValue.IsInt())
					b.paramInt = paramValue.GetInt();
				break;
			case ScriptMethodParamType::Bool:
				if (paramValue.IsBool())
					b.paramBool = paramValue.GetBool();
				break;
			case ScriptMethodParamType::Vec3:
				if (paramValue.IsArray() && paramValue.Size() == 3)
				{
					b.paramVec3 = Vector3(paramValue[0].GetFloat(), paramValue[1].GetFloat(), paramValue[2].GetFloat());
				}
				break;
			case ScriptMethodParamType::String:
				if (paramValue.IsString())
					b.paramString = paramValue.GetString();
				break;
			case ScriptMethodParamType::None:
			case ScriptMethodParamType::Unsupported:
				break;
			}
		}

		outBindings.push_back(b);
	}
}

bool UIButton::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("TargetGraphicUID"))
	{
		m_targetGraphicUid = (UID)componentInfo["TargetGraphicUID"].GetUint64();
	}

	if (componentInfo.HasMember("DefaultTextureAssetId"))
	{
		m_defaultTextureAssetId->deserializeJson(componentInfo["DefaultTextureAssetId"]);
	}

	if (componentInfo.HasMember("HoverTextureAssetId"))
	{
		m_hoverTextureAssetId->deserializeJson(componentInfo["HoverTextureAssetId"]);
	}

	if (componentInfo.HasMember("PressedTextureAssetId"))
	{
		m_pressedTextureAssetId->deserializeJson(componentInfo["PressedTextureAssetId"]);
	}

	if (componentInfo.HasMember("NavUpUID"))
		m_navUpUid = (UID)componentInfo["NavUpUID"].GetUint64();
	else
		m_navUpUid = 0;

	if (componentInfo.HasMember("NavDownUID"))
		m_navDownUid = (UID)componentInfo["NavDownUID"].GetUint64();
	else
		m_navDownUid = 0;

	if (componentInfo.HasMember("NavLeftUID"))
		m_navLeftUid = (UID)componentInfo["NavLeftUID"].GetUint64();
	else
		m_navLeftUid = 0;

	if (componentInfo.HasMember("NavRightUID"))
		m_navRightUid = (UID)componentInfo["NavRightUID"].GetUint64();
	else
		m_navRightUid = 0;

	m_targetGraphic = nullptr;
	m_navUp = nullptr;
	m_navDown = nullptr;
	m_navLeft = nullptr;
	m_navRight = nullptr;

	if (componentInfo.HasMember("OnHover"))
	{
		DeserializeBindings(componentInfo["OnHover"], m_bindingsOnHover);
	}

	if (componentInfo.HasMember("OnPress"))
	{
		DeserializeBindings(componentInfo["OnPress"], m_bindingsOnPress);
	}

	if (componentInfo.HasMember("OnRelease"))
	{
		DeserializeBindings(componentInfo["OnRelease"], m_bindingsOnRelease);
	}

	return true;
}

void UIButton::ResolveBinding(UIButton::ButtonEventBinding& b, const SceneReferenceResolver& resolver)
{
	b.targetComponent = nullptr;
	b.targetGameObject = nullptr;
	b.function = nullptr;
	b.paramFunc = nullptr;
	b.paramType = ScriptMethodParamType::None;
	b.paramName.clear();

	if (b.gameObjectUid != 0)
	{
		b.targetGameObject = resolver.getClonedGameObject(b.gameObjectUid);
	}

	if (b.methodName == "GameObject.SetActive")
	{
		b.paramName = "Active";
		b.paramType = ScriptMethodParamType::Bool;
		return;
	}

	if (b.componentUid == 0)
		return;

	Component* resolved = resolver.getClonedComponent(b.componentUid);
	if (!resolved)
		return;

	b.targetComponent = resolved;
	b.targetGameObject = resolved->getOwner();

	if (resolved->getType() != ComponentType::SCRIPT)
		return;

	ScriptComponent* sc = static_cast<ScriptComponent*>(resolved);

	Script* script = sc->getScript();
	if (!script)
		return;

	ScriptMethodList methods = script->getExposedMethods();

	for (size_t i = 0; i < methods.count; ++i)
	{
		if (b.methodName == methods.methods[i].name)
		{
			const ScriptMethodInfo& method = methods.methods[i];
			b.function = method.func;
			b.paramFunc = method.paramFunc;
			b.paramType = method.paramType;
			b.paramName = method.paramName ? method.paramName : "";
			break;
		}
	}
}

void UIButton::fixReferences(const SceneReferenceResolver& resolver)
{
	if (m_targetGraphicUid != 0)
	{
		m_targetGraphic = static_cast<UIImage*>(resolver.getClonedComponent(m_targetGraphicUid));
	}

	if (m_targetGraphic)
	{
		if (!m_defaultTextureAssetId->isValid())
		{
			m_defaultTextureAssetId = m_targetGraphic->getTextureAssetId();
		}
		applyCurrentStateTexture();
	}

	for (auto& b : m_bindingsOnHover)
	{
		ResolveBinding(b, resolver);
	}

	for (auto& b : m_bindingsOnPress)
	{
		ResolveBinding(b, resolver);
	}

	for (auto& b : m_bindingsOnRelease)
	{
		ResolveBinding(b, resolver);
	}

	if (m_navUpUid != 0)
		m_navUp = static_cast<UIButton*>(resolver.getClonedComponent(m_navUpUid));
	if (m_navDownUid != 0)
		m_navDown = static_cast<UIButton*>(resolver.getClonedComponent(m_navDownUid));
	if (m_navLeftUid != 0)
		m_navLeft = static_cast<UIButton*>(resolver.getClonedComponent(m_navLeftUid));
	if (m_navRightUid != 0)
		m_navRight = static_cast<UIButton*>(resolver.getClonedComponent(m_navRightUid));
}

#pragma endregion