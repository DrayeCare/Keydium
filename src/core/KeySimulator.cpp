#include "core/KeySimulator.h"

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

    void KeySimulator::PressKey(int keyCode) {
        #ifdef _WIN32
            // keyCode di Windows adalah Virtual-Key Code (misal 0x41 untuk 'A')
            INPUT input = {0};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = keyCode;
            SendInput(1, &input, sizeof(INPUT));
            
        #elif __linux__
            // keyCode di Linux sebaiknya menggunakan X11 KeySym (misal XK_a)
            if (!display) display = XOpenDisplay(nullptr);
            if (display) {
                KeyCode kc = XKeysymToKeycode(display, keyCode);
                XTestFakeKeyEvent(display, kc, True, 0); // True = Tekan
                XFlush(display);
            }
        #endif
    }

    void KeySimulator::ReleaseKey(int keyCode) {
        #ifdef _WIN32
            INPUT input = {0};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = keyCode;
            input.ki.dwFlags = KEYEVENTF_KEYUP; // Flag untuk lepas
            SendInput(1, &input, sizeof(INPUT));
            
        #elif __linux__
            if (display) {
                KeyCode kc = XKeysymToKeycode(display, keyCode);
                XTestFakeKeyEvent(display, kc, False, 0); // False = Lepas
                XFlush(display);
            }
        #endif
    }
}