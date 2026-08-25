#ifndef QUIKY_RUNTIME_H
#define QUIKY_RUNTIME_H

#include "quiky/map.h"

#include <array>
#include <cstdint>

namespace quiky {

struct Fixed16 {
    static const std::int32_t kOne = 1 << 16;

    std::int32_t raw;

    Fixed16() : raw(0) {}
    explicit Fixed16(std::int32_t rawValue) : raw(rawValue) {}

    static Fixed16 fromPixels(std::int32_t pixels) {
        return Fixed16(pixels * kOne);
    }

    std::int32_t floorPixels() const;
};

struct InputState {
    bool left;
    bool right;
    bool up;
    bool down;
    bool jump;
    bool alternate;

    InputState()
        : left(false), right(false), up(false), down(false), jump(false),
          alternate(false) {}

    static InputState fromActionFlags(std::uint16_t flags);
};

struct PlayerConfig {
    std::int32_t width;
    std::int32_t height;
    std::int32_t acceleration;
    std::int32_t maxHorizontalSpeed;
    std::int32_t friction;
    std::int32_t gravity;
    std::int32_t jumpVelocity;

    PlayerConfig();
};

struct CollisionRules {
    // These masks are evidence-guided defaults, not a completed collision map.
    // MAP property B was tentatively associated with side/solid behavior and D
    // with head impacts in the existing research notes.
    std::uint16_t horizontalMask;
    std::uint16_t floorMask;
    std::uint16_t ceilingMask;
    bool outsideIsSolid;

    CollisionRules();
};

// Rules recovered from the protected-mode 5C27/3D02 probes.  This is kept
// separate from CollisionRules because the current MAP-backed simulation does
// not yet carry the executable's descriptor table alongside each cell.
struct PlayerDescriptorRules {
    static const std::size_t kEntryCount = 512;

    static std::uint16_t quadrantMask(std::uint16_t x, std::uint16_t y);
    static bool blocksProbe(std::uint16_t descriptor,
                            std::uint16_t x, std::uint16_t y);
    static bool hasVerticalResponse(std::uint16_t descriptor);
    static bool alignsEightPixels(std::uint16_t descriptor);
    static std::uint16_t snapProbeY(std::uint16_t y);
};

// The executable keeps collision descriptors in a runtime table separate
// from the upper seven MAP-cell property bits.  The table is deliberately
// supplied by the caller: its bytes come from the loaded game's descriptor
// segment, not from the MAP archive payload.
class PlayerDescriptorTable {
public:
    PlayerDescriptorTable();
    explicit PlayerDescriptorTable(
        const std::array<std::uint16_t, PlayerDescriptorRules::kEntryCount> &words);

    std::uint16_t word(std::uint16_t tileId) const;
    void setWord(std::uint16_t tileId, std::uint16_t descriptor);

private:
    std::array<std::uint16_t, PlayerDescriptorRules::kEntryCount> _words;
};

// Resolves a streamed MAP cell through the descriptor table.  This is a
// read-only bridge for the recovered player helpers; it does not yet claim
// that every descriptor flag has a final floor/ceiling gameplay name.
class MapDescriptorQuery {
public:
    MapDescriptorQuery(const Map &map, const PlayerDescriptorTable &descriptors);

    std::uint16_t tileIdAt(std::int32_t tileX, std::int32_t tileY) const;
    std::uint16_t descriptorAt(std::int32_t tileX, std::int32_t tileY) const;
    std::uint16_t descriptorAtPixel(std::int32_t x, std::int32_t y) const;
    bool blocksProbeAt(std::int32_t x, std::int32_t y) const;
    bool hasVerticalResponseAt(std::int32_t x, std::int32_t y) const;
    bool alignsEightPixelsAt(std::int32_t x, std::int32_t y) const;

private:
    static std::int32_t floorTile(std::int32_t pixels);

    const Map &_map;
    const PlayerDescriptorTable &_descriptors;
};

// The executable's leaf collision helpers consume pixel probes and descriptor
// flags, not only tile coordinates. This optional interface preserves that
// distinction while keeping the older synthetic CollisionQuery useful for
// engine tests and tools.
class PlayerProbeQuery {
public:
    virtual ~PlayerProbeQuery() {}

