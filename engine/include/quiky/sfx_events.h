#ifndef QUIKY_SFX_EVENTS_H
#define QUIKY_SFX_EVENTS_H

#include "quiky/level.h"

#include <cstdint>

namespace quiky {

enum class GameplayCollectible : std::uint16_t {
    TenAmmoBox = 0x6f,
    ExtraHealthPackage = 0x70,
    HealthUpgrade = 0x71,
    TemporaryInvulnerability = 0x72,
    PuzzleLetterN = 0x79,
    PuzzleLetterE = 0x7a,
    PuzzleLetterS = 0x7b,
    PuzzleLetterQ = 0x7c,
    PuzzleLetterU = 0x7d,
    PuzzleLetterI = 0x7e,
    PuzzleLetterK = 0x7f,
};

enum class GameplaySfx : int {
    Silent = -1,
    PlayerJump = 0,
    PlayerDeathSequence = 1,
    EntityCollisionImpact = 4,
    TileInteractionEffect = 7,
    AlternateActionObject = 8,
    PooledObjectInteractionBurst = 2,
    AmmoOrHealthPackagePickup = 9,
    HealthUpgradePickup = 10,
    PuzzleLetterPickup = 11,
    TemporaryInvulnerabilityPickup = 12,
    WorldObjectInteraction = 13,
};

// Returns the named gameplay SFX role for an event. GameplaySfx::Silent marks
// events that intentionally have no sound yet.
GameplaySfx gameplaySfxForEvent(const LevelEvent &event);

} // namespace quiky

#endif
