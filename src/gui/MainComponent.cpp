#include "gui/MainComponent.h"
#include <cstdio>
#include <cctype>

namespace Theme {
    const juce::Colour bg          { 0xff0f0f1a };
    const juce::Colour panelBg     { 0xff1a1a2e };
    const juce::Colour panelBorder { 0xff2a2a3e };
    const juce::Colour textPrimary { 0xffe0e0e0 };
    const juce::Colour textMuted   { 0xff888899 };
    const juce::Colour accent      { 0xffffa726 };
    const juce::Colour accentDark  { 0xffff8f00 };
}

MainComponent::MainComponent() {
    setOpaque(true);
    setupStyle();
    
    // Initial 21-key diatonic mapping (Genshin Windsong Lyre scale)
    noteToKeyMap = {
        {48, "Z"}, {50, "X"}, {52, "C"}, {53, "V"}, {55, "B"}, {57, "N"}, {59, "M"},
        {60, "A"}, {62, "S"}, {64, "D"}, {65, "F"}, {67, "G"}, {69, "H"}, {71, "J"},
        {72, "Q"}, {74, "W"}, {76, "E"}, {77, "R"}, {79, "T"}, {81, "Y"}, {83, "U"}
    };

    // Synchronize mapping with NoteTracker
    std::map<int, std::string> stdMap;
    for (const auto& [note, key] : noteToKeyMap) {
        stdMap[note] = key.toStdString();
    }
    noteTracker.setMapping(stdMap);
    // Same mapping drives the live MIDI -> PC key router (no duplicate system).
    keyRouter.setMapping(stdMap);

    // Route NoteTracker events to the audio engine
    noteTracker.onNoteOn = [this](int note, int velocity) {
        if (audioEngine) {
            audioEngine->noteOn(note, velocity);
        }
    };

    noteTracker.onNoteOff = [this](int note) {
        if (audioEngine) {
            audioEngine->noteOff(note);
        }
    };

    pianoKeyboard.setKeyMapping(noteToKeyMap);

    for (auto* b : {&tabMapping, &tabLatency, &tabSandbox, &tabGuide}) {
        b->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        b->setColour(juce::TextButton::textColourOffId, Theme::textMuted);
        b->setColour(juce::TextButton::textColourOnId, Theme::accent);
        addAndMakeVisible(b);
    }
    
    exportBtn.setColour(juce::TextButton::buttonColourId, Theme::accent);
    exportBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    addAndMakeVisible(exportBtn);
    
    for (auto* l : {&midiInputLabel, &channelLabel, &soundLabel}) {
        l->setColour(juce::Label::textColourId, Theme::textMuted);
        l->setFont(juce::Font(13.0f));
        addAndMakeVisible(l);
    }
    for (auto* c : {&midiInputCombo, &channelCombo, &soundCombo}) {
        c->setColour(juce::ComboBox::backgroundColourId, Theme::panelBg);
        c->setColour(juce::ComboBox::textColourId, Theme::textPrimary);
        c->setColour(juce::ComboBox::outlineColourId, Theme::panelBorder);
        addAndMakeVisible(c);
    }
    // Channel selection is real: id 1 = Omni, id 2..17 = channels 1..16
    channelCombo.addItem("Omni (All)", 1);
    for (int ch = 1; ch <= 16; ++ch) {
        channelCombo.addItem("Channel " + juce::String(ch), ch + 1);
    }
    channelCombo.setSelectedId(1);
    soundCombo.addItem("Acoustic Grand Piano", 1);
    soundCombo.setSelectedId(1);

    // Live MIDI controls (do not remove any existing controls)
    for (auto* b : {&midiOpenBtn, &panicBtn}) {
        b->setColour(juce::TextButton::buttonColourId, Theme::panelBg);
        b->setColour(juce::TextButton::textColourOffId, Theme::textPrimary);
        addAndMakeVisible(b);
    }
    panicBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff5a1a1a));
    panicBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    for (auto* l : {&midiStatusLabel, &lastMidiLabel}) {
        l->setColour(juce::Label::textColourId, Theme::textMuted);
        l->setFont(juce::Font(11.0f));
        addAndMakeVisible(l);
    }
    
    pianoTitle.setColour(juce::Label::textColourId, Theme::textPrimary);
    pianoTitle.setFont(juce::Font(16.0f, juce::Font::bold));
    addAndMakeVisible(pianoTitle);
    
    pianoSubtitle.setColour(juce::Label::textColourId, Theme::textMuted);
    pianoSubtitle.setFont(juce::Font(12.0f));
    addAndMakeVisible(pianoSubtitle);
    
    for (auto* b : {&octaveDownBtn, &octaveUpBtn, &keys49Btn, &keys61Btn, &keys88Btn}) {
        b->setColour(juce::TextButton::buttonColourId, Theme::panelBg);
        b->setColour(juce::TextButton::textColourOffId, Theme::textPrimary);
        addAndMakeVisible(b);
    }
    octaveLabel.setColour(juce::Label::textColourId, Theme::textPrimary);
    octaveLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(octaveLabel);
    
    keys61Btn.setColour(juce::TextButton::buttonColourId, Theme::accent);
    keys61Btn.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
    
    addAndMakeVisible(pianoKeyboard);
    
    for (auto* l : {&legendWhite, &legendBlack, &legendMapped}) {
        l->setColour(juce::Label::textColourId, Theme::textMuted);
        l->setFont(juce::Font(12.0f));
        addAndMakeVisible(l);
    }
    
    matrixTitle.setColour(juce::Label::textColourId, Theme::textPrimary);
    matrixTitle.setFont(juce::Font(15.0f, juce::Font::bold));
    addAndMakeVisible(matrixTitle);
    
    matrixSub.setColour(juce::Label::textColourId, Theme::textMuted);
    matrixSub.setFont(juce::Font(11.0f));
    addAndMakeVisible(matrixSub);
    
    activeKeysLabel.setColour(juce::Label::textColourId, Theme::accent);
    activeKeysLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    activeKeysLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(activeKeysLabel);

    sandboxEditor.setMultiLine(true);
    sandboxEditor.setColour(juce::TextEditor::backgroundColourId, Theme::panelBg);
    sandboxEditor.setColour(juce::TextEditor::textColourId, Theme::textPrimary);
    sandboxEditor.setColour(juce::TextEditor::outlineColourId, Theme::panelBorder);
    sandboxEditor.setFont(juce::Font(16.0f));
    sandboxEditor.setText("Klik di sini, lalu coba ketik teks secara normal (piano tidak akan berbunyi saat mengetik di sini). Klik area luar untuk kembali memainkan piano...");
    addChildComponent(sandboxEditor);
    
    // Enable keyboard focus on MainComponent
    setWantsKeyboardFocus(true);
    addKeyListener(this);
    pianoKeyboard.addKeyListener(this);
    juce::Desktop::getInstance().addFocusChangeListener(this);

    setupCallbacks();
    setupAudio();
    setupLiveMidi();
    switchTab(ActiveTab::Mapping);
    setSize(1320, 720);
}

