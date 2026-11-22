# 🔒 Alat Pengganggu Sinyal Bluetooth dan Wi-Fi dengan ESP32 + NRF24L01+

Proyek ini menggunakan ESP32 dan dua modul NRF24L01+ untuk mengganggu secara selektif kanal Bluetooth, Bluetooth BLE, dan Wi-Fi dengan memanfaatkan tumpang tindih frekuensi 2,4 GHz.

> ⚠️ Proyek ini bersifat edukatif dan eksperimental, dengan kode yang dimodularisasi, tertata baik, dan mudah dipahami, ditujukan untuk pelajar dan pengembang. Banyak contoh di internet yang membingungkan dan/atau sulit dimodifikasi. Di sini, fokusnya adalah membuat dasar yang jelas dan dapat digunakan kembali untuk mempelajari interferensi pada kanal radio 2,4 GHz.


> ⚠️ Perhatian: Pemancaran sinyal pengganggu secara sengaja dapat melanggar hukum di wilayah Anda. Proyek ini tidak boleh digunakan untuk memblokir sinyal secara nyata tanpa izin. Pelajari hukum telekomunikasi setempat sebelum penggunaan praktis apa pun.

---

## 📸 Demonstrasi
Kami akan menggunakan papan standar ESP32 + NRF24L01+
![image](https://github.com/user-attachments/assets/aac3ba4d-ca61-428d-b074-e78014b7ac05)

---

## 🧰 Persyaratan Perangkat Keras

- ✅ 1x ESP32 DevKit
- ✅ 2x Modul **NRF24L01+**
- ✅ 1x Papan standar untuk dua modul **NRF24L01+**
- ✅ Antena (disarankan: 3 dBi atau lebih)
- ✅ Tombol (opsional, untuk berpindah mode; kami akan menggunakan tombol "0" pada ESP agar lebih mudah)
- ✅ Kabel USB untuk pemrograman dan/atau sumber daya

---

## 📦 Pustaka yang Digunakan

- [`RF24`](https://github.com/nRF24/RF24) – kontrol modul NRF24L01
- [`ezButton`](https://github.com/ArduinoGetStarted/ezButton) – pengelolaan tombol
- `SPI` (bawaan ESP32)
- esp_bt dan esp_wifi (bawaan ESP32)

---

## 📁 Struktur Kode

| Folder / Berkas | Deskripsi |
|------------------|-----------|
| `src/`           | Kode utama |
| `radioRF24.*`    | Kelas yang membungkus konfigurasi dan kontrol NRF24 |
| `modeRF24.*`     | Kelas mode operasi (Bluetooth, BLE, Wi-Fi, semua) |
| `MyJammer.ino`   | Kode utama Arduino (setup/loop) |
| `README.md`      | Berkas ini |

---

## ⚙️ Mode Operasi

Tombol yang terhubung (atau secara bawaan tombol "0" / BOOT) ke ESP32 berganti-ganti antar mode:

| Mode | Deskripsi |
|------|-----------|
| `MODE_BLUETOOTH`       | Kanal klasik Bluetooth |
| `MODE_BLUETOOTH_BLE`   | Kanal advertising BLE (2, 26, 80) |
| `MODE_WIFI`            | Seluruh pita Wi-Fi 2,4 GHz |
| `MODE_WIFI_COMMOM`     | Hanya kanal Wi-Fi yang paling umum |
| `MODE_ALL_CHANNELS`    | Pemindaian penuh dari 0 hingga 80 |
| `MODE_DISABLE`         | Menonaktifkan interferensi |

---

## 🧪 Cara Kerja

- ESP32 mengonfigurasi dua modul NRF24L01+ untuk **mengirim sinyal kontinu (`startConstCarrier`)** pada kanal tertentu.
- Kanal dipilih secara berurutan atau acak.
- Emisi kontinu mengganggu komunikasi Bluetooth/Wi-Fi pada kanal tersebut.

---

## 🚀 Unggah dan Jalankan

1. Pasang pustaka yang disebutkan.
2. Hubungkan modul sesuai pemetaan pin.
3. Kompilasi dan unggah kode ke ESP32.
4. Pantau melalui `Serial Monitor` (115200 baud).
5. Tekan tombol untuk mengganti mode pemblokiran.

---

## 🔒 Peringatan Hukum

Proyek ini **semata-mata untuk tujuan edukasi**. Memancarkan sinyal interferensi yang disengaja pada frekuensi berlisensi **dapat melanggar hukum** di wilayah Anda. Periksa hukum setempat sebelum penggunaan praktis apa pun.

---

## 📃 Lisensi

Lisensi MIT – bebas untuk mempelajari, memodifikasi, dan membagikan. Gunakan dengan penuh tanggung jawab.

---

## ✉️ Kontak

Dikembangkan oleh **Socramsss**  
📧 [socramsss@ucl.com]  
💬 Kontribusi dan saran sangat diterima!
# 🔒 Alat Pengganggu Sinyal Bluetooth dan Wi-Fi dengan ESP32 + NRF24L01+

Proyek ini menggunakan ESP32 dan dua modul NRF24L01+ untuk mengganggu secara selektif kanal Bluetooth, Bluetooth BLE, dan Wi-Fi dengan memanfaatkan tumpang tindih frekuensi 2,4 GHz.

> ⚠️ Proyek ini hanya untuk tujuan **edukatif** dan **eksperimental**. Penggunaan yang tidak tepat dapat melanggar hukum telekomunikasi setempat.

---

## 📸 Demonstrasi
*(Tambahkan foto atau GIF proyek yang sedang berjalan di sini, jika ada)*
