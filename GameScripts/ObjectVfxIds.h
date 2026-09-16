#pragma once

#include "AssetId.h"
#include "AssetType.h"

namespace ObjectVfxIds
{
    // Assets/Prefabs/Particles/VFXRemake/Objects/Cristals/PS_Cristals.prefab
    inline AssetId crystalActiveEffect()
    {
        return AssetId(6448028941363253388ULL, "0f6088eddd09221af406ea8c64485088", AssetType::PREFAB);
    }

    // Assets/Prefabs/Particles/VFXRemake/Objects/Doors_Barricades/PS_BarricadesMist.prefab
    inline AssetId barricadeMist()
    {
        return AssetId(15273449107757228221ULL, "fa43c1d628ba49431efb586779ad07af", AssetType::PREFAB);
    }

    // Assets/Prefabs/Particles/VFXRemake/Objects/Doors_Barricades/PS_BarricadesParticles.prefab
    inline AssetId barricadeBurst()
    {
        return AssetId(11466636781170600960ULL, "8db70634adecb71377d75c889814c250", AssetType::PREFAB);
    }

    // Assets/Prefabs/Particles/VFXRemake/Objects/Spectral Path/PS_SpectralPathEntrances.prefab
    inline AssetId spectralPathEntrance()
    {
        return AssetId(3123702398107190830ULL, "75d537ddb341f71d5572ff9faee0a0a0", AssetType::PREFAB);
    }

    // Assets/Prefabs/Particles/VFXRemake/Objects/Barrels/Ps_BarrelBase.prefab
    inline AssetId barrelBreakBase()
    {
        return AssetId(3095189587158284560ULL, "b899d86ac09257d064490a791dbabf95", AssetType::PREFAB);
    }

    // Assets/Prefabs/Particles/VFXRemake/Objects/Barrels/PS_BarrelExplosion.prefab
    inline AssetId barrelExplosion()
    {
        return AssetId(4420715343403901024ULL, "2650125feba2f6228bd9a40f76a50d42", AssetType::PREFAB);
    }

    // Assets/Prefabs/Particles/VFXRemake/Objects/Barrels/PS_BarrelsHeal.prefab
    inline AssetId barrelHeal()
    {
        return AssetId(6369742607687498240ULL, "3bcfb6a2be9f3f11866851b15bde4c97", AssetType::PREFAB);
    }
}
