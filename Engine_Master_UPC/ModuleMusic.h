#pragma once
#include "Module.h"

#include "WwiseBank.h"

#include <vector>

class ModuleMusic : public Module
{
private:
	std::vector<WwiseBank> m_banks;

public:
	ModuleMusic();
	~ModuleMusic();

	bool init() override;
	void update() override;
	bool cleanUp() override;

	const std::vector<WwiseBank>& getBankList() const { return m_banks; }

	void postEvent(const char* bankName, const char* eventName);

private:
	bool initWwise();
	void cleanUpWwise();

	bool loadBanksFromFolder();
};