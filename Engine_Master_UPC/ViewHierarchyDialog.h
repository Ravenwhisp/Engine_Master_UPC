#pragma once
#include "EditorWindow.h"

class EditorModule;
class SceneModule;

class ViewHierarchyDialog : public EditorWindow
{
private:
	EditorModule* m_editorModule;
	SceneModule* m_sceneModule;

public:
	ViewHierarchyDialog();

	void		render() override;
	const char* getWindowName() const override { return "HierarchyDialog"; }

private:

};
