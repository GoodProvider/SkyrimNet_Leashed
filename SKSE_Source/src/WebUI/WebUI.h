#pragma once

#include "PCH.h"

#include <string_view>

namespace SkyrimNetLeashed::WebUI {
    void Init();
    void SetGameReady();
    void Open();
    void OpenFor(RE::Actor* a_leashed, std::string_view a_layout, std::string_view a_verb);
}
