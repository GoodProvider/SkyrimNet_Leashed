#pragma once

namespace SkyrimNetLeashed::SkyrimNet {
    // Returns false when SkyrimNet is missing or decorator registration is unavailable.
    [[nodiscard]] bool Register();
}
