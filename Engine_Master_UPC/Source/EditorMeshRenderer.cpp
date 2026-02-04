#include "Globals.h"
#include "EditorMeshRenderer.h"
#include "Material.h"


void EditorMeshRenderer::Render()
{
	Emeika::Model* model = GetComponent();

	for (Emeika::Material* material : model->GetMaterials()) {
		auto data = material->GetMaterial();
		float diffuseColour[3], specularColour[3], shininess;
		VecToFloat(data.diffuseColour, diffuseColour);
		VecToFloat(data.specularColour, specularColour);
		shininess = data.shininess;

		if (ImGui::InputFloat3("Diffuse Colour", diffuseColour)) {
			data.diffuseColour = Vector3(diffuseColour[0], diffuseColour[1], diffuseColour[2]);
		}

		if (ImGui::InputFloat3("Specular Colour", specularColour)) {
			data.specularColour = Vector3(specularColour[0], specularColour[1], specularColour[2]);
		}

		if (ImGui::InputFloat("Shininess", &shininess)) {
			data.shininess = shininess;
		}

		material->SetMaterial(data);
	}
}
