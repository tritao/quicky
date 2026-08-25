#include "quiky/sfx_events.h"

namespace quiky {

GameplaySfx gameplaySfxForEvent(const LevelEvent &event) {
    if (event.type == LevelEventType::PlayerJumped) {
        return GameplaySfx::PlayerJump;
    }
    if (event.type == LevelEventType::EntityCollisionImpact) {
        return GameplaySfx::EntityCollisionImpact;
    }
    if (event.type == LevelEventType::TileInteraction) {
        return GameplaySfx::TileInteractionEffect;
    }
    if (event.type == LevelEventType::AlternateActionObject) {
        return GameplaySfx::AlternateActionObject;
    }
    if (event.type == LevelEventType::PooledObjectInteractionBurst) {
        return GameplaySfx::PooledObjectInteractionBurst;
    }
    if (event.type == LevelEventType::WorldObjectInteraction) {
        return GameplaySfx::WorldObjectInteraction;
    }
    if (event.type == LevelEventType::PlayerDied) {
        return GameplaySfx::PlayerDeathSequence;
    }
    if (event.type != LevelEventType::Collected) {
        return GameplaySfx::Silent;
    }

    switch (static_cast<GameplayCollectible>(event.entityType)) {
    case GameplayCollectible::TenAmmoBox:
    case GameplayCollectible::ExtraHealthPackage:
        return GameplaySfx::AmmoOrHealthPackagePickup;
    case GameplayCollectible::HealthUpgrade:
        return GameplaySfx::HealthUpgradePickup;
    case GameplayCollectible::TemporaryInvulnerability:
        return GameplaySfx::TemporaryInvulnerabilityPickup;
    case GameplayCollectible::PuzzleLetterN:
    case GameplayCollectible::PuzzleLetterE:
    case GameplayCollectible::PuzzleLetterS:
    case GameplayCollectible::PuzzleLetterQ:
    case GameplayCollectible::PuzzleLetterU:
    case GameplayCollectible::PuzzleLetterI:
    case GameplayCollectible::PuzzleLetterK:
        return GameplaySfx::PuzzleLetterPickup;
    default:
        return GameplaySfx::Silent;
    }
}

} // namespace quiky
