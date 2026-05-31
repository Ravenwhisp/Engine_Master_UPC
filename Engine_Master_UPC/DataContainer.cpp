#include "Globals.h"
#include "DataContainer.h"
#include "Application.h"
#include "ModuleAssets.h"
#include "Script.h"
#include "ScriptComponent.h"

#include <string>

namespace
{
	struct DataContainerScriptDummy : Script
	{
		DataContainerScriptDummy() : Script(nullptr) {}
		void onFieldEdited(const ScriptFieldInfo&) override {}
	};
	static DataContainerScriptDummy s_dummyScript;

	struct DataContainerComponentDummy : ScriptComponent
	{
		DataContainerComponentDummy() : ScriptComponent(0, nullptr) {}
	};
	static DataContainerComponentDummy s_dummyComponent;
}

rapidjson::Value DataContainer::getJson(rapidjson::Document::AllocatorType& allocator) const
{
	ScriptFieldList fields = getExposedFields();
	if (!fields.fields.empty())
	{
		rapidjson::Value obj(rapidjson::kObjectType);
		obj.AddMember("_typeName", rapidjson::Value(getTypeName(), allocator), allocator);

		rapidjson::Document tempDoc(&allocator);
		const char* base = reinterpret_cast<const char*>(this);
		for (const auto& field : fields.fields)
		{
			field.handler->serialize(field, base + field.offset, obj, tempDoc);
		}
		return obj;
	}

	return rapidjson::Value(m_data, allocator);
}

bool DataContainer::deserializeJson(const rapidjson::Value& obj)
{
	if (!obj.IsObject())
	{
		return false;
	}

	m_data.CopyFrom(obj, m_data.GetAllocator());

	ScriptFieldList fields = getExposedFields();
	if (!fields.fields.empty())
	{
		char* base = reinterpret_cast<char*>(this);
		for (const auto& field : fields.fields)
		{
			if (obj.HasMember(field.name))
			{
				field.handler->deserialize(field, base + field.offset, obj[field.name]);
			}
		}
	}

	return true;
}

void DataContainer::drawUI()
{
	ImGui::Text("Data Container");
	ImGui::Separator();

	ImGui::TextDisabled("UID: %llu", static_cast<unsigned long long>(getUID()));

	ImGui::Spacing();

	if (ImGui::Button("Save"))
	{
		app->getModuleAssets()->save(*this);
	}

	ImGui::SameLine();
	ImGui::TextDisabled("(Ctrl+S)");

	ImGui::Spacing();
	ImGui::SeparatorText("Properties");

	ScriptFieldList fields = getExposedFields();
	if (!fields.fields.empty())
	{
		char* base = reinterpret_cast<char*>(this);
		for (const ScriptFieldInfo& field : fields.fields)
		{
			void* data = base + field.offset;
			field.handler->drawUi(field, data, s_dummyScript, s_dummyComponent);
		}
		return;
	}

	if (m_data.IsObject() && m_data.MemberCount() == 0)
	{
		ImGui::TextDisabled("No properties defined.");
		ImGui::Spacing();
		ImGui::TextWrapped("Override getExposedFields() in a subclass to add typed properties,");
		ImGui::TextWrapped("or add entries directly in the .datacontainer JSON file.");
		return;
	}

	if (!m_data.IsObject())
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Invalid data document.");
		return;
	}

	for (auto it = m_data.MemberBegin(); it != m_data.MemberEnd(); ++it)
	{
		const char* key = it->name.GetString();
		rapidjson::Value& value = it->value;

		ImGui::PushID(key);

		if (value.IsString())
		{
			std::string buffer = value.GetString();
			buffer.resize(256, '\0');
			if (ImGui::InputText(key, &buffer[0], buffer.size()))
			{
				value.SetString(buffer.c_str(), m_data.GetAllocator());
			}
		}
		else if (value.IsInt())
		{
			int v = value.GetInt();
			if (ImGui::InputInt(key, &v))
			{
				value.SetInt(v);
			}
		}
		else if (value.IsInt64())
		{
			int v = static_cast<int>(value.GetInt64());
			if (ImGui::InputInt(key, &v))
			{
				value.SetInt64(v);
			}
		}
		else if (value.IsUint64())
		{
			int v = static_cast<int>(value.GetUint64());
			if (ImGui::InputInt(key, &v))
			{
				value.SetUint64(static_cast<uint64_t>(v));
			}
		}
		else if (value.IsFloat() || value.IsDouble())
		{
			float v = value.GetFloat();
			if (ImGui::DragFloat(key, &v, 0.1f))
			{
				value.SetFloat(v);
			}
		}
		else if (value.IsBool())
		{
			bool v = value.GetBool();
			if (ImGui::Checkbox(key, &v))
			{
				value.SetBool(v);
			}
		}
		else if (value.IsArray())
		{
			if (ImGui::TreeNode(key, "%s [%u items]", key, value.Size()))
			{
				for (rapidjson::SizeType i = 0; i < value.Size(); ++i)
				{
					ImGui::PushID(static_cast<int>(i));
					rapidjson::Value& elem = value[i];

					if (elem.IsString())
					{
						ImGui::Text("[%u]: %s", i, elem.GetString());
					}
					else if (elem.IsNumber())
					{
						if (elem.IsInt())
							ImGui::Text("[%u]: %d", i, elem.GetInt());
						else
							ImGui::Text("[%u]: %.3f", i, elem.GetFloat());
					}
					else if (elem.IsBool())
					{
						ImGui::Text("[%u]: %s", i, elem.GetBool() ? "true" : "false");
					}
					else if (elem.IsObject())
					{
						ImGui::Text("[%u]: { ... }", i);
					}

					ImGui::PopID();
				}

				ImGui::TreePop();
			}
		}
		else if (value.IsObject())
		{
			ImGui::Text("%s: { ... }", key);
		}
		else
		{
			ImGui::TextDisabled("%s: (unsupported type)", key);
		}

		ImGui::PopID();
	}
}
