#pragma once
#include <JuceHeader.h>
#include <map>
#include <set>
#include <memory>
#include "PianoKeyboard.h"
#include "core/PianoAudioEngine.h"
#include "core/NoteTracker.h"
#include "core/MidiInputHandler.h"
#include "core/NoteToKeyRouter.h"

class MainComponent : public juce::Component,
                      public juce::KeyListener,
                      public juce::FocusChangeListener {
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Component keyboard & focus overrides
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
    void focusLost(FocusChangeType cause) override;

    // KeyListener overrides
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) override;

    // FocusChangeListener override
    void globalFocusChanged(juce::Component* focusedComponent) override;

    // Release all physical keyboard notes (e.g. window deactivated)
    void releaseAllPhysicalKeys();

    // Release all notes from all sources (e.g. audio toggled off)
    void releaseAllNotes();

private:
    enum class ActiveTab { Mapping, Latency, Sandbox, Guide };
    ActiveTab currentTab = ActiveTab::Mapping;

    // Tab buttons
    juce::TextButton tabMapping   {"Mapping & Piano GUI"};
    juce::TextButton tabLatency   {"Latency & Delay Timer"};
    juce::TextButton tabSandbox   {"Game & Typing Sandbox"};
    juce::TextButton tabGuide     {"Guide"};
    juce::TextButton exportBtn    {"Export Native Script"};
    
    // MIDI controls
    juce::Label      midiInputLabel {"", "MIDI Input:"};
    juce::ComboBox   midiInputCombo;
    juce::Label      channelLabel   {"", "Channel:"};
    juce::ComboBox   channelCombo;
    juce::Label      soundLabel     {"", "Sound:"};
    juce::ComboBox   soundCombo;

    // Live MIDI controls
    juce::TextButton midiOpenBtn  {"Open MIDI"};
    juce::TextButton panicBtn     {"PANIC"};
    juce::Label      midiStatusLabel {"", "MIDI: disconnected"};
    juce::Label      lastMidiLabel   {"", "Last: --"};
    
    // Piano panel header
    juce::Label      pianoTitle   {"", "Interactive Piano Keybed"};
    juce::Label      pianoSubtitle{"", "Left-click to test | Right-click to assign key"};
    juce::TextButton octaveDownBtn {"<"};
    juce::TextButton octaveUpBtn   {">"};
    juce::Label      octaveLabel   {"", "Octave 2"};
    juce::TextButton keys49Btn {"49-Key"};
    juce::TextButton keys61Btn {"61-Key"};
    juce::TextButton keys88Btn {"88-Key"};
    
    PianoKeyboard pianoKeyboard;
    
    // Legend — bullet digambar vektor di paint(), string di sini plain ASCII
    juce::Label legendWhite {"", "White Keys"};
    juce::Label legendBlack {"", "Black Keys"};
    juce::Label legendMapped{"", "Active / Mapped Key Badge"};
    
    // Bottom keyboard matrix panel
    juce::Label matrixTitle {"", "Simulated PC Keyboard Output Matrix"};
    juce::Label matrixSub   {"", "Keys light up when triggered by your piano"};
    juce::Label activeKeysLabel {"", "Active Keys: 0"};

    // Sandbox / Placeholder components untuk tab lain
    juce::TextEditor sandboxEditor;
    
    // State mapping & tombol aktif
    std::map<int, juce::String> noteToKeyMap;
    std::set<juce::String> activePcKeys;
    juce::Rectangle<int> matrixArea;

    // ── Note Tracker & Input Routing ──────────────────────────────────────────
    Core::NoteTracker noteTracker;

    // ── Audio Engine ──────────────────────────────────────────────────────────
    std::unique_ptr<Core::PianoAudioEngine> audioEngine;

    // ── Live MIDI Input pipeline ──────────────────────────────────────────────
    Core::MidiInputHandler midiHandler;
    Core::NoteToKeyRouter  keyRouter;
    juce::MidiDeviceListConnection midiDeviceListConnection;
    juce::Array<juce::MidiDeviceInfo> availableMidiDevices;
    juce::String connectedMidiIdentifier;
    juce::String midiOpenError;

    // Audio toggle button
    juce::TextButton audioToggleBtn {"🔊 Audio: ON"};

    // Helper to resolve the SoundFont path at runtime.
    static juce::String resolveSoundFontPath();

    bool handleKeyPress(const juce::KeyPress& key);
    bool handleKeyStateChange(bool isKeyDown);
    bool isTextInputActive() const;
    void syncVisualKeys();

    void setupStyle();
    void setupCallbacks();
    void setupAudio();
    void setupLiveMidi();
    void refreshMidiDevices();
    void openSelectedMidiDevice();
    void closeMidiDevice();
    void panicAll();
    void handleMidiEvent(const Core::MidiNoteEvent& event);
    void updateMidiStatus();
    void switchTab(ActiveTab newTab);
    void setKeyCountMode(int whiteKeys, juce::TextButton& activeBtn);
    void drawKeyboardMatrix(juce::Graphics& g, juce::Rectangle<int> bounds);
    void promptAssignKey(int midiNote);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
