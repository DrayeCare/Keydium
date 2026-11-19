#include "core/KeySimulator.h"

#include <cctype>

#ifdef _WIN32
    #include <windows.h>
#elif __linux__
    #include <X11/Xlib.h>
    #include <X11/extensions/XTest.h>
    #include <X11/keysym.h>
    // Static display pointer agar tidak buka-tutup koneksi X11 terus menerus
    static Display* display = nullptr; 
#endif

namespace Core {

    unsigned long KeySimulator::lastError = 0;

    int KeySimulator::virtualKeyFromName(const std::string& keyName) {
        if (keyName.size() != 1) return 0;
        const unsigned char c = static_cast<unsigned char>(
            std::toupper(static_cast<unsigned char>(keyName[0])));

        // Uppercase ASCII letters and digits are identical to VK_A..VK_Z and
        // VK_0..VK_9 on Windows, and to XK_A..XK_Z / XK_0..XK_9 on X11.
        if (c >= 'A' && c <= 'Z') return static_cast<int>(c);
        if (c >= '0' && c <= '9') return static_cast<int>(c);
        if (c == ' ') return 0x20;
        return 0;
    }

    bool KeySimulator::pressKey(int virtualKey) {
        if (virtualKey <= 0) { lastError = 0; return false; }

        #ifdef _WIN32
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(virtualKey);
            if (SendInput(1, &input, sizeof(INPUT)) != 1) {
                lastError = static_cast<unsigned long>(GetLastError());
                return false;
            }
            lastError = 0;
            return true;
        #elif defined(__linux__)
            if (!display) display = XOpenDisplay(nullptr);
            if (!display) { lastError = 1; return false; }
            const KeyCode kc = XKeysymToKeycode(display, static_cast<KeySym>(virtualKey));
            if (kc == 0) { lastError = 2; return false; }
            XTestFakeKeyEvent(display, kc, True, 0);
            XFlush(display);
            lastError = 0;
            return true;
        #else
            lastError = 3;
            return false;
        #endif
    }

    bool KeySimulator::releaseKey(int virtualKey) {
        if (virtualKey <= 0) { lastError = 0; return false; }

        #ifdef _WIN32
            INPUT input = {};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = static_cast<WORD>(virtualKey);
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            if (SendInput(1, &input, sizeof(INPUT)) != 1) {
                lastError = static_cast<unsigned long>(GetLastError());
                return false;
            }
            lastError = 0;
            return true;
        #elif defined(__linux__)
            if (!display) display = XOpenDisplay(nullptr);
            if (!display) { lastError = 1; return false; }
            const KeyCode kc = XKeysymToKeycode(display, static_cast<KeySym>(virtualKey));
            if (kc == 0) { lastError = 2; return false; }
            XTestFakeKeyEvent(display, kc, False, 0);
            XFlush(display);
            lastError = 0;
            return true;
        #else
            lastError = 3;
            return false;
        #endif
    }

    void KeySimulator::PressKey(int keyCode)   { pressKey(keyCode); }
    void KeySimulator::ReleaseKey(int keyCode) { releaseKey(keyCode); }

    unsigned long KeySimulator::getLastError() noexcept { return lastError; }
    void KeySimulator::clearLastError() noexcept { lastError = 0; }
    void KeySimulator::setLastError(unsigned long code) noexcept { lastError = code; }

} // namespace Core