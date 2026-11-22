# JAMMER 2.4GHz — ESP-12E (ESP8266EX) + nRF24L01+

Proyek jammer sinyal **Bluetooth** dan **WiFi** 2.4 GHz yang dibuat dari dua modul:

| Modul | Peran |
|---|---|
| ESP-12E (ESP8266EX, flash 4 MB) | Jammer **WiFi**: probe-flood + active scan di channel 1–13 |
| nRF24L01+ (dengan antena SMA) | Jammer **Bluetooth**: transmit continuous 1 Mbps +0 dBm, sweep 2402–2481 MHz |

> ESP8266 tidak punya Bluetooth, karena itu modul nRF24 dipakai untuk
> menutup pita frekuensi Bluetooth (1 MHz × 79 channel di 2402–2480 MHz).
> nRF24 juga membantu memperlemah WiFi channel 1/6/11.

## Hardware
- Papan development ESP-12E (NodeMcu, flash 4 MB, 160 MHz)
- nRF24L01+ (versi P/P+ dengan konektor antena SMA) + antena
- Kabel jumper (8 kabel)
- USB 5V (daya cukup)

## Wiring

```
  nRF24L01+      ESP-12E (NodeMcu)
  -------        --------------
  VCC     --->   3V3        (WAJIB 3.3V, JANGAN 5V!)
  GND     --->   GND
  CE      --->   D1  (GPIO5)
  CSN     --->   D3  (GPIO0)
  SCK     --->   D7  (GPIO13)
  MOSI    --->   D6  (GPIO12)
  MISO    --->   D5  (GPIO14)
  IRQ     --->   D2  (GPIO4, opsional — sketch memakai polling)
```

Antena nRF24: posisikan tegak lurus (90°) dari badan modul, jangan
dibengkokkan/dipegang.

## Cara flash (PC Windows)

Prasyarat: Arduino CLI + core esp8266 (sudah terpasang di mesin ini:
`arduino-cli` 1.6.0, port **COM9**).

Cara paling mudah: double-klik **`flash_jammer.bat`**
(compile + upload ke COM9 dalam sekali jalan).

Atau manual (perhatikan: di arduino-cli 1.6.0 flag `-b` adalah **FQBN**,
bukan baud rate — baud diarahkan lewat `--board-options`):

```bat
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 E:\02_Projek\jammer\jammer24
arduino-cli upload  -b esp8266:esp8266:nodemcuv2 -p COM9 E:\02_Projek\jammer\jammer24
```

Jika port berubah:
```bat
arduino-cli board list
```

## Cara pakai
1. Sambungkan semua kabel sesuai wiring di atas (VCC 3.3V!).
2. Flash sketch (perintah di atas).
3. Hubungkan USB 5V — jammer langsung berjalan otomatis:
   - WiFi : probe-flood di channel 1–13 (sweep berulang)
   - BT   : transmit continuous, sweep 2402–2481 MHz (sweep berulang)
   Kedua radio bekerja bergantian (time-slicing), jadi seluruh pita
   2.4 GHz tertutup bergiliran — perangkat BT/WiFi di dekatnya akan
   terasa putus-nyambung.
4. Buka Serial Monitor di 115200 baud — status tampil tiap 5 detik.

## Arsitektur firmware
- **Tanpa FreeRTOS.** Core esp8266 3.1.2 memakai NonOS SDK (tidak ada header
  `freertos/...`), jadi semua kerja berjalan di `loop()` secara kooperatif
  (time-slicing): 1 langkah jamming WiFi (1 channel) → `NRF_BURSTS_PER_WIFI`
  burst nRF24 (2 channel) → ulangi.
- **SPI software (bit-bang).** Core ESP8266 hanya mendukung hardware SPI di pin
  tetap (SCK=GPIO14, MISO=GPIO12, MOSI=GPIO13). Agar wiring bebas, driver nRF24
  memakai SPI bit-bang ~1,5 MHz — cukup untuk jamming, dan wiring di atas tetap
  berlaku apa adanya.
- **Self-contained.** Jika nRF24 tidak terdeteksi di SPI, firmware tetap jalan
  (jamming WiFi saja) dan mencetak peringatan — tidak crash.

## Parameter yang bisa diubah (di `jammer24.ino`)
```cpp
const uint32_t NRF_DWELL_MS = 300;  // makin besar = makin kuat per channel
const uint8_t  NRF_CH_MIN = 2;      // batas bawah sweep (MHz = 2400 + channel)
const uint8_t  NRF_CH_MAX = 81;     // batas atas sweep
const char* FAKE_SSID = "JAMMER-24";
```

## Status terverifikasi (12 Nov 2026)
- Firmware **compile bersih** (flash 241 KB / 22%, RAM 28 KB / 36%) dan
  **sudah diflash** ke ESP-12E di COM9 (MAC `e8:68:e7:60:68:f5`, flash 4 MB).
- Boot output terverifikasi via serial:
  ```
  ======================================
   JAMMER 2.4GHz
   ESP-12E (ESP8266EX) + nRF24L01+
  ======================================
  [nRF24] TIDAK TERDETEKSI - hanya WiFi yang dijam
  [WiFi ] probe-flood aktif, channel 1-13
  JAMMER BERJALAN - status ditampilkan tiap 5 detik
  [STATUS] WiFi: probe-flood aktif | nRF24: offline
  ```
- nRF24 belum terpasang/kabel belum disambungkan saat verifikasi, jadi
  firmware berjalan mode **WiFi-only**. Setelah nRF24 disambung sesuai
  tabel wiring, baris `[nRF24] TERDETEKSI - jamming Bluetooth aktif`
  yang muncul.
- Baca ulang output boot kapan saja: `python tools\read_boot.py`
  (reset chip via RTS + membaca COM9 @115200 selama 12 detik).

## Catatan & batasan
- Ini jammer *berdaya rendah* (radio < 100 mW). Efektif untuk jarak
  dekat, ~1–3 meter, dan paling terasa pada perangkat yang sinyalnya
  lemah (Bluetooth kelas 2/3, WiFi 2.4 GHz).
- nRF24 **tidak boleh** diberi 5 V — hanya 3.3 V (LDO/VCC pin papan).
- Jika `[nRF24] TIDAK TERDETEKSI` muncul di Serial: cek ulang kabel
  SPI, pastikan VCC 3.3 V, dan GND bersama.
- Untuk hasil lebih kuat, perbesar `NRF_DWELL_MS` (mis. 500–800 ms).

## Struktur folder
```
E:\02_Projek\jammer\
├── README.md              (file ini)
├── jammer24\
│   └── jammer24.ino       (sketch utama)
├── flash_jammer.bat       (compile + flash sekali-klik)
├── tools\
│   ├── read_boot.py       (reset + baca output boot via COM9)
│   └── setup_core.ps1     (install core esp8266 — sudah dijalankan)
└── dokumentasi\
    └── 123.jpeg           (foto perangkat asli)
```
