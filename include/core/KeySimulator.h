#pragma once

#include <string>

namespace Core {
    class KeySimulator {
    public:
        // Legacy API (kept unchanged for source compatibility)
        static void PressKey(int keyCode);
        static void ReleaseKey(int keyCode);

        // Convert a mapped key name ("A".."Z", "0".."9", " ") to a virtual-key
        // code. On Windows this is a VK_* code; on Linux it is an X11 KeySym.
        // Returns 0 when the name is unsupported.
        static int virtualKeyFromName(const std::string& keyName);

        // Diagnostics-capable API. Returns false when the OS rejected the
        // input; getLastError() then carries the platform error code.
        static bool pressKey(int virtualKey);
        static bool releaseKey(int virtualKey);

        static unsigned long getLastError() noexcept;
        static void clearLastError() noexcept;

    private:
        static unsigned long lastError;
    };
}
