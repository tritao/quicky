#include "quiky/sfx_events.h"

#include <cassert>
#include <iostream>

namespace {

quiky::LevelEvent collected(quiky::GameplayCollectible collectible) {
    quiky::LevelEvent event;
    event.type = quiky::LevelEventType::Collected;
    event.entityType = static_cast<std::uint16_t>(collectible);
    return event;
}

void testConfirmedMappings() {
    assert(quiky::gameplaySfxForEvent(
               collected(quiky::GameplayCollectible::TenAmmoBox)) ==
           quiky::GameplaySfx::AmmoOrHealthPackagePickup);
    assert(quiky::gameplaySfxForEvent(
               collected(quiky::GameplayCollectible::ExtraHealthPackage)) ==
           quiky::GameplaySfx::AmmoOrHealthPackagePickup);
    assert(quiky::gameplaySfxForEvent(
               collected(quiky::GameplayCollectible::HealthUpgrade)) ==
           quiky::GameplaySfx::HealthUpgradePickup);
    assert(quiky::gameplaySfxForEvent(
               collected(quiky::GameplayCollectible::TemporaryInvulnerability)) ==
           quiky::GameplaySfx::TemporaryInvulnerabilityPickup);

    const quiky::GameplayCollectible letters[] = {
        quiky::GameplayCollectible::PuzzleLetterN,
        quiky::GameplayCollectible::PuzzleLetterE,
        quiky::GameplayCollectible::PuzzleLetterS,
        quiky::GameplayCollectible::PuzzleLetterQ,
        quiky::GameplayCollectible::PuzzleLetterU,
        quiky::GameplayCollectible::PuzzleLetterI,
        quiky::GameplayCollectible::PuzzleLetterK,
    };
    for (std::size_t index = 0; index < sizeof(letters) / sizeof(letters[0]); ++index) {
        assert(quiky::gameplaySfxForEvent(collected(letters[index])) ==
               quiky::GameplaySfx::PuzzleLetterPickup);
    }
}

void testSilentMappings() {
    quiky::LevelEvent event;
    assert(quiky::gameplaySfxForEvent(event) == quiky::GameplaySfx::Silent);

    event.type = quiky::LevelEventType::PlayerJumped;
    assert(quiky::gameplaySfxForEvent(event) == quiky::GameplaySfx::PlayerJump);
    event.type = quiky::LevelEventType::EntityCollisionImpact;
    assert(quiky::gameplaySfxForEvent(event) ==
           quiky::GameplaySfx::EntityCollisionImpact);
    event.type = quiky::LevelEventType::TileInteraction;
    assert(quiky::gameplaySfxForEvent(event) ==
           quiky::GameplaySfx::TileInteractionEffect);
    event.type = quiky::LevelEventType::AlternateActionObject;
    assert(quiky::gameplaySfxForEvent(event) ==
           quiky::GameplaySfx::AlternateActionObject);
    event.type = quiky::LevelEventType::PooledObjectInteractionBurst;
    assert(quiky::gameplaySfxForEvent(event) ==
           quiky::GameplaySfx::PooledObjectInteractionBurst);
    event.type = quiky::LevelEventType::WorldObjectInteraction;
    assert(quiky::gameplaySfxForEvent(event) ==
           quiky::GameplaySfx::WorldObjectInteraction);

    event.type = quiky::LevelEventType::PlayerDied;
    assert(quiky::gameplaySfxForEvent(event) ==
           quiky::GameplaySfx::PlayerDeathSequence);
    event.type = quiky::LevelEventType::LevelExit;
    assert(quiky::gameplaySfxForEvent(event) == quiky::GameplaySfx::Silent);

    event.type = quiky::LevelEventType::Collected;
    event.entityType = 0x00;
    assert(quiky::gameplaySfxForEvent(event) == quiky::GameplaySfx::Silent);
    event.entityType = 0x73;
    assert(quiky::gameplaySfxForEvent(event) == quiky::GameplaySfx::Silent);
    event.entityType = 0x78;
    assert(quiky::gameplaySfxForEvent(event) == quiky::GameplaySfx::Silent);
    event.entityType = 0x80;
    assert(quiky::gameplaySfxForEvent(event) == quiky::GameplaySfx::Silent);
}

} // namespace

int main() {
    testConfirmedMappings();
    testSilentMappings();
    std::cout << "all quiky SFX event tests passed\n";
    return 0;
}
