#pragma once
#include "EditorWindow.h"
#include "Model.h"
#include "ImGuizmo.h"
#include "EditorComponent.h"

#include "Transform.h"

class EditorTransform : public EditorComponent<Transform>
{
public:
	const char* getName() const override { return "Transform"; }
	void		render() override;
};