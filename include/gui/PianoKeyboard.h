#pragma once
#include <JuceHeader.h>
#include <map>
#include <set>
#include <vector>

class PianoKeyboard : public juce::Component {
public:
    PianoKeyboard();
    ~PianoKeyboard() override;
    
    void paint(juce::Graphics&) override;
    void resized() override;
    
    // Mouse interaction untuk tes tanpa piano fisik
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    
    void setStartNote(int note);
    int getStartNote() const { return startNote; }
    void setNumWhiteKeys(int count);
    void setKeyMapping(const std::map<int, juce::String>& mapping);
    void setActiveKeys(const std::set<int>& keys);

    // Callback saat tuts diklik
    std::function<void(int note, bool isPressed)> onNoteEvent;
    std::function<void(int note)> onNoteRightClick; // Untuk assign key nanti
    
private:
    struct KeyInfo {
        int note;
        bool isBlack;
        juce::Rectangle<float> bounds;
    };
    
    int startNote = 36; // C2
    int numWhiteKeys = 22;
    int mousePressedNote = -1;
    
    std::map<int, juce::String> keyMappings;
    std::set<int> activeKeys;
    std::vector<KeyInfo> keys;
    
    void rebuildKeys();
    int getNoteAtPosition(juce::Point<float> pos) const;
    static bool isBlackKey(int note);
    static juce::String getNoteName(int note);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKeyboard)
};