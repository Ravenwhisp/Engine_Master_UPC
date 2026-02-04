#pragma once
#include "EditorWindow.h"

class GameObject;
class EditorTransform;
class EditorMeshRenderer;
class LightEditor;

//TODO: Handle more thinks like models, assets... Right now only the inspector works for the GameObjects
class Inspector: public EditorWindow
{
public:
	Inspector();
	const char* GetWindowName() const override { return "Inspector"; }
	void Render() override;
	void SetSelectedGameObject(GameObject* gameObject);

private:
	GameObject* _selectedGameObject;

	//PROVISIONAL
	EditorTransform* editorTransform;
	EditorMeshRenderer* editorMeshRenderer;
	LightEditor* editorLight;
};

