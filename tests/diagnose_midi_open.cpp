// Isolated diagnostic for MIDI device open failures.
// Kept separate from the GUI so each failing layer can be observed on its own.
// This program only reads WinMM state and attempts a normal open; it changes
// no driver, registry or system setting.

#include <juce_audio_devices/juce_audio_devices.h>

#ifdef _WIN32
    #include <windows.h>
    #include <mmsystem.h>

    #ifndef DRV_QUERYDEVICEINTERFACE
        #define DRV_RESERVED                  0x0800
        #define DRV_QUERYDEVICEINTERFACE     (DRV_RESERVED + 12)
        #define DRV_QUERYDEVICEINTERFACESIZE (DRV_RESERVED + 13)
    #endif
#endif

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

namespace {
#ifdef _WIN32
    void CALLBACK noopMidiCallback (HMIDIIN, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR) {}

    std::string mmError (MMRESULT code) {
        char text[256] = {};
        midiInGetErrorTextA (code, text, sizeof (text));
        return std::string (text) + " [code=" + std::to_string (code) + "]";
    }

    std::string interfaceIdFor (UINT deviceIndex) {
        ULONG size = 0;
        if (midiInMessage ((HMIDIIN) (UINT_PTR) deviceIndex, DRV_QUERYDEVICEINTERFACESIZE,
                           (DWORD_PTR) &size, 0) != MMSYSERR_NOERROR || size == 0 || size > 1024)
            return "<unavailable>";
        wchar_t buffer[512] = {};
        if (midiInMessage ((HMIDIIN) (UINT_PTR) deviceIndex, DRV_QUERYDEVICEINTERFACE,
                           (DWORD_PTR) buffer, sizeof (buffer)) != MMSYSERR_NOERROR)
            return "<query failed>";
        return juce::String (buffer).toStdString();
    }
#endif

    class CountingCallback final : public juce::MidiInputCallback {
    public:
        void handleIncomingMidiMessage (juce::MidiInput*, const juce::MidiMessage&) override {
            ++messages;
        }
        std::atomic<int> messages { 0 };
    };

    void printEnvironment() {
    #ifdef _WIN32
        DWORD pid = GetCurrentProcessId();
        DWORD session = 0;
        ProcessIdToSessionId (pid, &session);
        std::cout << "process pid=" << pid << " sessionId=" << session << "\n";
        std::cout << "midiInGetNumDevs=" << midiInGetNumDevs() << "\n";
    #else
        std::cout << "non-Windows build: WinMM probes skipped\n";
    #endif
    }

    void probeWinMM() {
    #ifdef _WIN32
        std::cout << "\n--- raw WinMM enumeration ---\n";
        const UINT count = midiInGetNumDevs();
        for (UINT i = 0; i < count; ++i) {
            MIDIINCAPS caps = {};
            MMRESULT capsResult = midiInGetDevCaps (i, &caps, sizeof (caps));
            std::cout << "[" << i << "] caps=" << mmError (capsResult)
                      << " name='" << caps.szPname << "'\n";
            std::cout << "    interfaceId=" << interfaceIdFor (i) << "\n";
        }

        std::cout << "\n--- raw midiInOpen variants (device index 0) ---\n";
        if (count == 0) {
            std::cout << "no device to probe\n";
            return;
        }

        {
            HMIDIIN handle = nullptr;
            MMRESULT r = midiInOpen (&handle, 0, 0, 0, CALLBACK_NULL);
            std::cout << "CALLBACK_NULL      -> " << mmError (r) << "\n";
            if (r == MMSYSERR_NOERROR) midiInClose (handle);
        }
        {
            HMIDIIN handle = nullptr;
            MMRESULT r = midiInOpen (&handle, 0, (DWORD_PTR) &noopMidiCallback, 0, CALLBACK_FUNCTION);
            std::cout << "CALLBACK_FUNCTION  -> " << mmError (r) << "\n";
            if (r == MMSYSERR_NOERROR) midiInClose (handle);
        }
        {
            HMIDIIN handle = nullptr;
            MMRESULT r = midiInOpen (&handle, MIDI_MAPPER, 0, 0, CALLBACK_NULL);
            std::cout << "MIDI_MAPPER        -> " << mmError (r) << "\n";
            if (r == MMSYSERR_NOERROR) midiInClose (handle);
        }
        {
            // Two consecutive opens tell us whether the first close really
            // released the port or whether something still holds it.
            HMIDIIN a = nullptr, b = nullptr;
            MMRESULT r1 = midiInOpen (&a, 0, 0, 0, CALLBACK_NULL);
            MMRESULT r2 = midiInOpen (&b, 0, 0, 0, CALLBACK_NULL);
            std::cout << "double open        -> first=" << mmError (r1)
                      << " second=" << mmError (r2) << "\n";
            if (r1 == MMSYSERR_NOERROR) midiInClose (a);
            if (r2 == MMSYSERR_NOERROR) midiInClose (b);
        }
    #endif
    }

