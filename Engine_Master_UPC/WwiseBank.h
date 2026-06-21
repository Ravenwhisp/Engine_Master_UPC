#pragma once
#include <string>
#include <vector>
#include <cstdint>

#include "WwiseEvent.h"
#include "AssetReference.h"

using AkBankID = unsigned int;
using AkGameObjectID = unsigned long long;

class WwiseBank
{
private:
	std::string m_bankName;
	std::string m_jsonPath;

	AkBankID m_bankID = 0;

	std::vector<WwiseEvent> m_events;

	bool m_loaded;
	AssetReference m_assetRef;

	std::vector<uint8_t> m_bankData;

public:
	WwiseBank() = default;
	~WwiseBank() = default;

	bool init(const char* bankName, const char* jsonPath);
	void cleanUp();

	const bool isLoaded() const { return m_loaded; }
	bool load();
	void unload();

	const std::vector<WwiseEvent>& getEvents() const;
	const std::string& getName() const { return m_bankName; }

	bool postEvent(const char* eventName, AkGameObjectID gameObjectID) const;

	void setName(const std::string& name) { m_bankName = name; }
	void addEvent(const WwiseEvent& e) { m_events.push_back(e); }
	void setAssetRef(const AssetReference& ref) { m_assetRef = ref; }
	const AssetReference& getAssetRef() const { return m_assetRef; }
	void setBankData(const std::vector<uint8_t>& d) { m_bankData = d; }

private:
	bool loadEventsFromJson();
};
