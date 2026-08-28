#include "quiky/scene_renderer.h"

#include <map>
#include <sstream>

namespace quiky {

namespace {

const BobRecord *findSlot(const Bob &bob, std::uint16_t slot) {
    for (std::size_t index = 0; index < bob.records.size(); ++index) {
        if (bob.records[index].slot == slot) {
            return &bob.records[index];
        }
    }
    return 0;
}

std::uint16_t worldEffectTile(const std::string &worldName,
                              std::uint16_t animationFrame) {
    const std::uint16_t frame = static_cast<std::uint16_t>(animationFrame / 3);
    if (worldName == "W1" || worldName == "W2") {
        const std::uint16_t sequence[] = {127, 126, 128, 129, 130};
        return sequence[frame % 5];
    }
    if (worldName == "W3") {
        const std::uint16_t sequence[] = {401, 400, 402, 403, 404};
        return sequence[frame % 5];
    }
    if (worldName == "W4") {
        const std::uint16_t sequence[] = {241, 240, 242, 243, 244};
        return sequence[frame % 5];
    }
    if (worldName == "W5") {
        const std::uint16_t sequence[] = {62, 61, 63, 64};
        return sequence[frame % 4];
    }
    return 0xffff;
}

std::uint16_t dedicatedEffectSlot(const LevelEffect &effect,
                                  const std::string &worldName) {
    if (effect.sourceType == 0x65 && worldName == "W1") {
        return static_cast<std::uint16_t>(1 + (effect.animationFrame & 1));
    }
    return effect.effectSlot;
}

const BobRecord &choosePlayerFrame(const Bob &bob,
                                   const PlayerAnimation &animation) {
    const BobRecord *record = findSlot(bob, animation.slot());
    if (record == 0) {
        record = findSlot(bob, animation.slot() >= 50 ? 50 : 0);
    }
    if (record == 0) {
        throw FormatError("player BOB resource is missing the selected frame");
    }
    return *record;
}

} // namespace

const BobRecord &selectPlayerFrame(const LevelRuntime &runtime,
                                   const PlayerAnimation &animation) {
    return choosePlayerFrame(runtime.playerBob(), animation);
}

void drawLevelEntities(IndexedSurface &surface,
                       const LevelRuntime &runtime) {
    const LevelSession &level = runtime.session();
    const std::map<std::string, Bob> &resources = runtime.entityBobs();
    for (std::size_t index = 0; index < level.entities().size(); ++index) {
        const LevelEntity &entity = level.entities()[index];
        if (entity.phase != EntityPhase::Active) {
            continue;
        }

        if (!entity.spriteResource.empty() && entity.spriteSlot != 0xffff) {
            const std::map<std::string, Bob>::const_iterator resource =
                resources.find(entity.spriteResource);
            if (resource != resources.end()) {
                const std::uint16_t slot = renderSpriteSlot(entity);
                const BobRecord *record = findSlot(resource->second, slot);
                if (record == 0) {
                    std::ostringstream message;
                    message << entity.spriteResource
                            << " is missing entity sprite slot " << slot;
                    throw FormatError(message.str());
                }
                drawBobRecord(surface, *record, entity.x, entity.y);
            }
        }

        if (entity.effectResource == "WORLD") {
            const std::uint16_t tile = worldEffectTile(
                runtime.worldName(), entity.animationFrame);
            if (tile != 0xffff) {
                drawIcoTile(surface, runtime.tileset(), tile,
                            entity.x, entity.y);
            }
        }
    }
}

void drawTransientEffects(IndexedSurface &surface,
                          const LevelRuntime &runtime,
                          bool highEffectOnly) {
    const std::vector<LevelEffect> &effects = runtime.session().effects();
    const std::map<std::string, Bob> &effectBobs = runtime.effectBobs();
    for (std::size_t index = 0; index < effects.size(); ++index) {
        const LevelEffect &effect = effects[index];
        if (!effect.active) {
            continue;
        }
        if (highEffectOnly && effect.sourceEntityId != 0) {
            continue;
        }
        if (!effect.spriteResource.empty()) {
            const std::map<std::string, Bob>::const_iterator resource =
                effectBobs.find(effect.spriteResource);
            if (resource == effectBobs.end()) {
                throw FormatError("missing effect BOB resource: " +
                                  effect.spriteResource);
            }
            const BobRecord *record = findSlot(resource->second,
                                               effect.spriteSlot);
            if (record == 0) {
                std::ostringstream message;
                message << effect.spriteResource
                        << " is missing effect sprite slot "
                        << effect.spriteSlot;
                throw FormatError(message.str());
            }
            drawBobRecord(surface, *record, effect.x, effect.y);
        } else {
            const std::uint16_t slot = dedicatedEffectSlot(
                effect, runtime.worldName());
            drawIcoTile(surface, runtime.loopTileset(), slot,
                        effect.x, effect.y);
        }
    }
}

void drawPlayerSprite(IndexedSurface &surface, const LevelRuntime &runtime,
                      const PlayerAnimation &animation,
                      const PlayerRecord &player) {
    const BobRecord &record = selectPlayerFrame(runtime, animation);
    drawBobRecord(surface, record,
                  player.positionX.floorPixels(),
                  player.positionY.floorPixels());
}

} // namespace quiky