MainComponent::~MainComponent() {
    // Stop input and release every OS key before anything is torn down.
    midiDeviceListConnection.reset();
    midiHandler.closeDevice();
    keyRouter.panic();

    juce::Desktop::getInstance().removeFocusChangeListener(this);
    removeKeyListener(this);
    pianoKeyboard.removeKeyListener(this);

    releaseAllNotes();

    if (audioEngine) {
        audioEngine->shutdown();
    }
}

void MainComponent::setupStyle() {
    juce::LookAndFeel_V4::setDefaultLookAndFeel(nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Audio Engine Setup
// ─────────────────────────────────────────────────────────────────────────────

juce::String MainComponent::resolveSoundFontPath() {
    auto exeDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    auto candidate = exeDir.getChildFile("GeneralUser-GS.sf2");
    if (candidate.existsAsFile()) return candidate.getFullPathName();

    candidate = exeDir.getChildFile("assets/sounds/GeneralUser-GS.sf2");
    if (candidate.existsAsFile()) return candidate.getFullPathName();

    juce::File srcRoot = exeDir;
    for (int depth = 0; depth < 6; ++depth) {
        auto sf2 = srcRoot.getChildFile("assets/sounds/GeneralUser-GS.sf2");
        if (sf2.existsAsFile()) return sf2.getFullPathName();
        srcRoot = srcRoot.getParentDirectory();
    }

    return {};
}

void MainComponent::setupAudio() {
    audioToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e4d2b));
    audioToggleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff4caf50));
    addAndMakeVisible(audioToggleBtn);

    audioEngine = std::make_unique<Core::PianoAudioEngine>();

    juce::String sf2Path = resolveSoundFontPath();
    if (sf2Path.isEmpty()) {
        fprintf(stderr, "[MainComponent] SoundFont not found — audio disabled.\n");
        audioToggleBtn.setButtonText("🔇 Audio: No SF2");
        audioToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a1a1a));
        audioToggleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffef5350));
        audioToggleBtn.setEnabled(false);
        return;
    }

    bool ok = audioEngine->initialise(sf2Path.toStdString());
    if (!ok) {
        fprintf(stderr, "[MainComponent] FluidSynth init failed — audio disabled.\n");
        audioToggleBtn.setButtonText("🔇 Audio: Error");
        audioToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3a1a1a));
        audioToggleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffef5350));
        audioToggleBtn.setEnabled(false);
        return;
    }

    audioToggleBtn.onClick = [this] {
        bool nowEnabled = !audioEngine->isEnabled();
        if (!nowEnabled) {
            // Silence all active notes when turning audio off
            releaseAllNotes();
        }
        audioEngine->setEnabled(nowEnabled);

        if (nowEnabled) {
            audioToggleBtn.setButtonText("🔊 Audio: ON");
            audioToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e4d2b));
            audioToggleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff4caf50));
        } else {
            audioToggleBtn.setButtonText("🔇 Audio: OFF");
            audioToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
            audioToggleBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888899));
        }
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Live MIDI Input
// ─────────────────────────────────────────────────────────────────────────────

