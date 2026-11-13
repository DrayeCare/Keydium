#include "gui/PianoKeyboard.h"

PianoKeyboard::PianoKeyboard() { setOpaque(false); }
PianoKeyboard::~PianoKeyboard() {}

void PianoKeyboard::paint(juce::Graphics& g) {
    // Draw white keys
    for (auto& k : keys) {
        if (k.isBlack) continue;
        bool isActive = activeKeys.count(k.note) > 0;
        auto col = isActive ? juce::Colour(0xffffa726) : juce::Colour(0xffe8e8ee);
        g.setColour(col);
        g.fillRoundedRectangle(k.bounds, 3.0f);
        g.setColour(juce::Colour(0xff2a2a3e));
        g.drawRoundedRectangle(k.bounds, 3.0f, 1.0f);

        auto it = keyMappings.find(k.note);
        if (it != keyMappings.end()) {
            g.setColour(juce::Colours::black.withAlpha(0.6f));
            g.setFont(juce::Font(11.0f, juce::Font::bold));
            g.drawText(it->second, k.bounds.reduced(2.0f).removeFromBottom(18.0f), juce::Justification::centred);
        }
    }

    // Draw black keys
    for (auto& k : keys) {
        if (!k.isBlack) continue;
        bool isActive = activeKeys.count(k.note) > 0;
        auto col = isActive ? juce::Colour(0xffff8f00) : juce::Colour(0xff101018);
        g.setColour(col);
        g.fillRoundedRectangle(k.bounds, 2.0f);
        g.setColour(juce::Colour(0xff2a2a3e));
        g.drawRoundedRectangle(k.bounds, 2.0f, 1.0f);

        auto it = keyMappings.find(k.note);
        if (it != keyMappings.end()) {
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(juce::Font(10.0f, juce::Font::bold));
            g.drawText(it->second, k.bounds.reduced(1.0f).removeFromBottom(14.0f), juce::Justification::centred);
        }
    }
}

void PianoKeyboard::resized() { rebuildKeys(); }

void PianoKeyboard::mouseDown(const juce::MouseEvent& e) {
    int note = getNoteAtPosition(e.position);
    if (note < 0) return;
    if (e.mods.isRightButtonDown()) {
        if (onNoteRightClick) onNoteRightClick(note);
        return;
    }
    mousePressedNote = note;
    if (onNoteEvent) onNoteEvent(note, true);
    repaint();
}

void PianoKeyboard::mouseUp(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (mousePressedNote >= 0) {
        if (onNoteEvent) onNoteEvent(mousePressedNote, false);
        mousePressedNote = -1;
        repaint();
    }
}

void PianoKeyboard::mouseExit(const juce::MouseEvent& e) {
    juce::ignoreUnused(e);
    if (mousePressedNote >= 0) {
        if (onNoteEvent) onNoteEvent(mousePressedNote, false);
        mousePressedNote = -1;
        repaint();
    }
}

void PianoKeyboard::setStartNote(int note) {
    startNote = juce::jlimit(0, 108, note);
    rebuildKeys();
    repaint();
}

void PianoKeyboard::setNumWhiteKeys(int count) {
    numWhiteKeys = juce::jmax(1, count);
    rebuildKeys();
    repaint();
}

void PianoKeyboard::setKeyMapping(const std::map<int, juce::String>& mapping) {
    keyMappings = mapping;
    repaint();
}

void PianoKeyboard::setActiveKeys(const std::set<int>& keysIn) {
    activeKeys = keysIn;
    repaint();
}

void PianoKeyboard::rebuildKeys() {
    keys.clear();
    auto area = getLocalBounds().toFloat();
    if (area.isEmpty() || numWhiteKeys <= 0) return;

    int firstC = startNote;
    while (firstC % 12 != 0) firstC--;

    const float whiteKeyW = area.getWidth() / (float)numWhiteKeys;
    const float blackKeyW = whiteKeyW * 0.6f;
    const float blackKeyH = area.getHeight() * 0.62f;

    // White keys
    int whiteCount = 0;
    for (int n = firstC; whiteCount < numWhiteKeys; n++) {
        if (isBlackKey(n)) continue;
        KeyInfo k;
        k.note = n;
        k.isBlack = false;
        k.bounds = { area.getX() + (float)whiteCount * whiteKeyW, area.getY(), whiteKeyW, area.getHeight() };
        keys.push_back(k);
        whiteCount++;
    }

    // Black keys — FIX: Hanya gambar jika ada white key SEBELUM dan SESUDAHnya
    int lastWhiteNote = keys.empty() ? firstC : keys.back().note;
    for (int n = firstC; n < lastWhiteNote; n++) {
        if (!isBlackKey(n)) continue;
        int prevWhite = n - 1;
        for (auto& wk : keys) {
            if (!wk.isBlack && wk.note == prevWhite) {
                KeyInfo k;
                k.note = n;
                k.isBlack = true;
                k.bounds = { wk.bounds.getRight() - blackKeyW * 0.5f, area.getY(), blackKeyW, blackKeyH };
                keys.push_back(k);
                break;
            }
        }
    }
}

int PianoKeyboard::getNoteAtPosition(juce::Point<float> pos) const {
    for (auto& k : keys) if (k.isBlack && k.bounds.contains(pos)) return k.note;
    for (auto& k : keys) if (!k.isBlack && k.bounds.contains(pos)) return k.note;
    return -1;
}

bool PianoKeyboard::isBlackKey(int note) {
    int n = note % 12;
    return n == 1 || n == 3 || n == 6 || n == 8 || n == 10;
}

juce::String PianoKeyboard::getNoteName(int note) {
    static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    int oct = (note / 12) - 1;
    return juce::String(names[note % 12]) + juce::String(oct);
}