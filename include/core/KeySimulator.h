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

        // Diagnostics-capable API. The virtual key is translated to a hardware
        // scan code (MapVirtualKeyW) and injected with KEYEVENTF_SCANCODE so
        // targets that read physical key positions (Chrome KeyboardEvent.code,
        // games using raw input) see the same events as a real keyboard.
        // Returns false when the OS rejected the input or the key has no
        // scan-code mapping; getLastError() then carries the error code.
        static bool pressKey(int virtualKey);
        static bool releaseKey(int virtualKey);

        // Synthetic error: the virtual key has no scan-code mapping in the
        // active keyboard layout. SendInput is never called for such keys.
        static constexpr unsigned long NO_SCANCODE = 0xE0000001UL;

        static unsigned long getLastError() noexcept;
        static void clearLastError() noexcept;

        // Diagnostic/test hook: overwrite the stored platform error code so
        // unit tests can simulate an OS key-press rejection (e.g. UIPI).
        static void setLastError(unsigned long code) noexcept;

    private:
        static unsigned long lastError;
    };
}
