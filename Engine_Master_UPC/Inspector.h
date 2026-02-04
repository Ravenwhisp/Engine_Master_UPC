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
	const char* getWindowName() const override { return "Inspector"; }
	void		render() override;
	void		setSelectedGameObject(GameObject* gameObject);

private:
	GameObject* m_selectedGameObject;

	//PROVISIONAL
	EditorTransform* m_editorTransform;
	EditorMeshRenderer* m_editorMeshRenderer;
	LightEditor* m_editorLight;
};

