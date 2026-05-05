#include "Globals.h"
#include "ParticleSystemModule.h"

ParticleEmitter* ParticleSystemModule::addEmitter(Transform* parent)
{
	m_emitters.push_back(std::make_unique<ParticleEmitter>(parent));
	return m_emitters.back().get();
}

bool ParticleSystemModule::removeEmitter(ParticleEmitter* emitter)
{
	for (unsigned int i = 0; i < m_emitters.size(); ++i)
	{
		if (m_emitters[i].get() == emitter)
		{
			m_emitters.erase(m_emitters.begin() + i);
			return true;
		}
	}

    return false;
}
