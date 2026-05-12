#pragma once
#include "Module.h"

class ModuleInput;

class ModuleMusic : public Module
{
private:
	ModuleInput* m_moduleInput = nullptr;

public:
	ModuleMusic();
	~ModuleMusic();

	bool init() override;
	void update() override;
	bool cleanUp() override;
};