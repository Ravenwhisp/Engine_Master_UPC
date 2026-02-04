#include "Globals.h"
#include "LightEditor.h"



void LightEditor::Render()
{
	Light* light = GetComponent();

	float colour[3], ambientColour[3];
	VecToFloat(light->GetColour(), colour);
	VecToFloat(light->GetAmbientColour(), ambientColour);

	if (ImGui::InputFloat3("Colour", colour)) {
		auto colourVec = Vector3(colour);
		light->SetColour(colourVec);
	}

	if (ImGui::InputFloat3("AmbientColour", ambientColour)) {
		auto colourVec = Vector3(ambientColour);
		light->SetAmbientColour(colourVec);
	}
}
