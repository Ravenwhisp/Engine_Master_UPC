#include "Globals.h"
#include "UIButton.h"
#include "JsonArchive.h"

#include "SceneReferenceResolver.h"
#include "Script.h"
#include "GameObject.h"
#include "UIImage.h"
#include "ScriptComponent.h"
#include "Transform2D.h"

#include <imgui.h>
#include <format>
#include <cstring>

#include "Application.h"
#include "ModuleAssets.h"


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

void UIButton::setTargetGraphic(UIImage* img)
{
	m_targetGraphic = img;
	m_targetGraphicUid = img ? img->getID() : 0;
	m_defaultTextureAssetId = img->getTextureAssetId();
	applyCurrentStateTexture();
}

#pragma region Events
void UIButton::applyTargetTexture(const AssetId& assetId)
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

const AssetId& UIButton::getDefaultTextureAssetId()
{
	if (m_defaultTextureAssetId.isValid())
	{
		return m_defaultTextureAssetId;
	}

	if (m_targetGraphic)
	{
		return m_targetGraphic->getTextureAssetId();
	}

	static AssetId s_defaultAsset{};
	return s_defaultAsset;
}

void UIButton::applyCurrentStateTexture()
{
	const AssetId* targetAsset = &getDefaultTextureAssetId();

	if (m_isPressed && m_isHovered)
	{
		if (m_pressedTextureAssetId.isValid())
		{
			targetAsset = &m_pressedTextureAssetId;
		}
		else if (m_hoverTextureAssetId.isValid())
		{
			targetAsset = &m_hoverTextureAssetId;
		}
	}
	else if (m_isHovered || m_isSelected)
	{
		if (m_hoverTextureAssetId.isValid())
		{
			targetAsset = &m_hoverTextureAssetId;
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
}

void UIButton::onPointerClick(PointerEventData&)
{
	if (!isActive()) return;

	executeBindings(m_bindingsOnRelease);
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
				UID* data = static_cast<UID*>(payload->Data);
				m_hoverTextureAssetId = *app->getModuleAssets()->findReference(*data);
				applyCurrentStateTexture();
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();
		ImGui::Text("%s", m_hoverTextureAssetId.isValid() ? "Assigned" : "Default");
		ImGui::SameLine();
		if (ImGui::Button("Clear##HoverTexture"))
		{
			m_hoverTextureAssetId = AssetId();
			applyCurrentStateTexture();
		}

		ImGui::Button("Pressed Texture");
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
			{
				UID* data = static_cast<UID*>(payload->Data);
				m_pressedTextureAssetId = *app->getModuleAssets()->findReference(*data);
				applyCurrentStateTexture();
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();
		ImGui::Text("%s", m_pressedTextureAssetId.isValid() ? "Assigned" : "Default");
		ImGui::SameLine();
		if (ImGui::Button("Clear##PressedTexture"))
		{
			m_pressedTextureAssetId = AssetId();
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

void UIButton::serialize(IArchive& archive)
{
	Component::serialize(archive);

	archive.serialize(m_targetGraphicUid, "TargetGraphicUID");

	archive.beginObject("DefaultTextureAssetId");
	m_defaultTextureAssetId.serialize(archive);
	archive.endObject();

	archive.beginObject("HoverTextureAssetId");
	m_hoverTextureAssetId.serialize(archive);
	archive.endObject();

	archive.beginObject("PressedTextureAssetId");
	m_pressedTextureAssetId.serialize(archive);
	archive.endObject();

	archive.serialize(m_navUpUid, "NavUpUID");
	archive.serialize(m_navDownUid, "NavDownUID");
	archive.serialize(m_navLeftUid, "NavLeftUID");
	archive.serialize(m_navRightUid, "NavRightUID");

	auto serializeBindingVector = [&archive](std::vector<UIButton::ButtonEventBinding>& bindings, const char* name)
	{
		uint32_t count = static_cast<uint32_t>(bindings.size());
		archive.beginArray(count, name);
		if (archive.mode() == ArchiveMode::Input)
			bindings.resize(count);
		for (auto& b : bindings)
		{
			archive.beginObject();
			archive.serialize(b.gameObjectUid, "GameObjectUID");
			archive.serialize(b.componentUid, "ComponentUID");
			archive.serialize(b.methodName, "Method");

			uint8_t paramType = static_cast<uint8_t>(b.paramType);
			archive.serialize(paramType, "ParamType");
			if (archive.mode() == ArchiveMode::Input)
				b.paramType = static_cast<ScriptMethodParamType>(paramType);

			switch (b.paramType)
			{
			case ScriptMethodParamType::None:
			case ScriptMethodParamType::Unsupported:
				break;
			case ScriptMethodParamType::Float:
				archive.serialize(b.paramFloat, "ParamValue");
				break;
			case ScriptMethodParamType::Int:
			{
				uint32_t tmp = static_cast<uint32_t>(b.paramInt);
				archive.serialize(tmp, "ParamValue");
				if (archive.mode() == ArchiveMode::Input)
					b.paramInt = static_cast<int>(tmp);
				break;
			}
			case ScriptMethodParamType::Bool:
				archive.serialize(b.paramBool, "ParamValue");
				break;
			case ScriptMethodParamType::Vec3:
				archive.serialize(b.paramVec3, "ParamValue");
				break;
			case ScriptMethodParamType::String:
				archive.serialize(b.paramString, "ParamValue");
				break;
			}
			archive.endObject();
		}
		archive.endArray();
	};

	serializeBindingVector(m_bindingsOnHover, "BindingsOnHover");
	serializeBindingVector(m_bindingsOnPress, "BindingsOnPress");
	serializeBindingVector(m_bindingsOnRelease, "BindingsOnRelease");
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
		if (!m_defaultTextureAssetId.isValid())
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