void MainComponent::setupLiveMidi() {
    // AsyncUpdater guarantees this callback runs on the message thread.
    midiHandler.setEventCallback([this](const Core::MidiNoteEvent& event) {
        handleMidiEvent(event);
    });

    // Hot-plug / disconnect notifications may arrive on a system thread.
    // SafePointer prevents a queued lambda from touching a destroyed component.
    midiDeviceListConnection = juce::MidiDeviceListConnection::make(
        [safeThis = juce::Component::SafePointer<MainComponent>(this)] {
            juce::MessageManager::callAsync([safeThis] {
                if (auto* self = safeThis.getComponent()) self->refreshMidiDevices();
            });
        });

    midiOpenBtn.onClick = [this] {
        if (midiHandler.isConnected()) closeMidiDevice();
        else openSelectedMidiDevice();
    };

    panicBtn.onClick = [this] {
        panicAll();
        midiStatusLabel.setText("MIDI: panic - all keys released",
                                juce::dontSendNotification);
    };

    midiInputCombo.onChange = [this] {
        // A device switch must never leave a key held down.
        if (midiHandler.isConnected() || keyRouter.getPressedKeyCount() > 0) {
            panicAll();
        }
        if (midiInputCombo.getSelectedId() <= 1) closeMidiDevice();
        else                                     openSelectedMidiDevice();
    };

    channelCombo.onChange = [this] {
        const int id = channelCombo.getSelectedId();
        midiHandler.setChannelFilter(id <= 1 ? 0 : id - 1);
        updateMidiStatus();
    };

    refreshMidiDevices();
}

void MainComponent::refreshMidiDevices() {
    availableMidiDevices = midiHandler.getAvailableDevices();
    const juce::String previouslyConnected = midiHandler.getConnectedIdentifier();

    midiInputCombo.clear();
    midiInputCombo.addItem(availableMidiDevices.isEmpty()
                               ? "No MIDI devices detected"
                               : "-- Select MIDI input --", 1);

    bool connectedStillPresent = false;
    int selectedId = 1;
    for (int i = 0; i < availableMidiDevices.size(); ++i) {
        const int id = i + 2;
        midiInputCombo.addItem(availableMidiDevices[i].name, id);
        if (availableMidiDevices[i].identifier == previouslyConnected) {
            connectedStillPresent = true;
            selectedId = id;
        }
    }

    if (!previouslyConnected.isEmpty() && !connectedStillPresent) {
        // Physical device unplugged: release everything before closing.
        keyRouter.panic();
        noteTracker.releaseAllNotes();
        if (audioEngine) audioEngine->allNotesOff();
        midiHandler.closeDevice();
        connectedMidiIdentifier.clear();
        midiOpenError.clear();
        syncVisualKeys();
        midiStatusLabel.setText("MIDI: device disconnected - keys released",
                                juce::dontSendNotification);
    }

    midiInputCombo.setSelectedId(selectedId, juce::dontSendNotification);
    updateMidiStatus();
}

