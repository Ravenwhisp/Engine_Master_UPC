#include "Globals.h"
#include "DataContainer.h"
#include "Application.h"
#include "ModuleAssets.h"
#include "FieldUtils.h"
#include "IArchive.h"

void DataContainer::serialize(IArchive& archive)
{
	if (archive.mode() == ArchiveMode::Output && m_typeName.empty())
	{
		m_typeName = getTypeName();
	}
	archive.serialize(m_typeName, "_typeName");

	FieldUtils::serialize(*this, reinterpret_cast<char*>(this), archive);
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

	FieldList fields = getExposedFields();
	if (!fields.fields.empty())
	{
		FieldUtils::drawUi(*this, reinterpret_cast<char*>(this));
		return;
	}

}
