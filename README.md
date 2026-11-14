<div align="center">

# 🎹 Keydium

**Your MIDI. Your Keys.**

*A universal, low-latency MIDI-to-keyboard bridge for games and desktop applications.*

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=c%2B%2B&logoColor=white)
![JUCE](https://img.shields.io/badge/JUCE-7.0.9-8CC63F?style=flat-square)
![CMake](https://img.shields.io/badge/CMake-3.22%2B-064F8C?style=flat-square&logo=cmake&logoColor=white)
![Status](https://img.shields.io/badge/Status-Early_Development-orange?style=flat-square)

</div>

---

**Keydium** is a desktop application designed to translate MIDI signals into simulated computer keyboard inputs. Whether performing live on a digital piano or automating playback from standard `.mid` files, Keydium acts as a seamless bridge to control games and software that rely solely on keyboard inputs.

> [!NOTE]
> **Early Prototype Notice:** The current repository contains an initial GUI prototype. While the virtual piano and mapping interface are functional for visual inspection, the backend pipeline for physical MIDI input and OS-level keyboard simulation is still under active development.

---

## 🎯 Operating Modes

### 1. Live MIDI Input
Connect a physical MIDI controller or digital piano to play directly in real time. Incoming MIDI note events are intercepted and translated into corresponding keystrokes based on your active mapping profile.
* **Genshin Impact** — Perform on the *Windsong Lyre* using a real piano.
* **Roblox** — Play in-game virtual pianos with full hardware tactile control.
* **Custom Software** — Map MIDI keys to shortcuts, macros, or other keyboard-driven apps.

### 2. MIDI File Auto Play *(Planned)*
Load standard `.mid` files and let Keydium perform them automatically via simulated keystrokes.
* **File Parsing:** Load and inspect multi-track `.mid` files.
* **Accurate Timing:** Handle note durations, tempo, and playback controls.
* **Profile-Driven:** Output uses the exact same mapping rules as live performance.

---

## ✨ Key Features

* **🎹 Interactive Virtual Piano:** Inspect and test note assignments visually with support for **49, 61, and 88-key** layouts, octave shifting, and live keystroke feedback.
* **🎛️ Custom Key Mapping:** Bind any MIDI note to a specific keyboard key with an intuitive per-note configuration interface.
* **🎮 Dedicated Game Profiles:** Switch seamlessly between tailored presets or build your own configurations:

| Profile | Target Application | Description |
| :--- | :--- | :--- |
| **Genshin Impact** | Windsong Lyre | Optimized layout matching the 21-key in-game lyre scale. |
| **Roblox** | Virtual Pianos | Expanded key mappings tailored for Roblox piano games. |
| **Custom** | User-Defined | Flexible, user-configured MIDI-to-keyboard bindings. |

---

## ⚙️ Architecture & Workflow

Both live performance and automated file playback share a unified mapping engine inside **Keydium**, ensuring you only need to configure a profile once.

```mermaid
flowchart TD
    A[🎹 Live MIDI Keyboard] --> C[MIDI Processing]
    B[📄 MIDI File .mid] --> C
    C --> D{Keydium Mapping Engine}
    E[🎮 Game Profile] -.-> D
    F[🛠️ Custom Mapping] -.-> D
    D --> G[⌨️ Keyboard Output]
    G --> H[Target App / Game]

    style D fill:#2b3137,stroke:#58a6ff,stroke-width:2px,color:#fff
    style G fill:#1f6feb,stroke:#58a6ff,color:#fff
