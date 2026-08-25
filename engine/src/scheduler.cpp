#include "quiky/scheduler.h"

namespace quiky {

SchedulerObject::SchedulerObject()
    : active(false),
      pendingSpawn(false),
      pendingRelease(false),
      cameraParticipating(false),
      playerCallback(false),
      generation(0),
      sourceId(0),
      callback() {
}

ObjectScheduler::ObjectScheduler(std::size_t capacity)
    : _objects(capacity), _invocations(), _pendingMutations() {
}

void ObjectScheduler::reset() {
    for (std::size_t index = 0; index < _objects.size(); ++index) {
        _objects[index] = SchedulerObject();
    }
    _invocations.clear();
    _pendingMutations.clear();
}

SchedulerHandle ObjectScheduler::queueSpawn(const CallbackIdentity &callback,
                                            std::uint32_t sourceId,
                                            bool playerCallback) {
    for (std::size_t index = 0; index < _objects.size(); ++index) {
        SchedulerObject &object = _objects[index];
        if (!object.active && !object.pendingSpawn) {
            object.pendingSpawn = true;
            object.pendingRelease = false;
            object.cameraParticipating = true;
            object.playerCallback = playerCallback;
            object.sourceId = sourceId;
            object.callback = callback;
            ++object.generation;
            if (object.generation == 0) {
                ++object.generation;
            }
            _pendingMutations.push_back(SchedulerMutation(
                SchedulerMutationKind::Spawn,
                SchedulerHandle(static_cast<std::uint16_t>(index),
                                object.generation)));
            return SchedulerHandle(static_cast<std::uint16_t>(index),
                                   object.generation);
        }
    }
    return SchedulerHandle();
}

void ObjectScheduler::queueRelease(SchedulerHandle handle) {
    if (owns(handle)) {
        _objects[handle.slot].pendingRelease = true;
        _pendingMutations.push_back(
            SchedulerMutation(SchedulerMutationKind::Release, handle));
    }
}

void ObjectScheduler::setCameraParticipation(SchedulerHandle handle,
                                              bool participating) {
    if (owns(handle)) {
        _objects[handle.slot].cameraParticipating = participating;
    }
}

void ObjectScheduler::setPlayerCallback(SchedulerHandle handle,
                                         bool playerCallback) {
    if (owns(handle)) {
        _objects[handle.slot].playerCallback = playerCallback;
    }
}

void ObjectScheduler::beginTick(std::uint64_t tick) {
    applyPendingMutations();
    _invocations.clear();

    for (std::size_t index = 0; index < _objects.size(); ++index) {
        const SchedulerObject &object = _objects[index];
        if (!object.active ||
            (!object.playerCallback && !object.cameraParticipating)) {
            continue;
        }

        SchedulerInvocation invocation;
        invocation.tick = tick;
        invocation.slot = static_cast<std::uint16_t>(index);
        invocation.generation = object.generation;
        invocation.playerCallback = object.playerCallback;
        invocation.cameraParticipating = object.cameraParticipating;
        invocation.callback = object.callback;
        _invocations.push_back(invocation);
    }
}

std::size_t ObjectScheduler::activeCount() const {
    std::size_t count = 0;
    for (std::size_t index = 0; index < _objects.size(); ++index) {
        if (_objects[index].active) {
            ++count;
        }
    }
    return count;
}

bool ObjectScheduler::owns(SchedulerHandle handle) const {
    return handle.valid() && handle.slot < _objects.size() &&
           _objects[handle.slot].generation == handle.generation &&
           (_objects[handle.slot].active ||
            _objects[handle.slot].pendingSpawn);
}

void ObjectScheduler::applyPendingMutations() {
    for (std::size_t index = 0; index < _pendingMutations.size(); ++index) {
        const SchedulerMutation &mutation = _pendingMutations[index];
        if (!owns(mutation.handle)) {
            continue;
        }
        SchedulerObject &object = _objects[mutation.handle.slot];
        if (mutation.kind == SchedulerMutationKind::Spawn) {
            object.pendingSpawn = false;
            object.active = true;
        } else {
            object.active = false;
            object.pendingSpawn = false;
            object.pendingRelease = false;
            object.cameraParticipating = false;
            object.playerCallback = false;
            object.sourceId = 0;
            object.callback = CallbackIdentity();
        }
    }
    _pendingMutations.clear();
}

} // namespace quiky
