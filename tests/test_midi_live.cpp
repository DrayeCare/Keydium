// Hardware-free verification of the Live MIDI Input pipeline.
// Build target: MidiLiveInputTest

#include "core/MidiInputHandler.h"
#include "core/NoteToKeyRouter.h"
#include "core/KeySimulator.h"

#include <juce_audio_devices/juce_audio_devices.h>

#ifdef _WIN32
    #include <windows.h>
    #include <mmsystem.h>
#endif

#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {
    int checksRun   = 0;
    int checksFailed = 0;

    void check(bool condition, const std::string& name) {
        ++checksRun;
        if (condition) {
            std::cout << "  PASS  " << name << "\n";
        } else {
            ++checksFailed;
            std::cout << "  FAIL  " << name << "\n";
        }
    }

    void section(const std::string& name) {
        std::cout << "\n[" << name << "]\n";
    }

    using Events = std::vector<std::pair<int, bool>>; // {virtualKey, down}

    Events recorded;
    void record(int vk, bool down) { recorded.push_back({ vk, down }); }

    void clearRecorded() { recorded.clear(); }
}

int main() {
    std::cout << "=== Keydium Live MIDI Input tests (no hardware required) ===\n";

    // -------------------------------------------------------------------------
    section("1. MIDI event decoding");
    {
        Core::MidiNoteEvent e;

        auto noteOn = juce::MidiMessage::noteOn(1, 60, (juce::uint8)100);
        bool ok = Core::MidiInputHandler::decodeNoteMessage(noteOn, e);
        check(ok && e.note == 60 && e.velocity == 100 && e.channel == 1 && e.noteOn,
              "Note On ch1 note60 vel100 decoded");

        auto noteOff = juce::MidiMessage::noteOff(1, 60, (juce::uint8)0);
        ok = Core::MidiInputHandler::decodeNoteMessage(noteOff, e);
        check(ok && !e.noteOn && e.note == 60,
              "Note Off ch1 note60 decoded");

        auto velZeroOn = juce::MidiMessage::noteOn(2, 64, (juce::uint8)0);
        ok = Core::MidiInputHandler::decodeNoteMessage(velZeroOn, e);
        check(ok && !e.noteOn && e.note == 64 && e.channel == 2,
              "Note On velocity 0 decoded as Note Off (ch2)");

        auto cc = juce::MidiMessage::controllerEvent(1, 7, 100);
        ok = Core::MidiInputHandler::decodeNoteMessage(cc, e);
        check(!ok, "Non-note message (CC) rejected");
    }

    // -------------------------------------------------------------------------
    section("2. MIDI channel filtering");
    {
        check(Core::MidiInputHandler::shouldAcceptChannel(0, 1) &&
              Core::MidiInputHandler::shouldAcceptChannel(0, 16),
              "Omni filter accepts all channels");
        check(Core::MidiInputHandler::shouldAcceptChannel(3, 3) &&
              !Core::MidiInputHandler::shouldAcceptChannel(3, 4),
              "Single-channel filter accepts only its channel");
    }

    // -------------------------------------------------------------------------
    section("3. Virtual key conversion");
    {
        check(Core::KeySimulator::virtualKeyFromName("A") == 0x41, "'A' -> 0x41");
        check(Core::KeySimulator::virtualKeyFromName("z") == 0x5A, "lowercase 'z' -> 0x5A");
        check(Core::KeySimulator::virtualKeyFromName("7") == 0x37, "'7' -> 0x37");
        check(Core::KeySimulator::virtualKeyFromName("AB") == 0, "multi-char rejected");
        check(Core::KeySimulator::virtualKeyFromName("!") == 0, "unsupported char rejected");
    }

    // -------------------------------------------------------------------------
    section("4. Note -> key mapping and key press/release");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"}, {62, "S"} });

        clearRecorded();
        router.noteOn(60, 100);
        check(recorded == Events({ { 0x41, true } }), "Note On 60 sends key A down");
        check(router.getPressedKeyCount() == 1, "One key tracked as pressed");

        clearRecorded();
        router.noteOff(60);
        check(recorded == Events({ { 0x41, false } }), "Note Off 60 sends key A up");
        check(router.getPressedKeyCount() == 0, "No keys tracked after release");
    }

    // -------------------------------------------------------------------------
    section("5. Velocity 0 Note On is a Note Off");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"} });

        router.noteOn(60, 100);
        clearRecorded();
        router.noteOn(60, 0);
        check(recorded == Events({ { 0x41, false } }), "velocity 0 releases the key");
        check(router.getPressedKeyCount() == 0, "velocity 0 clears tracked key");
    }

    // -------------------------------------------------------------------------
    section("6. Duplicate Note On and active note tracking");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"} });

        router.noteOn(60, 100);
        clearRecorded();
        router.noteOn(60, 110);
        check(recorded.empty(), "duplicate Note On does not re-press key");
        check(router.getPressedKeyCount() == 1, "duplicate keeps single ref");

        clearRecorded();
        router.noteOff(60);
        check(recorded == Events({ { 0x41, false } }), "single Note Off releases once");

        // A Note Off for a note that was never on must be harmless.
        clearRecorded();
        router.noteOff(60);
        check(recorded.empty(), "redundant Note Off sends nothing");
    }

    // -------------------------------------------------------------------------
    section("7. Multiple notes / chords");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"}, {62, "S"}, {64, "D"} });

        clearRecorded();
        router.noteOn(60, 100);
        router.noteOn(62, 100);
        router.noteOn(64, 100);
        check(recorded == Events({ {0x41,true}, {0x53,true}, {0x44,true} }),
              "chord presses three distinct keys");
        check(router.getPressedKeyCount() == 3, "three keys tracked");

        clearRecorded();
        router.noteOff(62);
        check(recorded == Events({ { 0x53, false } }), "partial release only releases S");
        check(router.getPressedKeyCount() == 2, "two keys remain pressed");
    }

    // -------------------------------------------------------------------------
    section("8. Several MIDI notes mapped to the same PC key");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"}, {72, "A"} });

        clearRecorded();
        router.noteOn(60, 100);
        router.noteOn(72, 100);
        check(recorded == Events({ { 0x41, true } }),
              "second note on same key does not re-press");

        clearRecorded();
        router.noteOff(60);
        check(recorded.empty(),
              "key NOT released while another note still holds it");
        check(router.getPressedKeyCount() == 1, "ref count keeps key pressed");

        clearRecorded();
        router.noteOff(72);
        check(recorded == Events({ { 0x41, false } }),
              "key released when last holding note ends");
        check(router.getPressedKeyCount() == 0, "no keys left pressed");
    }

    // -------------------------------------------------------------------------
    section("9. Panic / release all keys");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"}, {62, "S"}, {72, "A"} });

        router.noteOn(60, 100);
        router.noteOn(62, 100);
        router.noteOn(72, 100);
        check(router.getPressedKeyCount() == 2, "two physical keys pressed");

        clearRecorded();
        router.panic();
        check(recorded.size() == 2 &&
              recorded[0].second == false && recorded[1].second == false,
              "panic releases every pressed key");
        check(router.getPressedKeyCount() == 0, "panic clears tracked state");

        clearRecorded();
        router.panic();
        check(recorded.empty(), "repeated panic is harmless");
        router.noteOff(60);
        check(recorded.empty(), "Note Off after panic does not double-release");
    }

    // -------------------------------------------------------------------------
    section("10. Mapping change and shutdown release keys");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"} });
        router.noteOn(60, 100);

        clearRecorded();
        router.setMapping({ {60, "B"} });
        check(recorded == Events({ { 0x41, false } }),
              "mapping change releases old key");

        clearRecorded();
        router.noteOn(60, 100);
        check(recorded == Events({ { 0x42, true } }), "new mapping takes effect");

        recorded.clear();
    } // router destructor must release the held key
    check(recorded == Events({ { 0x42, false } }),
          "destructor releases all keys (shutdown safety)");
    check(!Core::NoteToKeyRouter(record).getDiagnostics().empty(),
          "diagnostics available");
    recorded.clear();

    // -------------------------------------------------------------------------
    section("11. Unmapped / invalid notes");
    {
        Core::NoteToKeyRouter router(record);
        router.setMapping({ {60, "A"}, {64, "?"} });
        clearRecorded();

        router.noteOn(61, 100);   // no mapping
        router.noteOn(64, 100);   // unsupported key name
        router.noteOn(200, 100);  // out of range
        check(recorded.empty(), "unmapped/invalid notes send no key events");
        check(router.getPressedKeyCount() == 0, "no phantom pressed keys");
    }

    // -------------------------------------------------------------------------
    section("12. Device enumeration and lifecycle");
    {
        Core::MidiInputHandler handler;
        auto devices = handler.getAvailableDevices();
        std::cout << "  INFO  OS reports " << devices.size()
                  << " MIDI input device(s) on this machine\n";
        for (int i = 0; i < devices.size(); ++i) {
            std::cout << "  INFO  [" << i << "] " << devices[i].name
                      << " (id=" << devices[i].identifier << ")\n";
        }
        check(!handler.isConnected(), "handler starts disconnected");

        bool opened = handler.openDevice("");
        check(!opened, "empty identifier cannot be opened");

        handler.closeDevice();
        check(!handler.isConnected(), "close on disconnected device is safe");

        handler.setChannelFilter(5);
        check(handler.getChannelFilter() == 5, "channel filter stored");
        handler.setChannelFilter(99);
        check(handler.getChannelFilter() == 0, "invalid filter resets to Omni");

        if (!devices.isEmpty()) {
            #ifdef _WIN32
            UINT numDevs = midiInGetNumDevs();
            MIDIINCAPS caps = {};
            MMRESULT capsErr = midiInGetDevCaps (0, &caps, sizeof (caps));
            HMIDIIN probe = nullptr;
            MMRESULT openErr = midiInOpen (&probe, 0, 0, 0, CALLBACK_NULL);
            if (openErr == MMSYSERR_NOERROR) midiInClose (probe);
            std::cout << "  INFO  WinMM devs=" << numDevs << " capsErr=" << capsErr
                      << " name=" << caps.szPname << " rawOpenErr=" << openErr << "\n";
            #endif

            const bool realOpen = handler.openDevice(devices[0].identifier);
            if (realOpen) {
                check(handler.isConnected(), "opens the real OS MIDI device");
                check(handler.getConnectedDeviceName() == devices[0].name,
                      "reports the real device name (not hardcoded)");
                handler.closeDevice();
                check(!handler.isConnected(), "closes the real device cleanly");
            } else {
                // The OS is free to refuse the port (driver/busy/resource limits).
                // This is reported, never counted as a pass.
                std::cout << "  INFO  SKIP hardware note test: port '"
                          << devices[0].name
                          << "' is unavailable (WinMM 7 typically means another app holds it)"
                          << " - no MIDI events injected here\n";
            }
        } else {
            std::cout << "  INFO  SKIP hardware open/close: no device present\n";
        }
    } // destructor: no dangling callback, no crash

    // -------------------------------------------------------------------------
    section("13. SendInput failure consistency (B1 regression)");
    {
        // Sink that can simulate an OS key-press rejection (e.g. UIPI
        // ERROR_ACCESS_DENIED when the foreground game runs elevated).
        bool failNextPress = false;
        auto failingSink = [&failNextPress](int vk, bool down) {
            record(vk, down);
            if (down && failNextPress) {
                failNextPress = false;
                Core::KeySimulator::setLastError(5); // ERROR_ACCESS_DENIED
            } else {
                Core::KeySimulator::clearLastError();
            }
        };

        // A: a failed press leaves no phantom hold, Note Off is inert,
        //    and a later retry can still press the key.
        {
            Core::NoteToKeyRouter router(failingSink);
            router.setMapping({ {60, "A"} });
            failNextPress = true;
            clearRecorded();
            router.noteOn(60, 100);
            check(recorded == Events({ {0x41, true} }), "failed press attempted exactly once");
            check(router.getPressedKeyCount() == 0, "failed press leaves no phantom held key");
            clearRecorded();
            router.noteOff(60);
            check(recorded.empty(), "Note Off after failed press sends no key-up");
            router.noteOn(60, 100); // sink now succeeds
            check(recorded == Events({ {0x41, true} }), "retry after failed press works");
            check(router.getPressedKeyCount() == 1, "retry holds the key");
            clearRecorded();
            router.noteOff(60);
            check(recorded == Events({ {0x41, false} }), "release after retry is clean");
            check(router.getPressedKeyCount() == 0, "nothing held after final release");
        }

        // B: two MIDI notes share one PC key; the failed note's Note Off
        //    must never decrement the successful note's ref-count.
        {
            Core::NoteToKeyRouter router(failingSink);
            router.setMapping({ {60, "A"}, {72, "A"} });
            failNextPress = true;
            clearRecorded();
            router.noteOn(60, 100);          // press fails
            check(router.getPressedKeyCount() == 0, "shared key: failed note holds nothing");
            clearRecorded();
            router.noteOn(72, 100);          // press succeeds
            check(recorded == Events({ {0x41, true} }), "shared key: second note presses the key");
            check(router.getPressedKeyCount() == 1, "shared key: one holder tracked");
            clearRecorded();
            router.noteOff(60);              // failed note releases: must be inert
            check(recorded.empty(), "shared key: failed note's Note Off does NOT release the key");
            check(router.getPressedKeyCount() == 1, "shared key: successful note still holds the key");
            clearRecorded();
            router.noteOff(72);
            check(recorded == Events({ {0x41, false} }), "shared key: last real holder releases the key");
            check(router.getPressedKeyCount() == 0, "shared key: nothing held at the end");
        }
    }

    std::cout << "\n=== " << (checksRun - checksFailed) << "/" << checksRun
              << " checks passed ===\n";
    if (checksFailed != 0) {
        std::cout << "RESULT: FAIL (" << checksFailed << " failed)\n";
        return 1;
    }
    std::cout << "RESULT: PASS\n";
    return 0;
}
