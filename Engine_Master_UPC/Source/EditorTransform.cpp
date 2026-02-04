#include "Globals.h"
#include "EditorTransform.h"
#include "Application.h"
#include "CameraModule.h"
#include "SceneEditor.h"
#include "D3D12Module.h"
#include "Transform.h"

void EditorTransform::Render()
{
    // Get current model matrix
    auto transform = GetComponent();
    auto modelMatrix = transform->GetWorldMatrix();

    // Decompose matrix for manual editing
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    ImGuizmo::DecomposeMatrixToComponents((float*)&modelMatrix, matrixTranslation, matrixRotation, matrixScale);

    // ===== MANUAL TRANSFORM CONTROLS =====
    ImGui::Text("Transform:");
    if (ImGui::InputFloat3("Position", matrixTranslation))
    {
        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, (float*)&modelMatrix);
        transform->SetWorldMatrix(modelMatrix);
    }

    if (ImGui::InputFloat3("Rotation", matrixRotation))
    {
        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, (float*)&modelMatrix);
        transform->SetWorldMatrix(modelMatrix);
    }

    if (ImGui::InputFloat3("Scale", matrixScale))
    {
        ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation, matrixScale, (float*)&modelMatrix);
        transform->SetWorldMatrix(modelMatrix);
    }
}