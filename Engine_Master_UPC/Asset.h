#pragma once
#include "AssetReference.h"
#include "ISerializable.h"
#include "UID.h"
#include "MD5Fwd.h"
#include "ImportSettings.h"
#include <memory>

class IArchive;

class Asset : public ISerializable
{
public:
    Asset() = default;
    Asset(AssetReference& id, AssetType type = AssetType::UNKNOWN) : m_reference(id), m_type(type) {}
    virtual ~Asset() = default;

    AssetReference getReference() const { return m_reference; }
    UID getUID() const { return m_reference.m_uid; }
    void setUID(const UID& uid) { m_reference.m_uid = uid; }
    MD5Hash getLibId() const { return m_reference.m_libId; }
    void setLibId(const MD5Hash& libId) { m_reference.m_libId = libId; }
    AssetType getType() const { return m_type; }

    virtual void drawUI();

    ImportSettings* getImportSettings() const { return m_importSettings.get(); }
    void setImportSettings(std::unique_ptr<ImportSettings> settings) { m_importSettings = std::move(settings); }
    virtual std::unique_ptr<ImportSettings> createDefaultImportSettings() const { return nullptr; }

    void serialize(IArchive& archive) override = 0;

protected:
    AssetReference m_reference;
    AssetType m_type = AssetType::UNKNOWN;
    std::unique_ptr<ImportSettings> m_importSettings;
};
