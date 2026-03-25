#pragma once
#include "EditorWindow.h"

class GameObject;

//TODO: Handle more thinks like models, assets... Right now only the inspector works for the GameObjects
class WindowInspector: public EditorWindow
{
public:
	WindowInspector();
	const char* getWindowName() const override { return "WindowInspector"; }
	void		drawInternal() override;

};

