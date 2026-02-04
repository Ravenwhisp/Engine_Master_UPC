#pragma once
#include "EditorComponent.h"
#include "Light.h"

class LightEditor: public EditorComponent<Light>
{
public:
	const char* GetName() const override { return "Light"; }

	void Render() override;
};

