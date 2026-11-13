# 🎹 MIDI to Key & Auto-Player

Aplikasi/skrip utilitas untuk menghubungkan perangkat MIDI fisik (seperti digital piano/keyboard) ke input keyboard komputer, serta memutar file `.mid` secara otomatis untuk instrumen virtual di dalam game seperti **Genshin Impact** dan **Roblox**.

---

## ✨ Fitur Utama

### 1. Live MIDI-to-Keyboard Input
Mengubah sinyal tuts piano fisik menjadi tekanan tombol keyboard PC secara *real-time*.
* **Preset Khusus Game:** Dilengkapi profil bawaan yang sudah disesuaikan dengan tata letak instrumen di **Genshin Impact** (21-key layout) dan **Roblox** (layout piano virtual standar).
* **Custom Key Mapping:** Bebas mengatur dan memetakan tuts nada tertentu pada piano ke tombol keyboard apa pun sesuai kebutuhan.

### 2. MIDI Auto-Player
Memainkan file lagu berformat `.mid` secara otomatis di dalam game tanpa perlu menekan tombol secara manual.
* **Plug & Play:** Cukup pilih atau masukkan file `.mid` yang diinginkan, dan program akan mensimulasikan penekanan tombol sesuai tempo dan nada pada file tersebut.
* **Kompatibel Multi-Game:** Dapat digunakan dengan mengikuti profil mapping yang sudah diatur (Genshin Impact, Roblox, maupun preset *custom*).

---

## 🛠️ Cara Penggunaan

1. **Persiapan Perangkat:** Pastikan digital piano/MIDI controller sudah terhubung ke PC (untuk mode Live Input), atau siapkan file `.mid` (untuk mode Auto-Play).
2. **Pilih Mode:**
   * Jalankan mode **Live Input** untuk bermain langsung menggunakan piano.
   * Jalankan mode **Auto-Play** dan pilih file `.mid` untuk memutar lagu secara otomatis.
3. **Atur Mapping (Opsional):** Buka file konfigurasi (`config.json` / menu pengaturan) jika ingin mengubah pemetaan tuts nada ke tombol keyboard tertentu.
4. **Mulai Bermain:** Buka game tujuan (Genshin Impact / Roblox), masuk ke mode instrumen, dan mulai mainkan.

---

## ⚙️ Contoh Konfigurasi Mapping

Kamu bisa menyesuaikan mapping nada ke tombol keyboard secara mandiri. Berikut contoh format pengaturannya:

```json
{
  "preset": "genshin_impact",
  "mapping": {
    "C4": "z",
    "D4": "x",
    "E4": "c",
    "F4": "v",
    "G4": "b",
    "A4": "n",
    "B4": "m"
  }
}