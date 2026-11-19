#include "core/NoteToKeyRouter.h"

#include "core/KeySimulator.h"

#include <utility>

namespace Core {

NoteToKeyRouter::NoteToKeyRouter(KeySink sinkIn)
    : sink(std::move(sinkIn ? sinkIn : KeySink([](int virtualKey, bool down) {
          if (down) KeySimulator::pressKey(virtualKey);
          else      KeySimulator::releaseKey(virtualKey);
      }))) {
    lastDiagnostic = "Router ready (OS keyboard output)";
}

NoteToKeyRouter::~NoteToKeyRouter() {
    panic();
}

void NoteToKeyRouter::setMapping(const std::map<int, std::string>& noteToKey) {
    // A mapping change can invalidate a held key; release everything first.
    panic();
    mapping = noteToKey;
    lastDiagnostic = "Mapping applied (" + std::to_string(mapping.size()) + " notes)";
}

void NoteToKeyRouter::sendKey(int virtualKey, bool down) {
    if (sink) sink(virtualKey, down);
}

void NoteToKeyRouter::noteOn(int note, int velocity) {
    if (note < 0 || note > 127) return;
    if (velocity <= 0) {
        noteOff(note); // velocity 0 == Note Off
        return;
    }

    auto mappingIt = mapping.find(note);
    if (mappingIt == mapping.end()) return;

    const int virtualKey = KeySimulator::virtualKeyFromName(mappingIt->second);
    if (virtualKey == 0) {
        lastDiagnostic = "Unsupported key '" + mappingIt->second +
                         "' for note " + std::to_string(note);
        return;
    }

    auto& keysForNote = noteToPressedKeys[note];
    if (keysForNote.count(virtualKey) != 0) return; // duplicate Note On

    keysForNote.insert(virtualKey);
    if (keyRefCount[virtualKey] == 0) {
        KeySimulator::clearLastError();
        sendKey(virtualKey, true);
        const auto osError = KeySimulator::getLastError();
        if (osError != 0) {
            // B1 fix: a failed press must leave no trace. Without this rollback
            // the note kept a phantom hold: retries were blocked, and its later
            // Note Off decremented another note's ref-count, releasing a key
            // that was still legitimately held.
            keysForNote.erase(virtualKey);
            if (keysForNote.empty()) noteToPressedKeys.erase(note);
            keyRefCount.erase(virtualKey); // drop the zero-count entry
            lastDiagnostic = "Key press failed (vk=" + std::to_string(virtualKey) +
                             ", os error=" + std::to_string(osError) + ")";
            return;
        }
    }
    ++keyRefCount[virtualKey];
    lastDiagnostic = "Note " + std::to_string(note) + " ON -> key " + mappingIt->second;
}

void NoteToKeyRouter::releaseNoteKeys(int note) {
    auto noteIt = noteToPressedKeys.find(note);
    if (noteIt == noteToPressedKeys.end()) return;

    for (int virtualKey : noteIt->second) {
        auto refIt = keyRefCount.find(virtualKey);
        if (refIt == keyRefCount.end()) continue;
        if (--refIt->second <= 0) {
            sendKey(virtualKey, false);
            keyRefCount.erase(refIt);
        }
    }
    noteToPressedKeys.erase(noteIt);
}

void NoteToKeyRouter::noteOff(int note) {
    releaseNoteKeys(note);
    lastDiagnostic = "Note " + std::to_string(note) + " OFF";
}

void NoteToKeyRouter::panic() {
    for (const auto& entry : keyRefCount) {
        if (entry.second > 0) sendKey(entry.first, false);
    }
    keyRefCount.clear();
    noteToPressedKeys.clear();
    lastDiagnostic = "Panic: all simulated keys released";
}

int NoteToKeyRouter::getPressedKeyCount() const {
    return static_cast<int>(keyRefCount.size());
}

std::string NoteToKeyRouter::getDiagnostics() const {
    return lastDiagnostic;
}

} // namespace Core
