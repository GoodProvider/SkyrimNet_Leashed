#pragma once

namespace SkyrimNetLeash::SkyrimNet {
    // Returns false when SkyrimNet is missing or decorator registration is unavailable.
    [[nodiscard]] bool Register();
}