    void probeJUCE() {
        std::cout << "\n--- JUCE enumeration (twice, for identifier stability) ---\n";
        auto first  = juce::MidiInput::getAvailableDevices();
        auto second = juce::MidiInput::getAvailableDevices();
        std::cout << "count first=" << first.size() << " second=" << second.size() << "\n";

        bool stable = first.size() == second.size();
        for (int i = 0; i < first.size() && stable; ++i) {
            stable = first[i].name == second[i].name &&
                     first[i].identifier == second[i].identifier;
            std::cout << "[" << i << "] name='" << first[i].name
                      << "' identifier='" << first[i].identifier << "'\n";
        }
        std::cout << "identifiers stable across two enumerations: "
                  << (stable ? "YES" : "NO") << "\n";

        std::cout << "\n--- JUCE MidiInput::openDevice ---\n";
        CountingCallback callback;
        for (int i = 0; i < first.size(); ++i) {
            auto input = juce::MidiInput::openDevice (first[i].identifier, &callback);
            std::cout << "[" << i << "] '" << first[i].name << "' -> "
                      << (input ? "OPENED" : "FAILED") << "\n";
            if (input) {
                input->start();
                input->stop();
                input.reset();
            }
        }

        auto defaultDevice = juce::MidiInput::getDefaultDevice();
        std::cout << "default device name='" << defaultDevice.name
                  << "' identifier='" << defaultDevice.identifier << "'\n";
        auto defaultInput = juce::MidiInput::openDevice (defaultDevice.identifier, &callback);
        std::cout << "default open -> " << (defaultInput ? "OPENED" : "FAILED") << "\n";
        if (defaultInput) defaultInput.reset();

        // Mimic the application's close-then-open sequence in one process.
        if (!first.isEmpty()) {
            auto a = juce::MidiInput::openDevice (first[0].identifier, &callback);
            std::cout << "app-style open #1 -> " << (a ? "OPENED" : "FAILED") << "\n";
            if (a) { a->stop(); a.reset(); }
            auto b = juce::MidiInput::openDevice (first[0].identifier, &callback);
            std::cout << "app-style open #2 (immediate) -> " << (b ? "OPENED" : "FAILED") << "\n";
            if (b) { b->stop(); b.reset(); }
        }
    }
}

int main (int argc, char** argv) {
    // Optional mode: hold the port open from this process so a second process
    // can observe what WinMM reports when the device is genuinely in use.
    if (argc >= 3 && std::string (argv[1]) == "--hold") {
        const int seconds = std::atoi (argv[2]);
        const bool shouldStart = (argc < 4 || std::string (argv[3]) != "--no-start");
        auto devices = juce::MidiInput::getAvailableDevices();
        CountingCallback callback;
        if (devices.isEmpty()) {
            std::cout << "HOLD: no device\n";
            return 1;
        }
        auto input = juce::MidiInput::openDevice (devices[0].identifier, &callback);
        #ifdef _WIN32
            const auto holderPid = GetCurrentProcessId();
        #else
            const int holderPid = 0;
        #endif
        std::cout << "HOLD open -> " << (input ? "OPENED" : "FAILED")
                  << " holding " << seconds << "s pid=" << holderPid << "\n" << std::flush;
        if (!input) return 1;
        if (shouldStart) input->start();
        std::this_thread::sleep_for (std::chrono::seconds (seconds));
        if (shouldStart) input->stop();
        input.reset();
        std::cout << "HOLD released\n";
        return 0;
    }

    std::cout << "=== Keydium isolated MIDI open diagnostic ===\n";
    printEnvironment();
    probeWinMM();
    probeJUCE();
    std::cout << "\nDone (no system state was modified).\n";
    return 0;
}
