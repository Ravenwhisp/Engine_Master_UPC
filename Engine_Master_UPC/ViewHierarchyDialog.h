#pragma once
#include "EditorWindow.h"

class EditorModule;
class SceneModule;

class Hierarchy;

class ViewHierarchyDialog : public EditorWindow
{
private:
	EditorModule* m_editorModule;
	SceneModule* m_sceneModule;

	Hierarchy* m_hierarchy;

public:
	ViewHierarchyDialog(Hierarchy* hierarchy);

	void		render() override;
	const char* getWindowName() const override { return "HierarchyDialog"; }

private:

};
