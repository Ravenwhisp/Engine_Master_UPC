#include "Globals.h"
#include "UIButton.h"

#include "SceneReferenceResolver.h"
#include "Script.h"
#include "GameObject.h"
#include "UIImage.h"
#include "ScriptComponent.h"

#include <imgui.h>
#include <format>


UIButton::UIButton(UID id, GameObject* owner) : Component(id, ComponentType::UIBUTTON, owner) { }

std::unique_ptr<Component> UIButton::clone(GameObject* newOwner) const
{
	auto cloned = std::make_unique<UIButton>(m_uuid, newOwner);

	cloned->setActive(isActive());
	cloned->m_targetGraphic = m_targetGraphic;
	cloned->m_targetGraphicUid = m_targetGraphicUid;

	return cloned;
}

void UIButton::setTargetGraphic(UIImage* img)
{
	m_targetGraphic = img;
	m_targetGraphicUid = img ? img->getID() : 0;
}

#pragma region Events
void UIButton::onPointerEnter(PointerEventData&)
{
	if (!isActive()) return;

	executeBindings(m_bindingsOnHover);
}

void UIButton::onPointerExit(PointerEventData&)
{

}

void UIButton::onPointerDown(PointerEventData&)
{
	if (!isActive()) return;

	m_isPressed = true;

	executeBindings(m_bindingsOnPress);
}

void UIButton::onPointerUp(PointerEventData&)
{
	m_isPressed = false;
	executeBindings(m_bindingsOnRelease);
}

void UIButton::onPointerClick(PointerEventData&)
{
	press();
}

void UIButton::press()
{
	if (!isActive()) return;

	onClick.Broadcast();
}

void UIButton::executeBindings(std::vector<ButtonEventBinding>& bindings)
{
	for (auto& binding : bindings)
	{
		if (!binding.component || !binding.function)
			continue;

		Script* script = binding.component->getScript();
		if (!script)
			continue;

		binding.function(script);
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
			if (binding.component)
			{
				scriptLabel = std::format("{} ({})###Script{}_{}", binding.component->getOwner()->GetName(), binding.component->getScriptName(), label, i);
			}
			else
			{
				scriptLabel = std::format("Drop Script###Script{}_{}", label, i);
			}

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));

			ImGui::Button(scriptLabel.c_str());

			ImGui::PopStyleColor(3);

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("COMPONENT"))
				{
					Component* comp = *(Component**)payload->Data;

					if (comp && comp->getType() == ComponentType::SCRIPT)
					{
						binding.component = static_cast<ScriptComponent*>(comp);
						binding.componentUid = comp->getID();
						binding.gameObjectUid = comp->getOwner()->GetID();
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (binding.component)
			{
				Script* script = binding.component->getScript();

				if (script)
				{
					ScriptMethodList methods = script->getExposedMethods();
					const char* preview = binding.methodName.empty() ? "Select Method" : binding.methodName.c_str();
					std::string comboLabel = std::format("Method###Method{}_{}", label, i);
					if (ImGui::BeginCombo(comboLabel.c_str(), preview))
					{
						for (size_t j = 0; j < methods.count; ++j)
						{
							const auto& method = methods.methods[j];
							bool selected = (binding.methodName == method.name);
							if (ImGui::Selectable(method.name, selected))
							{
								binding.methodName = method.name;
								binding.function = method.func;
							}

							if (selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
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

		outBindings.push_back(b);
	}
}

bool UIButton::deserializeJSON(const rapidjson::Value& componentInfo)
{
	if (componentInfo.HasMember("TargetGraphicUID"))
	{
		m_targetGraphicUid = (UID)componentInfo["TargetGraphicUID"].GetUint64();
	}

	m_targetGraphic = nullptr;

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
	b.component = nullptr;
	b.function = nullptr;

	if (b.componentUid == 0)
		return;

	Component* resolved = resolver.getClonedComponent(b.componentUid);

	if (!resolved || resolved->getType() != ComponentType::SCRIPT)
		return;

	b.component = static_cast<ScriptComponent*>(resolved);

	Script* script = b.component->getScript();
	if (!script)
		return;

	ScriptMethodList methods = script->getExposedMethods();

	for (size_t i = 0; i < methods.count; ++i)
	{
		if (b.methodName == methods.methods[i].name)
		{
			b.function = methods.methods[i].func;
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
}

#pragma endregion