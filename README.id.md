# MyJammer

Firmware jammer RF 2.4 GHz otonom, untuk ESP8266 (ESP-12) dengan modul radio nRF24L01+. Menyapu seluruh 81 kanal (0 sampai 80, 2400 sampai 2480 MHz) sehingga menutupi lompatan AFH Bluetooth (79 kanal) maupun kanal iklan BLE. Mulai mengirim begitu dinyalakan, dan tidak ada tombol mati selain mencabut power.

[English](README.md)

## Peringatan hukum

Mengoperasikan jammer RF melanggar hukum di hampir semua negara, termasuk Indonesia, karena ini mengganggu spektrum publik bersama, bukan hanya satu perangkat yang ditarget: WiFi 2.4 GHz dan perangkat Bluetooth orang lain di sekitarnya ikut terdampak. Firmware ini dibuat untuk eksperimen perangkat keras pribadi. Periksa aturan telekomunikasi setempat sebelum menyalakan alat ini, dan uji hanya di ruang tertutup (shielded) atau yang terisolasi dari perangkat orang lain.

## Isi

| Path | Isi |
| --- | --- |
| `MyJammer/MyJammer.ino` | Sketch utama: pola dua tahap, sapuan cepat seluruh kanal dan tahan lebih lama di kanal yang dipakai Bluetooth dan WiFi |
| `MyJammer/radioRF24.cpp`, `.h` | Akses register tingkat rendah modul nRF24L01+ |
| `MyJammer/build-esp8266/` | Hasil build Arduino CLI |
| `variants/jammer24/`, `variants/scanner24/` | Varian sketch lain: jammer biasa dan pemindai kanal |

## Build

Arduino CLI dengan board core ESP8266, dan library RF24 untuk modul nRF24L01+. Flash `MyJammer/MyJammer.ino` ke ESP-12/ESP8266 yang terhubung ke nRF24L01+ lewat SPI.
