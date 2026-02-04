#include "Globals.h"
#include "LightEditor.h"



void LightEditor::render()
{
	Light* light = getComponent();

	float colour[3], ambientColour[3];
	VecToFloat(light->getColour(), colour);
	VecToFloat(light->getAmbientColour(), ambientColour);

	if (ImGui::InputFloat3("Colour", colour)) {
		auto colourVec = Vector3(colour);
		light->setColour(colourVec);
	}

	if (ImGui::InputFloat3("AmbientColour", ambientColour)) {
		auto colourVec = Vector3(ambientColour);
		light->setAmbientColour(colourVec);
	}
}
