#pragma once

namespace Core {
    class KeySimulator {
    public:
        // Fungsi untuk menekan tombol (bisa pakai karakter atau virtual key code)
        static void PressKey(int keyCode);
        
        // Fungsi untuk melepas tombol
        static void ReleaseKey(int keyCode);
    };
}