    virtual bool blocksProbeAt(std::int32_t x, std::int32_t y) const = 0;
    virtual bool hasVerticalResponseAt(std::int32_t x,
                                       std::int32_t y) const = 0;
    virtual bool alignsEightPixelsAt(std::int32_t x,
                                     std::int32_t y) const = 0;
};

// The player callback asks directional collision helpers about the current
// object rather than consuming a MAP object directly. Keeping that boundary
// explicit lets refined tile semantics replace the current masks without
// changing player movement code.
class CollisionQuery {
public:
    virtual ~CollisionQuery() {}

    virtual bool blocksHorizontal(std::int32_t tileX,
                                  std::int32_t tileY) const = 0;
    virtual bool blocksFloor(std::int32_t tileX,
                             std::int32_t tileY) const = 0;
    virtual bool blocksCeiling(std::int32_t tileX,
                               std::int32_t tileY) const = 0;
};

class MapDescriptorCollisionQuery : public CollisionQuery,
                                     public PlayerProbeQuery {
public:
    MapDescriptorCollisionQuery(const Map &map,
                                const PlayerDescriptorTable &descriptors);

    bool blocksHorizontal(std::int32_t tileX,
                          std::int32_t tileY) const override;
    bool blocksFloor(std::int32_t tileX,
                     std::int32_t tileY) const override;
    bool blocksCeiling(std::int32_t tileX,
                       std::int32_t tileY) const override;

    bool blocksProbeAt(std::int32_t x, std::int32_t y) const override;
    bool hasVerticalResponseAt(std::int32_t x,
                               std::int32_t y) const override;
    bool alignsEightPixelsAt(std::int32_t x,
                             std::int32_t y) const override;

private:
    MapDescriptorQuery _query;
};

class MapCollisionQuery : public CollisionQuery {
public:
    MapCollisionQuery(const Map &map, const CollisionRules &rules);

    bool blocksHorizontal(std::int32_t tileX,
                          std::int32_t tileY) const override;
    bool blocksFloor(std::int32_t tileX,
                     std::int32_t tileY) const override;
    bool blocksCeiling(std::int32_t tileX,
                       std::int32_t tileY) const override;

private:
    bool propertyMatches(std::int32_t tileX, std::int32_t tileY,
                         std::uint16_t mask) const;

    const Map &_map;
    CollisionRules _rules;
};

struct PlayerState {
    Fixed16 x;
    Fixed16 y;
    Fixed16 velocityX;
    Fixed16 velocityY;
    bool grounded;
    bool facingRight;

    // Callback-record state retained by the faithful movement core. The
    // names mirror recovered record offsets; grounded/facingRight remain
    // compatibility projections for the engine frontend.
    std::int8_t callbackMode;
    std::uint8_t callbackGate;
    std::uint8_t transition;
    std::uint8_t verticalResponse;
    std::uint8_t sideResponse;
    std::uint16_t deathTimer;

    PlayerState();
};

struct SpawnPoint {
    std::int32_t x;
    std::int32_t y;

    SpawnPoint(std::int32_t xValue = 0, std::int32_t yValue = 0)
        : x(xValue), y(yValue) {}
};

class PlayerSimulation {
public:
    PlayerSimulation(const PlayerConfig &config = PlayerConfig(),
                     const CollisionRules &collision = CollisionRules());

    void reset(PlayerState &player, std::int32_t x, std::int32_t y) const;
    void tick(PlayerState &player, const CollisionQuery &collision,
              const InputState &input) const;
    void tick(PlayerState &player, const Map &map, const InputState &input) const;
    void tick(PlayerState &player, const Map &map,
              const PlayerDescriptorTable &descriptors,
              const InputState &input) const;

    const PlayerConfig &config() const { return _config; }
    const CollisionRules &collisionRules() const { return _collision; }

private:
    bool collidesHorizontal(const CollisionQuery &collision,
                            const PlayerState &player) const;
    bool collidesFloor(const CollisionQuery &collision,
                       const PlayerState &player) const;
    bool collidesCeiling(const CollisionQuery &collision,
                         const PlayerState &player) const;
    void moveHorizontal(PlayerState &player,
                        const CollisionQuery &collision) const;
    void moveVertical(PlayerState &player,
                      const CollisionQuery &collision) const;

    PlayerConfig _config;
    CollisionRules _collision;
};

} // namespace quiky

#endif
