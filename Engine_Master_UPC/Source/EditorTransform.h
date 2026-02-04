#pragma once
#include "EditorWindow.h"
#include "Model.h"
#include "ImGuizmo.h"
#include "EditorComponent.h"

#include "Transform.h"

class EditorTransform : public EditorComponent<Transform>
{
public:
	const char* GetName() const override { return "Transform"; }

	void Render() override;
};