void MainComponent::openSelectedMidiDevice() {
    const int id = midiInputCombo.getSelectedId();
    if (id <= 1 || id - 2 >= availableMidiDevices.size()) {
        midiOpenError = "no device selected";
        updateMidiStatus();
        return;
    }

    const auto info = availableMidiDevices[id - 2];

    // Never carry pressed keys across a (re)connect.
    keyRouter.panic();
    noteTracker.releaseAllNotes();
    if (audioEngine) audioEngine->allNotesOff();
    midiHandler.closeDevice();

    if (!midiHandler.openDevice(info.identifier)) {
        // WinMM/Yamaha reports MMSYSERR_NOMEM (7) when the port is held by
        // another process (verified: browser Web MIDI / another app), not when
        // RAM is actually exhausted. Store the actionable cause for the status.
        midiOpenError = "port unavailable - '" + info.name +
                        "' may be open in another app; close it and retry";
        updateMidiStatus();
        return;
    }

    midiOpenError.clear();
    connectedMidiIdentifier = info.identifier;
    midiStatusLabel.setText("MIDI: connected to " + info.name,
                            juce::dontSendNotification);
    updateMidiStatus();
}

void MainComponent::closeMidiDevice() {
    if (midiHandler.isConnected()) {
        keyRouter.panic();
        noteTracker.releaseAllNotes();
        if (audioEngine) audioEngine->allNotesOff();
        syncVisualKeys();
    }
    midiHandler.closeDevice();
    connectedMidiIdentifier.clear();
    midiOpenError.clear();
    updateMidiStatus();
}

void MainComponent::panicAll() {
    keyRouter.panic();                 // OS key-up for every key Keydium pressed
    noteTracker.releaseAllNotes();     // clear note state for all sources
    if (audioEngine) audioEngine->allNotesOff();
    syncVisualKeys();
    updateMidiStatus();
}

void MainComponent::handleMidiEvent(const Core::MidiNoteEvent& event) {
    // Message thread only. NoteTracker keeps the existing note mapping, audio
    // monitoring and virtual-piano highlighting driven by real MIDI events.
    if (event.noteOn) {
        noteTracker.noteOn(event.note, Core::NoteSource::MidiDevice, event.velocity);
        keyRouter.noteOn(event.note, event.velocity);
    } else {
        noteTracker.noteOff(event.note, Core::NoteSource::MidiDevice);
        keyRouter.noteOff(event.note);
    }
    syncVisualKeys();

    auto it = noteToKeyMap.find(event.note);
    const juce::String keyName = it != noteToKeyMap.end() ? it->second : "--";
    lastMidiLabel.setText("Last: " + juce::String(event.note) + " " +
                          (event.noteOn ? "ON" : "OFF") + " v" +
                          juce::String(event.velocity) + " ch" +
                          juce::String(event.channel) + " -> " + keyName,
                          juce::dontSendNotification);
    updateMidiStatus();
}

