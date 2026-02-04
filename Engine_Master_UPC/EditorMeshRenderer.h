#pragma once
#include "EditorComponent.h"
#include "Model.h"

namespace Emeika { class Model; }

class EditorMeshRenderer: public EditorComponent<Emeika::Model>
{
public:
	const char* GetName() const override { return "Mesh Renderer"; }
	void Render() override;
private:
};

