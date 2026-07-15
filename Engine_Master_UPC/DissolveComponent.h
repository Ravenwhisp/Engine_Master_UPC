#pragma once
#include "Component.h"
#include <TextureAsset.h>

class Texture;

struct DissolveData
{
	float dissolveAmount = 0;
	Vector3 dissolveColor = Vector3::One;
	float dissolveThickness = 1;
};

struct DissolveCB
{
	UINT hasDissolveComponent = 0;
	Vector3 padding1 = Vector3::Zero;

	DissolveData dissolveData;
	Vector3 padding2 = Vector3::Zero;
};



class DissolveComponent : public Component
{
public:
	DissolveComponent(UID id, GameObject* owner);
	virtual std::unique_ptr<Component> clone(GameObject* newOwner) const override;

	void drawUi() override;
	void onTransformChange() override {}

	void serialize(IArchive& archive) override;

	void debugDraw() override {}

	void	setDissolveAmount(float value)   { m_dissolveData.dissolveAmount = value; }
	float	getDissolveAmount()              { return m_dissolveData.dissolveAmount; }
	void	setDissolveColor(Vector3 value)  { m_dissolveData.dissolveColor = value; }
	Vector3 getDissolveColor()				 { return m_dissolveData.dissolveColor; }
	void	setDissolveThikness(float value) { m_dissolveData.dissolveThickness = value; }
	float   getDissolveThikness()			 { return m_dissolveData.dissolveThickness; }

	DissolveData getDissolveData() { return m_dissolveData; }

	void requestLoad() { m_loadRequested = true; }

	Texture*			  getTexture()			       const { return m_texture.get(); }
	const AssetReference& getTextureAssetId()          const { return m_textureAssetId; }
	TextureAsset*		  getTextureAsset()            const { return m_textureAsset.get(); }
	
	bool consumeLoadRequest();
	void setTextureAssetId(const AssetReference& assetId);


private:
	DissolveData m_dissolveData{};

	AssetReference m_textureAssetId{};
	std::shared_ptr<Texture> m_texture = nullptr;
	std::shared_ptr<TextureAsset> m_textureAsset = nullptr;
	bool m_loadRequested = false;

	void LoadTexture(UID* data);
};

