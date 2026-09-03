#pragma once

#include "PCH.h"

#include <string>

// SkyrimNet invokes decorator callbacks on its own worker threads, so none of them may
// touch the game directly. Every value a decorator can return is computed here on the
// main thread and published as an immutable per-speaker snapshot.
namespace SkyrimNetLeash::SkyrimNet::StateCache {
    enum class Flag {
        LeashAvailable,
        UnleashAvailable,
        LeashedNearby,
        UnleashedNearby,
        SpeakerOnLeash,
        SpeakerIsLeashed,
        CollaredNearby,
    };

    enum class Payload {
        VisiblePairs,
        LeashedActors,
        UnleashedActors,
        LeashPartners,
        CollaredActors,
        NearbyActors,
    };

    void Start();
    void Stop();
    void Reset();

    // Main thread only.
    void Refresh();

    // Safe from any thread; both only read the published snapshot.
    [[nodiscard]] bool Flagged(RE::Actor* a_speaker, Flag a_flag);
    [[nodiscard]] std::string Json(RE::Actor* a_speaker, Payload a_payload);
}
