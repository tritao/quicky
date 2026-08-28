#ifndef QUIKY_SCENE_RENDERER_H
#define QUIKY_SCENE_RENDERER_H

#include "quiky/level_runtime.h"
#include "quiky/player_animation.h"
#include "quiky/player_update.h"
#include "quiky/renderer.h"

namespace quiky {

// Draw shared world-space gameplay layers. Frontends remain responsible for
// map creation, camera composition, and presentation-specific overlays.
void drawLevelEntities(IndexedSurface &surface, const LevelRuntime &runtime);
void drawTransientEffects(IndexedSurface &surface,
                          const LevelRuntime &runtime,
                          bool highEffectOnly = false);
const BobRecord &selectPlayerFrame(const LevelRuntime &runtime,
                                   const PlayerAnimation &animation);
void drawPlayerSprite(IndexedSurface &surface, const LevelRuntime &runtime,
                      const PlayerAnimation &animation,
                      const PlayerRecord &player);

} // namespace quiky

#endif
