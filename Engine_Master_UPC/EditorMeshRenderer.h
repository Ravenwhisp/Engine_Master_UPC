#pragma once
#include "EditorComponent.h"
#include "Model.h"

namespace Emeika { class Model; }

class EditorMeshRenderer: public EditorComponent<Emeika::Model>
{
public:
	const char* getName() const override { return "Mesh Renderer"; }
	void		render() override;
private:
};

