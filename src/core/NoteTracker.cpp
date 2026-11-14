#include "core/NoteTracker.h"

#include <algorithm>
#include <cctype>

namespace Core {

static std::string toUpperStr(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

NoteTracker::NoteTracker() {
    activeSourceMask.fill(0);
}

void NoteTracker::setMapping(const std::map<int, std::string>& mapping) {
    releaseAllPhysicalKeys();
    noteToKeyMap.clear();
    for (const auto& [note, key] : mapping) {
        noteToKeyMap[note] = toUpperStr(key);
    }
}

void NoteTracker::setNoteMapping(int note, const std::string& keyChar) {
    releaseAllPhysicalKeys();
    if (keyChar.empty()) {
        noteToKeyMap.erase(note);
    } else {
        noteToKeyMap[note] = toUpperStr(keyChar);
    }
}

void NoteTracker::clearNoteMapping(int note) {
    releaseAllPhysicalKeys();
    noteToKeyMap.erase(note);
}

std::vector<int> NoteTracker::getNotesMappedTo(const std::string& keyChar) const {
    std::vector<int> notes;
    if (keyChar.empty()) return notes;

    std::string upperKey = toUpperStr(keyChar);
    for (const auto& [note, mappedKey] : noteToKeyMap) {
        if (mappedKey == upperKey) {
            notes.push_back(note);
        }
    }
    return notes;
}

void NoteTracker::noteOn(int note, NoteSource source, int velocity) {
    if (note < 0 || note > 127) return;

    bool wasActive = (activeSourceMask[note] != 0);
    activeSourceMask[note] |= static_cast<uint32_t>(source);

    // Only fire onNoteOn if the note transitioned from inactive to active
    if (!wasActive && onNoteOn) {
        onNoteOn(note, velocity);
    }
}

void NoteTracker::noteOff(int note, NoteSource source) {
    if (note < 0 || note > 127) return;

    activeSourceMask[note] &= ~static_cast<uint32_t>(source);

    // Only fire onNoteOff if no sources remain holding this note
    if (activeSourceMask[note] == 0 && onNoteOff) {
        onNoteOff(note);
    }
}

bool NoteTracker::processKeyPress(int keyCode, const std::string& keyChar, int velocity) {
    // 1. Auto-repeat check: if this key code is already recorded as held,
    // ignore this press to prevent re-triggering Note On.
    if (heldPhysicalKeys.find(keyCode) != heldPhysicalKeys.end()) {
        return true; // Handled / swallowed auto-repeat
    }

    // 2. Look up matching notes for this key
    std::vector<int> notes = getNotesMappedTo(keyChar);
    if (notes.empty()) {
        return false; // Key is unmapped, let system or other UI handle it
    }

    // 3. Record key and trigger Note On for all mapped notes
    heldPhysicalKeys[keyCode] = notes;
    for (int note : notes) {
        noteOn(note, NoteSource::PhysicalKeyboard, velocity);
    }

    return true;
}

void NoteTracker::processKeyRelease(const std::function<bool(int keyCode)>& isKeyCurrentlyDown) {
    std::vector<int> releasedCodes;

    for (const auto& [code, notes] : heldPhysicalKeys) {
        if (!isKeyCurrentlyDown(code)) {
            releasedCodes.push_back(code);
        }
    }

    for (int code : releasedCodes) {
        auto it = heldPhysicalKeys.find(code);
        if (it != heldPhysicalKeys.end()) {
            for (int note : it->second) {
                noteOff(note, NoteSource::PhysicalKeyboard);
            }
            heldPhysicalKeys.erase(it);
        }
    }
}

void NoteTracker::releaseAllPhysicalKeys() {
    for (const auto& [code, notes] : heldPhysicalKeys) {
        for (int note : notes) {
            noteOff(note, NoteSource::PhysicalKeyboard);
        }
    }
    heldPhysicalKeys.clear();
}

void NoteTracker::releaseAllNotes() {
    heldPhysicalKeys.clear();

    for (int note = 0; note < 127; ++note) {
        if (activeSourceMask[note] != 0) {
            activeSourceMask[note] = 0;
            if (onNoteOff) {
                onNoteOff(note);
            }
        }
    }
}

bool NoteTracker::isNoteActive(int note) const {
    if (note < 0 || note > 127) return false;
    return activeSourceMask[note] != 0;
}

std::set<int> NoteTracker::getActiveNotes() const {
    std::set<int> active;
    for (int note = 0; note < 128; ++note) {
        if (activeSourceMask[note] != 0) {
            active.insert(note);
        }
    }
    return active;
}

std::set<std::string> NoteTracker::getActiveKeyNames() const {
    std::set<std::string> activeNames;
    for (int note = 0; note < 128; ++note) {
        if (activeSourceMask[note] != 0) {
            auto it = noteToKeyMap.find(note);
            if (it != noteToKeyMap.end()) {
                activeNames.insert(it->second);
            }
        }
    }
    return activeNames;
}

} // namespace Core
