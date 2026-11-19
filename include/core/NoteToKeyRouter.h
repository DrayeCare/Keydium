#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>

namespace Core {

/**
 * Live MIDI note -> simulated PC key router.
 *
 * Reuses the note -> key mapping owned by the rest of Keydium (it is passed in
 * via setMapping) and keeps its own reference count per physical key so that
 * several MIDI notes mapped to the same key never release it prematurely.
 *
 * Intended to be driven from the JUCE message thread only, which is guaranteed
 * by MidiInputHandler's AsyncUpdater delivery.
 */
class NoteToKeyRouter {
public:
    using KeySink = std::function<void(int virtualKey, bool down)>;

    /** A null sink forwards to KeySimulator (real OS keyboard input). */
    explicit NoteToKeyRouter(KeySink sink = nullptr);
    ~NoteToKeyRouter();

    /** Replaces the mapping and releases every currently held key. */
    void setMapping(const std::map<int, std::string>& noteToKey);

    /** velocity <= 0 is treated as Note Off, per the MIDI spec. */
    void noteOn(int note, int velocity);
    void noteOff(int note);

    /** Releases every key this router pressed. Safe to call repeatedly. */
    void panic();

    int getPressedKeyCount() const;
    std::string getDiagnostics() const;

private:
    KeySink sink;
    std::map<int, std::string> mapping;
    std::map<int, std::set<int>> noteToPressedKeys; // note -> virtual keys
    std::map<int, int> keyRefCount;                 // virtual key -> holders
    std::string lastDiagnostic;

    void sendKey(int virtualKey, bool down);
    void releaseNoteKeys(int note);

    NoteToKeyRouter(const NoteToKeyRouter&) = delete;
    NoteToKeyRouter& operator=(const NoteToKeyRouter&) = delete;
};

} // namespace Core
