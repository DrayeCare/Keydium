#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <array>
#include <cstdint>
#include <functional>

namespace Core {

enum class NoteSource : uint32_t {
    Mouse            = (1 << 0),
    PhysicalKeyboard = (1 << 1),
    MidiDevice       = (1 << 2),
    MidiPlayer       = (1 << 3)
};

/**
 * @class NoteTracker
 * @brief Thread-safe manager for MIDI note routing, multi-source reference counting,
 *        key-to-note mapping, and keyboard auto-repeat suppression.
 */
class NoteTracker {
public:
    using NoteOnCallback  = std::function<void(int note, int velocity)>;
    using NoteOffCallback = std::function<void(int note)>;

    NoteTracker();
    ~NoteTracker() = default;

    // Callbacks invoked when a note transitions active or inactive
    NoteOnCallback  onNoteOn;
    NoteOffCallback onNoteOff;

    // ── Mapping Management ────────────────────────────────────────────────────
    void setMapping(const std::map<int, std::string>& mapping);
    void setNoteMapping(int note, const std::string& keyChar);
    void clearNoteMapping(int note);
    const std::map<int, std::string>& getMapping() const { return noteToKeyMap; }

    // ── Direct Note Triggering ────────────────────────────────────────────────
    void noteOn(int note, NoteSource source, int velocity = 100);
    void noteOff(int note, NoteSource source);

    // ── Physical Keyboard Input Processing ────────────────────────────────────
    /**
     * Called when a physical key is pressed.
     * @param keyCode   The OS / framework key code (e.g. KeyPress::getKeyCode()).
     * @param keyChar   The text/printable character representation (e.g. "A").
     * @param velocity  MIDI velocity (default 100).
     * @return true if the key is mapped or was ignored due to auto-repeat; false if unmapped.
     */
    bool processKeyPress(int keyCode, const std::string& keyChar, int velocity = 100);

    /**
     * Called on key state change events to check whether previously held keys were released.
     * @param isKeyCurrentlyDown  Predicate checking if a key code is still physically held down.
     */
    void processKeyRelease(const std::function<bool(int keyCode)>& isKeyCurrentlyDown);

    // ── Emergency / Focus Release ─────────────────────────────────────────────
    /** Release only physical keyboard notes (e.g. when app loses focus). */
    void releaseAllPhysicalKeys();

    /** Release all notes from all sources immediately (e.g. audio mute, app exit). */
    void releaseAllNotes();

    // ── Status Queries ────────────────────────────────────────────────────────
    bool isNoteActive(int note) const;
    std::set<int> getActiveNotes() const;
    std::set<std::string> getActiveKeyNames() const;
    const std::map<int, std::vector<int>>& getHeldPhysicalKeys() const { return heldPhysicalKeys; }

private:
    std::map<int, std::string> noteToKeyMap;

    // Bitmask for each MIDI note (0..127) tracking active sources
    std::array<uint32_t, 128> activeSourceMask{};

    // Active physical key codes -> list of MIDI notes triggered
    std::map<int, std::vector<int>> heldPhysicalKeys;

    std::vector<int> getNotesMappedTo(const std::string& keyChar) const;
};

} // namespace Core
