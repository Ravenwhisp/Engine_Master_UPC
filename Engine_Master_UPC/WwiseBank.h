#pragma once
#include <string>
#include <vector>

#include "WwiseEvent.h"

using AkBankID = unsigned int;
using AkGameObjectID = unsigned long long;

class WwiseBank
{
private:
	std::string m_bankName;
	std::string m_jsonPath;

	AkBankID m_bankID = 0;

	std::vector<WwiseEvent> m_events;

public:
	WwiseBank() = default;
	~WwiseBank() = default;

	bool init(const char* bankName, const char* jsonPath);
	void cleanUp();

	const std::vector<WwiseEvent>& getEvents() const;
	const std::string& getName() const { return m_bankName; }

	bool postEvent(const char* eventName, AkGameObjectID gameObjectID) const;

private:
	bool load();
	void unload();

	void loadEventsFromJson();
};