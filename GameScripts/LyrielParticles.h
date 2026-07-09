#pragma once
#include <Script.h>
#include "ScriptAPI.h"


class LyrielParticles : public Script
{
	DECLARE_SCRIPT(LyrielParticles)
public:

	explicit LyrielParticles(GameObject* owner);

	ComponentRef<Transform> m_dashTrail;

	Transform* m_dashTrailController = nullptr;

	FieldList getExposedFields() const override;

	void SetDashActive();
	void SetDashInactive();

private:

	Transform* getTransform(ComponentRef<Transform> controller);

};