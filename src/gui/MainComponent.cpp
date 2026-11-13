#include "gui/MainComponent.h"

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
    
    noteToKeyMap = {
        {48, "Z"}, {50, "X"}, {52, "C"}, {53, "V"}, {55, "B"}, {57, "N"}, {59, "M"},
        {60, "A"}, {62, "S"}, {64, "D"}, {65, "F"}, {67, "G"}, {69, "H"}, {71, "J"},
        {72, "Q"}, {74, "W"}, {76, "E"}, {77, "R"}, {79, "T"}, {81, "Y"}, {83, "U"}
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
    midiInputCombo.addItem("All Connected MIDI Devices (0)", 1);
    midiInputCombo.setSelectedId(1);
    channelCombo.addItem("Omni (All)", 1);
    channelCombo.setSelectedId(1);
    soundCombo.addItem("Acoustic Grand Piano", 1);
    soundCombo.setSelectedId(1);
    
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
    sandboxEditor.setText("Klik di sini, lalu coba tekan tuts piano virtual (atau hubungkan piano MIDI nanti) untuk melihat huruf yang diketik...");
    addChildComponent(sandboxEditor);
    
    setupCallbacks();
    switchTab(ActiveTab::Mapping);
    setSize(1200, 680);
}

MainComponent::~MainComponent() {}

void MainComponent::setupStyle() {
    juce::LookAndFeel_V4::setDefaultLookAndFeel(nullptr);
}

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

    pianoKeyboard.onNoteEvent = [this](int note, bool isPressed) {
        auto it = noteToKeyMap.find(note);
        if (it != noteToKeyMap.end()) {
            if (isPressed) activePcKeys.insert(it->second);
            else           activePcKeys.erase(it->second);
            
            activeKeysLabel.setText("Active Keys: " + juce::String((int)activePcKeys.size()), juce::dontSendNotification);
            repaint(matrixArea);
        }
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
    repaint();
}

void MainComponent::promptAssignKey(int midiNote) {
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
            if (text.isEmpty())
                noteToKeyMap.erase(midiNote);
            else
                noteToKeyMap[midiNote] = text.substring(0, 1);

            pianoKeyboard.setKeyMapping(noteToKeyMap);
            repaint();
        }
        delete alert;
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
    
    // Logo vektor (not balok) — bukan karakter ♪
    g.setColour(Theme::accent);
    g.fillRoundedRectangle(20, 15, 30, 30, 6.0f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(28.0f, 33.0f, 7.0f, 5.5f);
    g.fillRect  (33.5f, 21.0f, 2.0f, 14.0f);
    g.fillRect  (33.5f, 21.0f, 6.0f, 2.5f);
    
    g.setColour(Theme::textPrimary);
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.drawText("MidiKeyMapper Studio", 60, 10, 300, 40, juce::Justification::centredLeft);

    // Bullet legend vektor (menggantikan \u25CB / \u25CF)
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
        g.drawText("Panduan:\n1. Klik kiri tuts piano virtual untuk tes output matriks.\n2. Klik kanan tuts piano untuk mengubah mapping tombol keyboard.\n3. Gunakan Export Native Script untuk menghasilkan script AHK / xdotool.",
                   getLocalBounds().withTrimmedTop(120).withTrimmedBottom(170),
                   juce::Justification::centred);
    }
    
    // Separator bottom — sinkron dengan removeFromBottom(170) di resized()
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
    
    // BOTTOM MATRIX PANEL (170px)
    auto bottomPanel = area.removeFromBottom(170);
    bottomPanel.reduce(20, 8);
    auto bottomHeader = bottomPanel.removeFromTop(24);
    matrixTitle.setBounds(bottomHeader.removeFromLeft(300));
    activeKeysLabel.setBounds(bottomHeader.removeFromRight(120));
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