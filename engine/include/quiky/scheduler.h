#ifndef QUIKY_SCHEDULER_H
#define QUIKY_SCHEDULER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace quiky {

struct CallbackIdentity {
    std::uint16_t segment;
    std::uint16_t offset;
    std::string label;

    CallbackIdentity(std::uint16_t segmentValue = 0,
                     std::uint16_t offsetValue = 0,
                     const std::string &labelValue = std::string())
        : segment(segmentValue), offset(offsetValue), label(labelValue) {}
};

struct SchedulerHandle {
    std::uint16_t slot;
    std::uint16_t generation;

    SchedulerHandle(std::uint16_t slotValue = 0xffff,
                    std::uint16_t generationValue = 0)
        : slot(slotValue), generation(generationValue) {}

    bool valid() const { return slot != 0xffff; }
};

struct SchedulerObject {
    bool active;
    bool pendingSpawn;
    bool pendingRelease;
    bool cameraParticipating;
    bool playerCallback;
    std::uint16_t generation;
    std::uint32_t sourceId;
    CallbackIdentity callback;

    SchedulerObject();
};

struct SchedulerInvocation {
    std::uint64_t tick;
    std::uint16_t slot;
    std::uint16_t generation;
    bool playerCallback;
    bool cameraParticipating;
    CallbackIdentity callback;
};

enum class SchedulerMutationKind {
    Spawn,
    Release
};

struct SchedulerMutation {
    SchedulerMutationKind kind;
    SchedulerHandle handle;

    SchedulerMutation(SchedulerMutationKind kindValue,
                      SchedulerHandle handleValue)
        : kind(kindValue), handle(handleValue) {}
};

// Deterministic object-pool shell. Mutations requested during a tick are
// deferred until the next beginTick(), while dispatch always walks slots in
// ascending order. The shell records callback identity and order; it does not
// call gameplay code yet.
class ObjectScheduler {
public:
    explicit ObjectScheduler(std::size_t capacity = 64);

    void reset();
    SchedulerHandle queueSpawn(const CallbackIdentity &callback,
                               std::uint32_t sourceId = 0,
                               bool playerCallback = false);
    void queueRelease(SchedulerHandle handle);
    void setCameraParticipation(SchedulerHandle handle, bool participating);
    void setPlayerCallback(SchedulerHandle handle, bool playerCallback);

    void beginTick(std::uint64_t tick);

    const std::vector<SchedulerObject> &objects() const { return _objects; }
    const std::vector<SchedulerInvocation> &invocations() const {
        return _invocations;
    }
    std::size_t activeCount() const;

private:
    bool owns(SchedulerHandle handle) const;
    void applyPendingMutations();

    std::vector<SchedulerObject> _objects;
    std::vector<SchedulerInvocation> _invocations;
    std::vector<SchedulerMutation> _pendingMutations;
};

} // namespace quiky

#endif
