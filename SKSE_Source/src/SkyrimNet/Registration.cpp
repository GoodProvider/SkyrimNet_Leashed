#include "Registration.h"

#include "Api.h"
#include "StateCache.h"

#include <string>

namespace SkyrimNetLeash::SkyrimNet {
    namespace {
        void RegisterFlag(const char* a_name, const char* a_description, StateCache::Flag a_flag) {
            Api::RegisterDecorator(a_name, a_description,
                [a_flag](RE::Actor* a_actor) -> std::string { return StateCache::Flagged(a_actor, a_flag) ? "available" : "unavailable"; });
        }

        void RegisterPayload(const char* a_name, const char* a_description, StateCache::Payload a_payload) {
            Api::RegisterDecorator(a_name, a_description, [a_payload](RE::Actor* a_actor) -> std::string { return StateCache::Json(a_actor, a_payload); });
        }
    }

    bool Register() {
        if (!Api::Initialize()) {
            return false;
        }

        RegisterFlag("is_leash_available", "Returns 'available' if this actor can start a leash (Leash.esm loaded, alive, not in combat). Returns 'unavailable' otherwise.",
            StateCache::Flag::LeashAvailable);

        RegisterFlag("is_unleash_available", "Returns 'available' if this actor is in LeasherFaction or LeashedFaction. Returns 'unavailable' otherwise.",
            StateCache::Flag::UnleashAvailable);

        RegisterFlag("speaker_on_leash", "Returns 'available' if this actor shares a leash with another nearby actor who can be named as an unleash target.",
            StateCache::Flag::SpeakerOnLeash);

        RegisterFlag("leashed_nearby", "Returns 'available' if another nearby actor is in LeasherFaction or LeashedFaction. Returns 'unavailable' otherwise.",
            StateCache::Flag::LeashedNearby);

        RegisterFlag("unleashed_nearby", "Returns 'available' if another nearby actor is not in LeashedFaction. Returns 'unavailable' otherwise.",
            StateCache::Flag::UnleashedNearby);

        RegisterFlag("speaker_is_leashed", "Returns 'available' if this actor is in LeashedFaction. Returns 'unavailable' otherwise.",
            StateCache::Flag::SpeakerIsLeashed);

        RegisterFlag("collared_nearby", "Returns 'available' if another nearby actor is in LeashedFaction. Holders who are not themselves leashed do not count. Returns 'unavailable' otherwise.",
            StateCache::Flag::CollaredNearby);

        RegisterPayload("get_nearby_unleashed_actors", "JSON object with actorIds and actorsNameString for nearby actors who are not currently leashed. Excludes the speaker.",
            StateCache::Payload::UnleashedActors);

        RegisterPayload("get_nearby_leashed_actors", "JSON object with actorIds and actorsNameString for nearby actors in LeasherFaction or LeashedFaction. Excludes the speaker.",
            StateCache::Payload::LeashedActors);

        RegisterPayload("get_speaker_leash_partners", "JSON object with actorIds and actorsNameString for the actors on the other end of this speaker's leashes.",
            StateCache::Payload::LeashPartners);

        RegisterPayload("leashframework_visible_pairs", "JSON object with a pairs array of holder/leashed display names visible to this speaker.",
            StateCache::Payload::VisiblePairs);

        RegisterPayload("get_nearby_collared_actors", "JSON object with actorIds and actorsNameString for nearby actors in LeashedFaction. Excludes the speaker and holder-only actors.",
            StateCache::Payload::CollaredActors);

        RegisterPayload("get_nearby_actors", "JSON object with actorIds and actorsNameString for nearby living actors. Excludes the speaker.",
            StateCache::Payload::NearbyActors);

        return true;
    }
}
