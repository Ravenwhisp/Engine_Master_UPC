#pragma once
#include "Module.h"

#include "ParticleSystem.h"
#include "ParticleCommands.h"
#include "AssetReference.h"

#include <array>

class ParticleSystemComponent;

struct Particle {

    Vector3 position; // CHANGE TO A TRANSFORM?
    Vector4 colorAndAlpha;
    float rotationZ = 0.f; // since they are billboards, other rotations don't make sense
    Vector2 scale;
    float textureFrame = 0.f; // for animation, the current frame in the texture

    float velocity;
    float rotationVelocity;
    bool flippedRotation;
    Vector3 movementDirection; // should be normalized
    float addedGravityVelocity;
    
    Vector2 startScale;  // for interpolation calculations
    Vector2 endScale;    //
    float startLifeTime; //

    float lifeTime = 0.f;

    EmitterInstance* owner;
	bool isNew; // => added to an emitter this frame, so it needs to be initialized
	unsigned int vectorPosition; // if isNew, it is from the m_newParticles vector, otherwise it is from the m_aliveParticles vector
};


// IN THE FUTURE, THE IDEA IS THAT IT HOLDS ALL PARTICLE SYSTEMS FOR EASY ACCESS (?)

class ModuleParticleSystem : public Module
{
public:

    bool init()     override;
    void preRender()  override;
    void update()     override;
    //void render()   override;
    bool cleanUp()  override;

    //ParticleSystem* addSystem(Transform* parent);
    //bool removeSystem(ParticleSystem* system);

    auto& getPool() { return m_pool; }

    // Slot management functions //
    int requestPoolSlot(EmitterInstance* newOwner); // returns a free pool slot, or an old used if no one is free (and assigns the specified owner to it)

    void freePoolSlot(unsigned int index); // frees the slot at the index (BUT DOES NOT CHANGE THE OTHER ARRAY!)

    void resetFirstUsedSlot(); // ONLY USE IF AT LEAST ONE SLOT FREE

    void updateOwnerData (unsigned int index, bool isNew, unsigned int vectorPosition); // (owner is only changed when requesting pool slot, so this is only for changes over time)


    void buildParticleCommands(ParticleSystemComponent* particleSystemComponent);

    const std::vector<ParticleEmitterCommand>& getParticleCommands() const { return m_particleCommands; }

    float deltaTime() const; // required to have more control on Editor mode

    void setTimeScale(float value) { m_timeScale = value; };
    float getTimeScale() const { return m_timeScale; };

    void resetAllParticles();

#ifdef _DEBUG
    void debugCheckDuplicateAliveIndices();
#endif

private:

    void initSlotManagement();
    Texture* resolveTexture(AssetId& textureRef);

    //std::vector<std::unique_ptr<ParticleSystem>> m_particleSystems;
    //std::vector<Transform*> m_parents;

    std::array<Particle, MAX_PARTICLES> m_pool;

    // Slot management data
    std::vector<unsigned int> m_freeSlots;
    unsigned int m_firstUsed; // approximation, for claiming if there are no free slots

    std::vector<ParticleEmitterCommand> m_particleCommands; // we will probably want to directly get the shader parameters in the future
    std::unordered_map<MD5Hash, std::shared_ptr<Texture>> m_particleTextures;

    float m_timeScale = 1.f; // for particle speed control
};

