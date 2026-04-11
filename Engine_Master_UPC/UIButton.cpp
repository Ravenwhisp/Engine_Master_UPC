#include "Globals.h"
#include "UIButton.h"

#include "SceneReferenceResolver.h"
#include "Script.h"
#include "GameObject.h"
#include "UIImage.h"
#include "ScriptComponent.h"

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
	cloned->m_bindingsOnHover = m_bindingsOnHover;
	cloned->m_bindingsOnPress = m_bindingsOnPress;
	cloned->m_bindingsOnRelease = m_bindingsOnRelease;

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
		if (!binding.component || (!binding.function && !binding.paramFunc))
			continue;

		Script* script = binding.component->getScript();
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
				const ScriptMethodInfo* selectedMethod = nullptr;
				for (size_t j = 0; j < methods.count; ++j)
				{
					if (binding.methodName == methods.methods[j].name)
					{
						selectedMethod = &methods.methods[j];
						break;
					}
				}
					const char* preview = binding.methodName.empty() ? "Select Method" : binding.methodName.c_str();
					std::string comboLabel = std::format("Method###Method{}_{}", label, i);
					if (ImGui::BeginCombo(comboLabel.c_str(), preview))
					{
						for (size_t j = 0; j < methods.count; ++j)
						{
							const auto& method = methods.methods[j];
						if (method.paramType == ScriptMethodParamType::Unsupported)
						{
							continue;
						}
							bool selected = (binding.methodName == method.name);
							if (ImGui::Selectable(method.name, selected))
							{
							const ScriptMethodParamType previousType = binding.paramType;
								binding.methodName = method.name;
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
							selectedMethod = &method;
							}

							if (selected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

				if (selectedMethod)
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
	b.paramFunc = nullptr;
	b.paramType = ScriptMethodParamType::None;
	b.paramName.clear();

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