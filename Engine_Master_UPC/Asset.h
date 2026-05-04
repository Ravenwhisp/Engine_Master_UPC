#pragma once

#include "AssetReference.h"
#include "AssetsDictionary.h"
#include "AssetType.h"

class Asset
{
public:
	Asset() = default;
	Asset(AssetReference& id, AssetType type = AssetType::UNKNOWN): m_reference(id), m_type(type) {}
	virtual ~Asset() = default;
	AssetReference getReference() const { return m_reference; }

	UID getUID() const { return m_reference.m_uid; }
	void setUID(const UID& uid) { m_reference.m_uid = uid; }
	MD5Hash getLibId() const { return m_reference.m_libId; }
	void setLibId(const MD5Hash& libId) { m_reference.m_libId = libId; }

	AssetType getType() const { return m_type; }

	virtual void drawUI();
protected:
	AssetReference 				m_reference;
	AssetType			m_type = AssetType::UNKNOWN;
};
