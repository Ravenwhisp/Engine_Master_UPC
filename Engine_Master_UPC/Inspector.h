#pragma once
#include "EditorWindow.h"

class GameObject;

//TODO: Handle more thinks like models, assets... Right now only the inspector works for the GameObjects
class Inspector: public EditorWindow
{
public:
	Inspector();
	const char* getWindowName() const override { return "Inspector"; }
	void		render() override;

};

