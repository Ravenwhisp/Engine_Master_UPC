#pragma once
#include "EditorComponent.h"
#include "Light.h"

class LightEditor: public EditorComponent<Light>
{
public:
	const char* getName() const override { return "Light"; }
	void		render() override;
};