void MainComponent::updateMidiStatus() {
    const juce::String held = juce::String(keyRouter.getPressedKeyCount());
    if (midiHandler.isConnected()) {
        const int ch = midiHandler.getChannelFilter();
        const juce::String channelText = (ch == 0)
            ? "Omni" : ("CH " + juce::String(ch));
        midiStatusLabel.setText("MIDI: " + midiHandler.getConnectedDeviceName() +
                                " | " + channelText + " | keys held: " + held,
                                juce::dontSendNotification);
        midiOpenBtn.setButtonText("Close MIDI");
    } else {
        if (midiOpenError.isNotEmpty()) {
            midiStatusLabel.setText("MIDI: " + midiOpenError +
                                    " | keys held: " + held,
                                    juce::dontSendNotification);
        } else {
            midiStatusLabel.setText("MIDI: disconnected | keys held: " + held,
                                    juce::dontSendNotification);
        }
        midiOpenBtn.setButtonText("Open MIDI");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Physical Keyboard & Focus Handling
// ─────────────────────────────────────────────────────────────────────────────

bool MainComponent::isTextInputActive() const {
    if (juce::Component::getCurrentlyModalComponent() != nullptr)
        return true;

    auto* focused = juce::Component::getCurrentlyFocusedComponent();
    if (focused != nullptr) {
        if (dynamic_cast<juce::TextInputTarget*>(focused) != nullptr)
            return true;
        if (dynamic_cast<juce::TextEditor*>(focused) != nullptr)
            return true;
    }
    return false;
}

void MainComponent::syncVisualKeys() {
    auto activeNotes = noteTracker.getActiveNotes();
    pianoKeyboard.setActiveKeys(activeNotes);

    activePcKeys.clear();
    for (const auto& keyName : noteTracker.getActiveKeyNames()) {
        activePcKeys.insert(juce::String(keyName));
    }

    activeKeysLabel.setText("Active Keys: " + juce::String((int)activeNotes.size()), juce::dontSendNotification);
    repaint(matrixArea);
}

bool MainComponent::handleKeyPress(const juce::KeyPress& key) {
    if (isTextInputActive()) {
        return false;
    }

    // Determine key character
    auto ch = key.getTextCharacter();
    std::string keyChar;
    if (ch >= 32) {
        keyChar = juce::String::charToString(ch).toUpperCase().toStdString();
    }
    int code = key.getKeyCode();
    if (keyChar.empty() && code >= 32 && code <= 126) {
        keyChar = juce::String::charToString((juce::juce_wchar) std::toupper(code)).toStdString();
    }

    bool handled = noteTracker.processKeyPress(code, keyChar, 100);
    if (handled) {
        syncVisualKeys();
    }
    return handled;
}

bool MainComponent::handleKeyStateChange(bool isKeyDown) {
    juce::ignoreUnused(isKeyDown);
    if (isTextInputActive()) {
        releaseAllPhysicalKeys();
        return false;
    }

    noteTracker.processKeyRelease([](int keyCode) {
        return juce::KeyPress::isKeyCurrentlyDown(keyCode);
    });
    syncVisualKeys();
    return false;
}

bool MainComponent::keyPressed(const juce::KeyPress& key) {
    return handleKeyPress(key);
}

bool MainComponent::keyStateChanged(bool isKeyDown) {
    return handleKeyStateChange(isKeyDown);
}

bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) {
    juce::ignoreUnused(originatingComponent);
    return handleKeyPress(key);
}

bool MainComponent::keyStateChanged(bool isKeyDown, juce::Component* originatingComponent) {
    juce::ignoreUnused(originatingComponent);
    return handleKeyStateChange(isKeyDown);
}

void MainComponent::focusLost(FocusChangeType cause) {
    juce::ignoreUnused(cause);
    releaseAllPhysicalKeys();
}

void MainComponent::globalFocusChanged(juce::Component* focusedComponent) {
    juce::ignoreUnused(focusedComponent);
    if (isTextInputActive()) {
        releaseAllPhysicalKeys();
    }
}

void MainComponent::releaseAllPhysicalKeys() {
    noteTracker.releaseAllPhysicalKeys();
    syncVisualKeys();
}

void MainComponent::releaseAllNotes() {
    noteTracker.releaseAllNotes();
    if (audioEngine) {
        audioEngine->allNotesOff();
    }
    syncVisualKeys();
}

// ─────────────────────────────────────────────────────────────────────────────
// UI Callbacks
// ─────────────────────────────────────────────────────────────────────────────

void MainComponent::setupCallbacks() {
    tabMapping.onClick = [this] { switchTab(ActiveTab::Mapping); };
    tabLatency.onClick = [this] { switchTab(ActiveTab::Latency); };
    tabSandbox.onClick = [this] { switchTab(ActiveTab::Sandbox); };
    tabGuide.onClick   = [this] { switchTab(ActiveTab::Guide); };

    octaveDownBtn.onClick = [this] {
        pianoKeyboard.setStartNote(pianoKeyboard.getStartNote() - 12);
        int oct = (pianoKeyboard.getStartNote() / 12) - 1;
        octaveLabel.setText("Octave " + juce::String(oct), juce::dontSendNotification);
    };

    octaveUpBtn.onClick = [this] {
        pianoKeyboard.setStartNote(pianoKeyboard.getStartNote() + 12);
        int oct = (pianoKeyboard.getStartNote() / 12) - 1;
        octaveLabel.setText("Octave " + juce::String(oct), juce::dontSendNotification);
    };

    keys49Btn.onClick = [this] { setKeyCountMode(29, keys49Btn); };
    keys61Btn.onClick = [this] { setKeyCountMode(36, keys61Btn); };
    keys88Btn.onClick = [this] { setKeyCountMode(52, keys88Btn); };

    // Mouse events on virtual piano keybed route through NoteTracker
    pianoKeyboard.onNoteEvent = [this](int note, bool isPressed) {
        if (isPressed) {
            noteTracker.noteOn(note, Core::NoteSource::Mouse, 100);
        } else {
            noteTracker.noteOff(note, Core::NoteSource::Mouse);
        }
        syncVisualKeys();
    };

    pianoKeyboard.onNoteRightClick = [this](int note) {
        promptAssignKey(note);
    };
}

void MainComponent::setKeyCountMode(int whiteKeys, juce::TextButton& activeBtn) {
    pianoKeyboard.setNumWhiteKeys(whiteKeys);
    for (auto* b : {&keys49Btn, &keys61Btn, &keys88Btn}) {
        b->setColour(juce::TextButton::buttonColourId, Theme::panelBg);
        b->setColour(juce::TextButton::textColourOffId, Theme::textPrimary);
    }
    activeBtn.setColour(juce::TextButton::buttonColourId, Theme::accent);
    activeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::black);
}

