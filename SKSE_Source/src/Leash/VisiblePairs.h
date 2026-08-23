#pragma once

#include "PCH.h"

namespace SkyrimNetLeash::VisiblePairs {
    void Start();
    void Refresh();
    [[nodiscard]] std::string JsonForSpeaker(RE::Actor* a_speaker);
}
