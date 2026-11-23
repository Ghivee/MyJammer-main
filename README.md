# MyJammer

MyJammer is firmware for an autonomous 2.4 GHz RF jammer, built from an ESP8266 (ESP-12) paired with an nRF24L01+ radio module. It was made as a personal hardware experiment in how 2.4 GHz spectrum jamming works at the firmware level: how Bluetooth's frequency-hopping and BLE's advertising channels can both be covered by one sweep pattern, and how far a $5 radio module can get toward disrupting them. It starts transmitting the moment it is powered on and has no software off switch; the only way to stop it is to cut power.

[Baca dalam Bahasa Indonesia di bawah](#bahasa-indonesia)

## Legal notice

Operating a radio jammer is illegal in most countries, including Indonesia under its telecommunications law, because it disrupts a shared public spectrum rather than a single targeted device: running this firmware also disrupts other people's 2.4 GHz WiFi and Bluetooth devices within range, not just a device you intended to target. This firmware was built for personal hardware experimentation. Check your local telecommunications regulations before powering the device on, and only test it inside a shielded enclosure (a Faraday cage or similar) or otherwise fully isolated from other people's equipment.

## How the sweep works

Bluetooth Classic's adaptive frequency hopping (AFH) jumps across 79 channels between 2402 and 2480 MHz hundreds of times per second, and BLE separately uses 40 channels with 3 fixed advertising channels at 2437, 2438 and 2439 MHz. Because the target signal keeps moving, jamming a single fixed channel does nothing useful: the firmware instead sweeps all 81 channels from 0 to 80 (2400 to 2480 MHz) continuously, spending about 1 millisecond per channel in a fast sweep pass (roughly 81 ms per full pass), which inevitably covers the same 2.4 GHz band WiFi uses as a side effect.

## Contents

| Path | What it holds |
| --- | --- |
| `MyJammer/MyJammer.ino` | The main sketch: the two-stage sweep pattern described above, running continuously with no button and no off switch |
| `MyJammer/radioRF24.cpp`, `.h` | Low-level, direct register access to the nRF24L01+ module (written by hand rather than using the standard RF24 library) |
| `MyJammer/build-esp8266/` | Build artifacts from the Arduino CLI, kept for reference |
| `variants/jammer24/` | A simpler, single-mode jammer variant, documented in its own `README-jammer24.md` |
| `variants/scanner24/` | A channel scanner variant used to observe activity across the band instead of jamming it |

## Building it

Arduino CLI with the ESP8266 board core installed, plus the RF24 library for the nRF24L01+ module. Flash `MyJammer/MyJammer.ino` to an ESP-12/ESP8266 board wired to the nRF24L01+ module over SPI.

---

## Bahasa Indonesia

MyJammer adalah firmware untuk jammer RF 2.4 GHz otonom, dibuat dari ESP8266 (ESP-12) yang dipasangkan dengan modul radio nRF24L01+. Ini dibuat sebagai eksperimen perangkat keras pribadi tentang cara kerja jamming spektrum 2.4 GHz di level firmware: bagaimana lompatan frekuensi Bluetooth dan kanal iklan BLE bisa sama-sama ditutupi satu pola sapuan, dan sejauh mana modul radio seharga lima dolar bisa mengganggu keduanya. Firmware ini mulai mengirim begitu dinyalakan dan tidak punya tombol mati lewat software; satu-satunya cara menghentikannya adalah mencabut power.

### Peringatan hukum

Mengoperasikan jammer radio melanggar hukum di hampir semua negara, termasuk Indonesia lewat undang-undang telekomunikasinya, karena ini mengganggu spektrum publik bersama, bukan hanya satu perangkat yang ditarget: menjalankan firmware ini juga mengganggu perangkat WiFi dan Bluetooth 2.4 GHz milik orang lain di sekitarnya, bukan hanya perangkat yang Anda maksudkan untuk ditarget. Firmware ini dibuat untuk eksperimen perangkat keras pribadi. Periksa aturan telekomunikasi setempat sebelum menyalakan alat ini, dan uji hanya di dalam ruang tertutup (Faraday cage atau sejenisnya) atau yang benar-benar terisolasi dari perangkat orang lain.

### Cara sapuan bekerja

Frequency hopping adaptif (AFH) Bluetooth Classic melompat di antara 79 kanal antara 2402 dan 2480 MHz ratusan kali per detik, sedangkan BLE secara terpisah memakai 40 kanal dengan 3 kanal iklan tetap di 2437, 2438, dan 2439 MHz. Karena sinyal targetnya terus berpindah, mengganggu satu kanal tetap saja tidak akan berguna: firmware ini sebagai gantinya menyapu seluruh 81 kanal dari 0 sampai 80 (2400 sampai 2480 MHz) secara terus-menerus, menghabiskan sekitar 1 milidetik per kanal dalam satu putaran sapuan cepat (sekitar 81 ms per putaran penuh), yang sebagai efek samping pasti ikut menutupi band 2.4 GHz yang sama dipakai WiFi.

### Isi

| Path | Isi |
| --- | --- |
| `MyJammer/MyJammer.ino` | Sketch utama: pola sapuan dua tahap di atas, berjalan terus-menerus tanpa tombol dan tanpa tombol mati |
| `MyJammer/radioRF24.cpp`, `.h` | Akses register tingkat rendah langsung ke modul nRF24L01+ (ditulis tangan, bukan memakai library RF24 standar) |
| `MyJammer/build-esp8266/` | Artefak build dari Arduino CLI, disimpan sebagai referensi |
| `variants/jammer24/` | Varian jammer yang lebih sederhana, satu mode saja, didokumentasikan di `README-jammer24.md` miliknya sendiri |
| `variants/scanner24/` | Varian pemindai kanal, dipakai untuk mengamati aktivitas di sepanjang band, bukan mengganggunya |

### Build

Arduino CLI dengan board core ESP8266 terpasang, plus library RF24 untuk modul nRF24L01+. Flash `MyJammer/MyJammer.ino` ke board ESP-12/ESP8266 yang tersambung ke modul nRF24L01+ lewat SPI.