void MainComponent::switchTab(ActiveTab newTab) {
    currentTab = newTab;
    releaseAllPhysicalKeys();

    tabMapping.setColour(juce::TextButton::textColourOffId, currentTab == ActiveTab::Mapping ? Theme::accent : Theme::textMuted);
    tabLatency.setColour(juce::TextButton::textColourOffId, currentTab == ActiveTab::Latency ? Theme::accent : Theme::textMuted);
    tabSandbox.setColour(juce::TextButton::textColourOffId, currentTab == ActiveTab::Sandbox ? Theme::accent : Theme::textMuted);
    tabGuide.setColour(juce::TextButton::textColourOffId,   currentTab == ActiveTab::Guide   ? Theme::accent : Theme::textMuted);

    bool isMapping = (currentTab == ActiveTab::Mapping);
    bool isSandbox = (currentTab == ActiveTab::Sandbox);

    for (auto* c : std::initializer_list<juce::Component*>{
        &pianoTitle, &pianoSubtitle, &octaveDownBtn, &octaveLabel, &octaveUpBtn,
        &keys49Btn, &keys61Btn, &keys88Btn, &pianoKeyboard,
        &legendWhite, &legendBlack, &legendMapped
    }) {
        c->setVisible(isMapping);
    }

    sandboxEditor.setVisible(isSandbox);
    if (isMapping) {
        grabKeyboardFocus();
    }
    repaint();
}

void MainComponent::promptAssignKey(int midiNote) {
    releaseAllPhysicalKeys();

    auto* alert = new juce::AlertWindow("Assign PC Key",
        "Masukkan 1 karakter tombol keyboard untuk MIDI Note " + juce::String(midiNote) + " (kosongkan untuk hapus):",
        juce::MessageBoxIconType::NoIcon);
    
    juce::String currentKey = noteToKeyMap.count(midiNote) ? noteToKeyMap[midiNote] : "";
    alert->addTextEditor("keyInput", currentKey, "Tombol (misal Q, W, E, 1):");
    alert->addButton("Simpan", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alert->addButton("Batal", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert, midiNote](int result) {
        if (result == 1) {
            auto text = alert->getTextEditorContents("keyInput").trim().toUpperCase();
            if (text.isEmpty()) {
                noteToKeyMap.erase(midiNote);
                noteTracker.clearNoteMapping(midiNote);
            } else {
                noteToKeyMap[midiNote] = text.substring(0, 1);
                noteTracker.setNoteMapping(midiNote, text.substring(0, 1).toStdString());
            }

            pianoKeyboard.setKeyMapping(noteToKeyMap);

            // Keep both mapping consumers in sync with the single source of truth.
            std::map<int, std::string> updatedMap;
            for (const auto& [n, k] : noteToKeyMap) {
                updatedMap[n] = k.toStdString();
            }
            keyRouter.setMapping(updatedMap);

            syncVisualKeys();
            repaint();
        }
        delete alert;
        grabKeyboardFocus();
    }));
}

