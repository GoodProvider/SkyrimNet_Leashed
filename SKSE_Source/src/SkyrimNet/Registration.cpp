#include "Registration.h"

#include "SkyrimNet/PublicAPI.h"

#include "../Leash/LeashConstants.h"
#include "../Leash/PapyrusCalls.h"
#include "../Leash/VisiblePairs.h"

namespace SkyrimNetLeash::SkyrimNet {
    namespace {
        void RegisterDecorator(const char* a_name, const char* a_description, std::function<std::string(RE::Actor*)> a_callback) {
            if (!PublicRegisterDecorator) {
                SKSE::log::error("PublicRegisterDecorator is null; cannot register '{}'", a_name);
                return;
            }
            if (PublicRegisterDecorator(a_name, a_description, std::move(a_callback))) {
                SKSE::log::info("Registered decorator '{}'", a_name);
            } else {
                SKSE::log::error("Failed to register decorator '{}' (name conflict or null callback)", a_name);
            }
        }

        std::string Availability(bool a_ok) { return a_ok ? "available" : "unavailable"; }
    }

    bool Register() {
        if (!FindFunctions()) {
            SKSE::log::error("SkyrimNet FindFunctions failed; decorators will not register");
            return false;
        }
        if (!PublicRegisterDecorator) {
            SKSE::log::warn("SkyrimNet PublicRegisterDecorator is null (v5+ required)");
            return false;
        }

        RegisterDecorator(
            "is_leash_available",
            "Returns 'available' if this actor can start a leash (alive, not in combat, Leash.esm loaded). Children may be leashed. Returns 'unavailable' otherwise.",
            [](RE::Actor* actor) -> std::string {
                if (!actor) {
                    return "unavailable";
                }
                const char* name = actor->GetName();
                if (!IsLeashPluginLoaded()) {
                    SKSE::log::debug("is_leash_available: '{}' (0x{:08X}) -> unavailable (Leash.esm missing)", name ? name : "(unnamed)", actor->GetFormID());
                    return "unavailable";
                }
                if (actor->IsDead()) {
                    SKSE::log::debug("is_leash_available: '{}' (0x{:08X}) -> unavailable (dead)", name ? name : "(unnamed)", actor->GetFormID());
                    return "unavailable";
                }
                if (actor->IsInCombat()) {
                    SKSE::log::debug("is_leash_available: '{}' (0x{:08X}) -> unavailable (in combat)", name ? name : "(unnamed)", actor->GetFormID());
                    return "unavailable";
                }
                return "available";
            });

        RegisterDecorator(
            "is_unleash_available",
            "Returns 'available' if this actor is in LeasherFaction or LeashedFaction. Returns 'unavailable' otherwise.",
            [](RE::Actor* actor) -> std::string {
                if (!actor) {
                    return "unavailable";
                }
                const bool ok = Papyrus::IsLeashHolder(actor) || Papyrus::IsLeashed(actor);
                if (!ok) {
                    const char* name = actor->GetName();
                    SKSE::log::debug("is_unleash_available: '{}' (0x{:08X}) -> unavailable (not in leash factions)", name ? name : "(unnamed)", actor->GetFormID());
                }
                return Availability(ok);
            });

        RegisterDecorator(
            "leashframework_visible_pairs",
            "JSON object with a pairs array of holder/leashed display names visible to this speaker.",
            [](RE::Actor* actor) -> std::string { return VisiblePairs::JsonForSpeaker(actor); });

        return true;
    }
}
