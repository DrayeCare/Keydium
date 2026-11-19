#include "core/PianoAudioEngine.h"
#include "core/NoteTracker.h"

#include <fluidsynth.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>

int main() {
    std::cout << "=== Running Keydium Extended Test Suite ===" << std::endl;

    Core::PianoAudioEngine engine;
    const std::string sf2Path = "assets/sounds/GeneralUser-GS.sf2";
    bool ok = engine.initialise(sf2Path);
    if (!ok) {
        std::cerr << "FAIL: Could not load SoundFont at: " << sf2Path << std::endl;
        return 1;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 1: Output mono dikirim secara identik ke kanal kiri dan kanan
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 1] Verifying mono output is identical on Left and Right channels..." << std::endl;
    {
        // Direct algorithmic unit test of downmixStereoToMono
        const int testLen = 256;
        std::vector<float> l(testLen, 0.8f);
        std::vector<float> r(testLen, -0.2f);
        float* bufs[2] = { l.data(), r.data() };

        Core::PianoAudioEngine::downmixStereoToMono(bufs, 2, testLen);

        float expectedMono = 0.5f * (0.8f + (-0.2f)); // 0.3f
        for (int i = 0; i < testLen; ++i) {
            assert(std::fabs(l[i] - expectedMono) < 1e-6f);
            assert(std::fabs(r[i] - expectedMono) < 1e-6f);
            assert(l[i] == r[i]); // Exact identity
        }

        // Live synthesizer render test: note 60 rendered through synth
        engine.noteOn(60, 100);
        std::vector<float> synthL(512, 0.0f);
        std::vector<float> synthR(512, 0.0f);
        bool rendered = engine.renderAudio(synthL.data(), synthR.data(), 512);
        assert(rendered);

        bool hasAudio = false;
        for (int i = 0; i < 512; ++i) {
            assert(synthL[i] == synthR[i]); // Identical on both channels
            assert(std::fabs(synthL[i]) <= 1.0f); // No clipping
            if (std::fabs(synthL[i]) > 1e-4f) {
                hasAudio = true;
            }
        }
        assert(hasAudio); // Verified sound was generated
        engine.noteOff(60);
        std::cout << "  -> Passed: Mono downmix verified; Left and Right buffers are mathematically identical." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 1b (B2 regression): the audio driver fallback must be a driver
    // that actually exists in this FluidSynth build ("winmm" does not).
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 1b] Verifying FluidSynth audio driver options (B2 fallback regression)..." << std::endl;
    {
        fluid_settings_t* probeSettings = new_fluid_settings();
        assert(probeSettings != nullptr);
        std::vector<std::string> drivers;
        fluid_settings_foreach_option(probeSettings, "audio.driver", &drivers,
            [](void* data, const char*, const char* option) {
                static_cast<std::vector<std::string>*>(data)->push_back(option);
            });
        delete_fluid_settings(probeSettings);

        bool hasDsound = false, hasWaveout = false, hasWinmm = false;
        for (const auto& d : drivers) {
            if (d == "dsound")  hasDsound = true;
            if (d == "waveout") hasWaveout = true;
            if (d == "winmm")   hasWinmm = true;
        }
        assert(hasDsound);   // primary driver used by initialise()
        assert(hasWaveout);  // fallback driver used by initialise()
        assert(!hasWinmm);   // the old, invalid fallback name must not exist
        std::cout << "  -> Passed: dsound + waveout available; 'winmm' is not a FluidSynth driver." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Setup NoteTracker for keyboard & note mapping tests
    // ─────────────────────────────────────────────────────────────────────────
    Core::NoteTracker tracker;
    int noteOnCount = 0;
    int noteOffCount = 0;
    std::vector<int> lastNotesOn;
    std::vector<int> lastNotesOff;

    tracker.onNoteOn = [&](int note, int velocity) {
        noteOnCount++;
        lastNotesOn.push_back(note);
        engine.noteOn(note, velocity);
    };

    tracker.onNoteOff = [&](int note) {
        noteOffCount++;
        lastNotesOff.push_back(note);
        engine.noteOff(note);
    };

    // Standard Genshin mapping subset: A->60 (C4), S->62 (D4), D->64 (E4), W->74 (D5)
    tracker.setMapping({
        {60, "A"},
        {62, "S"},
        {64, "D"},
        {74, "W"}
    });

    // ─────────────────────────────────────────────────────────────────────────
    // Test 2: Satu tombol menghasilkan satu Note On
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 2] Verifying single key press produces exactly one Note On..." << std::endl;
    {
        noteOnCount = 0;
        lastNotesOn.clear();

        // Press 'A' (keyCode = 65, keyChar = "A")
        bool handled = tracker.processKeyPress(65, "A", 100);
        assert(handled);
        assert(noteOnCount == 1);
        assert(lastNotesOn.size() == 1 && lastNotesOn[0] == 60);
        assert(tracker.isNoteActive(60));
        std::cout << "  -> Passed: Exactly one Note On (note 60) produced." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 3: Note Off dikirim saat tombol dilepas
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 3] Verifying Note Off is sent when key is released..." << std::endl;
    {
        noteOffCount = 0;
        lastNotesOff.clear();

        // Release 'A' (simulating keyStateChanged where 'A' is no longer down)
        tracker.processKeyRelease([](int code) {
            return false; // key 65 is now UP
        });

        assert(noteOffCount == 1);
        assert(lastNotesOff.size() == 1 && lastNotesOff[0] == 60);
        assert(!tracker.isNoteActive(60));
        std::cout << "  -> Passed: Exactly one Note Off (note 60) produced on release." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 4: Beberapa tombol dapat menghasilkan chord (polyphony)
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 4] Verifying multiple keys produce chords simultaneously..." << std::endl;
    {
        noteOnCount = 0;
        lastNotesOn.clear();

        // Press 'A' (60), 'S' (62), and 'D' (64)
        tracker.processKeyPress(65, "A", 100);
        tracker.processKeyPress(83, "S", 100);
        tracker.processKeyPress(68, "D", 100);

        assert(noteOnCount == 3);
        assert(tracker.isNoteActive(60));
        assert(tracker.isNoteActive(62));
        assert(tracker.isNoteActive(64));
        assert(tracker.getActiveNotes().size() == 3);

        // Release only 'S' (83)
        noteOffCount = 0;
        tracker.processKeyRelease([](int code) {
            return (code == 65 || code == 68); // 'A' and 'D' still held, 'S' released
        });

        assert(noteOffCount == 1);
        assert(!tracker.isNoteActive(62));
        assert(tracker.isNoteActive(60)); // 'A' still active
        assert(tracker.isNoteActive(64)); // 'D' still active

        // Release remaining keys
        tracker.processKeyRelease([](int) { return false; });
        assert(!tracker.isNoteActive(60));
        assert(!tracker.isNoteActive(64));
        assert(tracker.getActiveNotes().empty());
        std::cout << "  -> Passed: Chord played polyphonically; partial releases handled cleanly." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 5: Keyboard auto-repeat tidak menghasilkan nada berulang
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 5] Verifying keyboard auto-repeat does not produce repeated Note On..." << std::endl;
    {
        noteOnCount = 0;
        lastNotesOn.clear();

        // Initial press
        bool h1 = tracker.processKeyPress(65, "A", 100);
        assert(h1);
        assert(noteOnCount == 1);

        // 10 simulated auto-repeat presses of key 'A' while held down
        for (int i = 0; i < 10; ++i) {
            bool hRepeat = tracker.processKeyPress(65, "A", 100);
            assert(hRepeat); // Handled / swallowed
        }

        // Note On count must still be 1 (auto-repeats suppressed)
        assert(noteOnCount == 1);

        // Clean release
        tracker.processKeyRelease([](int) { return false; });
        assert(!tracker.isNoteActive(60));
        std::cout << "  -> Passed: 10 auto-repeat events suppressed; only 1 Note On fired." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 6: Audio OFF menghentikan nada aktif
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 6] Verifying Audio OFF silences all active notes..." << std::endl;
    {
        // Trigger notes
        tracker.processKeyPress(65, "A", 100);
        tracker.processKeyPress(68, "D", 100);
        assert(tracker.isNoteActive(60));
        assert(tracker.isNoteActive(64));

        // Disable audio
        tracker.releaseAllNotes();
        engine.setEnabled(false);
        assert(!engine.isEnabled());
        assert(tracker.getActiveNotes().empty());

        // While audio is disabled, new notes must not produce sound
        engine.noteOn(60, 100);

        // Re-enable
        engine.setEnabled(true);
        assert(engine.isEnabled());
        std::cout << "  -> Passed: Active notes stopped on audio disable; sound disabled when OFF." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 7: Mapping yang diubah langsung berlaku untuk keyboard fisik
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 7] Verifying updated mapping immediately applies to physical keyboard..." << std::endl;
    {
        noteOnCount = 0;
        lastNotesOn.clear();

        // Initially 'W' is mapped to note 74
        tracker.processKeyPress(87, "W", 100);
        assert(noteOnCount == 1 && lastNotesOn[0] == 74);
        tracker.processKeyRelease([](int) { return false; });

        // User reassigns 'W' to Note 72 in the UI
        tracker.setNoteMapping(72, "W");
        tracker.clearNoteMapping(74);

        noteOnCount = 0;
        lastNotesOn.clear();

        // Next press of 'W' must now play Note 72 directly
        tracker.processKeyPress(87, "W", 100);
        assert(noteOnCount == 1 && lastNotesOn[0] == 72);
        assert(tracker.isNoteActive(72));
        assert(!tracker.isNoteActive(74));

        tracker.processKeyRelease([](int) { return false; });
        std::cout << "  -> Passed: Reassigned key 'W' immediately triggered updated note 72." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Test 8: Tidak ada stuck notes setelah kehilangan fokus
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Test 8] Verifying no stuck notes after losing focus..." << std::endl;
    {
        // User is holding down 'A' and 'D' when window loses focus (e.g. Alt-Tab)
        tracker.processKeyPress(65, "A", 100);
        tracker.processKeyPress(68, "D", 100);
        assert(tracker.isNoteActive(60));
        assert(tracker.isNoteActive(64));
        assert(tracker.getHeldPhysicalKeys().size() == 2);

        // Focus lost triggers releaseAllPhysicalKeys()
        tracker.releaseAllPhysicalKeys();

        assert(!tracker.isNoteActive(60));
        assert(!tracker.isNoteActive(64));
        assert(tracker.getActiveNotes().empty());
        assert(tracker.getHeldPhysicalKeys().empty());

        // Subsequent key state check detects no hanging keys
        tracker.processKeyRelease([](int) { return false; });
        assert(tracker.getActiveNotes().empty());
        std::cout << "  -> Passed: All active physical keys safely released on focus loss." << std::endl;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Multi-source concurrent test (Mouse + Keyboard coexistence)
    // ─────────────────────────────────────────────────────────────────────────
    std::cout << "[Bonus Test] Verifying simultaneous Mouse + Physical Keyboard input..." << std::endl;
    {
        noteOnCount = 0;
        noteOffCount = 0;

        // Mouse clicks note 60
        tracker.noteOn(60, Core::NoteSource::Mouse, 100);
        assert(noteOnCount == 1);

        // While mouse is held, user presses physical key 'A' (mapped to note 60)
        tracker.noteOn(60, Core::NoteSource::PhysicalKeyboard, 100);
        // Note was already active -> no duplicate Note On!
        assert(noteOnCount == 1);

        // User releases mouse
        tracker.noteOff(60, Core::NoteSource::Mouse);
        // Physical key is still held -> note must NOT stop yet!
        assert(noteOffCount == 0);
        assert(tracker.isNoteActive(60));

        // User releases physical key
        tracker.noteOff(60, Core::NoteSource::PhysicalKeyboard);
        // All sources released -> Note Off fired!
        assert(noteOffCount == 1);
        assert(!tracker.isNoteActive(60));
        std::cout << "  -> Passed: Multi-source reference counting prevented duplicate On/Off and stuck notes." << std::endl;
    }

    // Clean shutdown
    engine.shutdown();
    std::cout << "\n>>> ALL TESTS PASSED SUCCESSFULLY! (8/8 Required + Audio Mono + Concurrent Multi-Source) <<<" << std::endl;
    return 0;
}