void MainComponent::drawKeyboardMatrix(juce::Graphics& g, juce::Rectangle<int> bounds) {
    const std::vector<std::vector<juce::String>> rows = {
        {"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P"},
        {"A", "S", "D", "F", "G", "H", "J", "K", "L"},
        {"Z", "X", "C", "V", "B", "N", "M"}
    };

    int keyW = 44;
    int keyH = 26;
    int gap  = 5;

    for (size_t r = 0; r < rows.size(); ++r) {
        int rowWidth = (int)rows[r].size() * (keyW + gap) - gap;
        int startX = bounds.getX() + (bounds.getWidth() - rowWidth) / 2;
        int y = bounds.getY() + (int)r * (keyH + gap);

        for (size_t c = 0; c < rows[r].size(); ++c) {
            auto keyName = rows[r][c];
            auto keyRect = juce::Rectangle<float>((float)(startX + c * (keyW + gap)),
                                                  (float)y,
                                                  (float)keyW, (float)keyH);

            bool isTriggered = activePcKeys.count(keyName) > 0;
            bool isMapped = false;
            for (const auto& pair : noteToKeyMap) {
                if (pair.second == keyName) { isMapped = true; break; }
            }

            g.setColour(isTriggered ? Theme::accent
                                    : (isMapped ? Theme::panelBg.brighter(0.15f) : Theme::panelBg));
            g.fillRoundedRectangle(keyRect, 5.0f);

            g.setColour(isTriggered ? juce::Colours::white
                                    : (isMapped ? Theme::accent.withAlpha(0.6f) : Theme::panelBorder));
            g.drawRoundedRectangle(keyRect, 5.0f, 1.2f);

            g.setColour(isTriggered ? juce::Colours::black
                                    : (isMapped ? Theme::textPrimary : Theme::textMuted));
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(keyName, keyRect, juce::Justification::centred);
        }
    }
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(Theme::bg);
    g.setColour(Theme::bg);
    g.fillRect(getLocalBounds());
    
    // Top bar
    g.setColour(Theme::panelBg);
    g.fillRect(0, 0, getWidth(), 60);
    g.setColour(Theme::panelBorder);
    g.drawHorizontalLine(60, 0, (float)getWidth());
    
    // Logo vektor
    g.setColour(Theme::accent);
    g.fillRoundedRectangle(20, 15, 30, 30, 6.0f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(28.0f, 33.0f, 7.0f, 5.5f);
    g.fillRect  (33.5f, 21.0f, 2.0f, 14.0f);
    g.fillRect  (33.5f, 21.0f, 6.0f, 2.5f);
    
    g.setColour(Theme::textPrimary);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("MidiKeyMapper Studio", 60, 10, 300, 40, juce::Justification::centredLeft);

    // Bullet legend vektor
    if (currentTab == ActiveTab::Mapping) {
        auto drawBullet = [&](juce::Label& lbl, juce::Colour col, bool filled) {
            auto b = lbl.getBounds().toFloat();
            juce::Rectangle<float> dot(b.getX() - 14.0f, b.getCentreY() - 4.5f, 9.0f, 9.0f);
            g.setColour(col);
            if (filled) g.fillEllipse(dot);
            else        g.drawEllipse(dot, 1.3f);
        };
        drawBullet(legendWhite,  juce::Colours::white, false);
        drawBullet(legendBlack,  Theme::textMuted,     true);
        drawBullet(legendMapped, Theme::accent,        true);
    }

    if (currentTab == ActiveTab::Latency) {
        g.setColour(Theme::textMuted);
        g.setFont(juce::Font(16.0f));
        g.drawText("Latency & Delay Timer UI — Siap diisi grafik jitter & konfigurasi hold duration.",
                   getLocalBounds().withTrimmedTop(120).withTrimmedBottom(170),
                   juce::Justification::centred);
    } else if (currentTab == ActiveTab::Guide) {
        g.setColour(Theme::textMuted);
        g.setFont(juce::Font(15.0f));
        g.drawText("Panduan:\n1. Tekan tombol keyboard fisik (A-Z) untuk memainkan nada piano langsung.\n2. Klik kiri tuts piano virtual untuk tes output matriks.\n3. Klik kanan tuts piano untuk mengubah mapping tombol keyboard.\n4. Gunakan Export Native Script untuk menghasilkan script AHK / xdotool.",
                   getLocalBounds().withTrimmedTop(120).withTrimmedBottom(170),
                   juce::Justification::centred);
    }
    
    // Separator bottom
    g.setColour(Theme::panelBorder);
    g.drawHorizontalLine(getHeight() - 170, 0, (float)getWidth());
    drawKeyboardMatrix(g, matrixArea);
}

void MainComponent::resized() {
    auto area = getLocalBounds();
    
    // TOP BAR (60px)
    auto topBar = area.removeFromTop(60);
    auto tabs = topBar.withTrimmedLeft(360).withTrimmedRight(200);
    int tabW = tabs.getWidth() / 4;
    tabMapping.setBounds(tabs.removeFromLeft(tabW));
    tabLatency.setBounds(tabs.removeFromLeft(tabW));
    tabSandbox.setBounds(tabs.removeFromLeft(tabW));
    tabGuide.setBounds(tabs);
    exportBtn.setBounds(topBar.removeFromRight(200).reduced(10, 12));
    
    // MIDI CONTROL BAR (50px)
    auto midiBar = area.removeFromTop(50).reduced(20, 10);
    midiInputLabel.setBounds(midiBar.removeFromLeft(80));
    midiInputCombo.setBounds(midiBar.removeFromLeft(220));
    midiBar.removeFromLeft(20);
    channelLabel.setBounds(midiBar.removeFromLeft(65));
    channelCombo.setBounds(midiBar.removeFromLeft(120));
    midiBar.removeFromLeft(20);
    soundLabel.setBounds(midiBar.removeFromLeft(50));
    soundCombo.setBounds(midiBar.removeFromLeft(180));

    // Right-hand controls: status, connect, panic, audio toggle
    audioToggleBtn.setBounds(midiBar.removeFromRight(130).reduced(4, 0));
    panicBtn.setBounds(midiBar.removeFromRight(70).reduced(4, 0));
    midiOpenBtn.setBounds(midiBar.removeFromRight(80).reduced(4, 0));
    midiStatusLabel.setBounds(midiBar.removeFromRight(200).reduced(4, 0));
    
    // BOTTOM MATRIX PANEL (170px)
    auto bottomPanel = area.removeFromBottom(170);
    bottomPanel.reduce(20, 8);
    auto bottomHeader = bottomPanel.removeFromTop(24);
    matrixTitle.setBounds(bottomHeader.removeFromLeft(300));
    activeKeysLabel.setBounds(bottomHeader.removeFromRight(120));
    lastMidiLabel.setBounds(bottomHeader.removeFromRight(280));
    matrixSub.setBounds(bottomPanel.removeFromTop(16));
    matrixArea = bottomPanel.reduced(0, 4);
    
    // MAIN CONTENT AREA
    auto mainContent = area.reduced(20, 10);
    sandboxEditor.setBounds(mainContent.reduced(10));

    auto panelHeader = mainContent.removeFromTop(40);
    panelHeader.reduce(15, 0);
    pianoTitle.setBounds(panelHeader.removeFromLeft(200));
    pianoSubtitle.setBounds(panelHeader.removeFromLeft(260));
    
    auto rightControls = panelHeader.removeFromRight(140);
    octaveDownBtn.setBounds(rightControls.removeFromLeft(30));
    octaveLabel.setBounds(rightControls.removeFromLeft(80));
    octaveUpBtn.setBounds(rightControls.removeFromLeft(30));
    
    auto keyToggle = mainContent.removeFromTop(30).withTrimmedRight(15);
    auto toggleRight = keyToggle.removeFromRight(240);
    int kw = toggleRight.getWidth() / 3;
    keys49Btn.setBounds(toggleRight.removeFromLeft(kw).reduced(2));
    keys61Btn.setBounds(toggleRight.removeFromLeft(kw).reduced(2));
    keys88Btn.setBounds(toggleRight.reduced(2));
    
    // PIANO KEYBOARD
    auto keyboardArea = mainContent.reduced(20, 10);
    auto legendRow = keyboardArea.removeFromBottom(25).withTrimmedLeft(16);
    legendWhite.setBounds(legendRow.removeFromLeft(110));
    legendBlack.setBounds(legendRow.removeFromLeft(110));
    legendMapped.setBounds(legendRow.removeFromLeft(230));
    
    pianoKeyboard.setBounds(keyboardArea);
}
