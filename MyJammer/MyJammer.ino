#include "radioRF24.h"
#include "RF24.h"
#include <SPI.h>
#include <ESP8266WiFi.h>

// =====================================================================
//  MyJammer v2 — jammer 2.4 GHz otonom (KHUSUS ESP12 / ESP8266)
//  -------------------------------------------------------------------
//  - Begitu power menyala, jammer LANGSUNG jalan. Tanpa tombol.
//    Untuk mati: cabut power.
//
//  - MENGAPA menyapu SEMUA kanal 0-80 (2400-2480 MHz)?
//    Bluetooth Classic (AFH) lompat-lompat di 79 kanal (2402-2480 MHz)
//    ratusan kali per detik, dan BLE pakai 40 kanal (3 kanal iklan di
//    2437/2438/2439 MHz). Karena sinyalnya berpindah-pindah, jamming
//    satu kanal saja tidak mempan - jammer harus menutupi SELURUH
//    rentang 2.4 GHz secara terus-menerus.
//    Konsekuensinya: WiFi 2.4 GHz (rentang yang sama) ikut terganggu.
//
//  - POLA 2 TAHAP (diulang terus):
//      1) SWEEP : seluruh 81 kanal (0-80), 1 ms/kanal  = 81 ms
//                 -> menutupi AFH 79 kanal secara merata
//      2) SOAK  : kanal iklan BLE 37/38/39 (2437/2438/2439 MHz)
//                 masing-masing dijam 25 ms NONSTOP = 75 ms
//                 -> di situlah PAIRING Bluetooth terjadi. PDU BLE
//                    hanya 300 us, jadi kalau kebagun di soak 25 ms,
//                    sinyalnya "teriak total" (bukan cuma tergores).
//      1 putaran penuh = 156 ms.
//
//  - PERBAIKAN v2 (alasan versi lama "ga ngaruh"):
//      * SPI 10 MHz (sesuai datasheet nRF24L01+; versi lama 16 MHz
//        di atas spek -> state machine SPI ESP12 bisa macet -> chip
//        direset watchdog tiap ~5 detik -> output acak/gabutan)
//      * jeda 150 ms setelah power (modul butuh waktu VCC/PLL stabil)
//      * verifikasi modul baca-tulis + retry (library lama mencetak
//        "terhubung" padahal modul mati)
//      * PWR_UP di-set eksplisit (tanpa ini carrier tidak terpancar)
//      * pengecekan modul tiap 10 detik + recovery otomatis
//      * LED heartbeat (bukti hidup walau serial monitor rusak)
//
//  - Kabelan sesuai foto (nRF24L01+ -> ESP12E / NodeMCU):
//      VCC  -> 3V3   (WAJIB 3.3V, jangan 5V!)
//      GND  -> GND
//      CE   -> D1    (GPIO5)
//      CSN  -> D3    (GPIO0)
//      SCK  -> D7    (GPIO13)
//      MOSI -> D6    (GPIO12)
//      MISO -> D5    (GPIO14)
//      IRQ  -> D2    (GPIO4, tidak dipakai)
//
//  - REKOMENDASI HARDWARE (penentu kekuatan jamming):
//      * Kapasitor 100-470uF di VCC-GND nRF24 (sedekat mungkin)
//      * Antena 2.4 GHz yang baik (>=3 dBi), kencang di port SMA
//      * Resistor 10k dari D3 (GPIO0) ke 3V3 -> ESP12 pasti boot
//        saat power dicolok (tanpa ini sering "stuck" di download)
//      * Power USB sehat (>=500mA), kabel data, bukan kabel cas-only
// =====================================================================

#define PIN_CE     5   // D1 (GPIO5)
#define PIN_CSN    0   // D3 (GPIO0)
#define PIN_LED    2   // LED onboard (GPIO2 / D4)

// JAM_SWEEP_MS : berapa lama tiap kanal "diteriaki" saat tahap SWEEP
#define JAM_SWEEP_MS   1

// JAM_SOAK_MS : berapa lama kanal iklan BLE (37/38/39) "diteriaki"
// nonstop saat tahap SOAK (ini yang paling memukul proses pairing)
#define JAM_SOAK_MS    25

// Batas sapuan: seluruh rentang 2.4 GHz dalam kanal nRF24L01+
#define JAM_CH_FIRST  0   // 2400 MHz
#define JAM_CH_LAST   80  // 2480 MHz

RadioRF24* radio;

// Pola sapuan: 81 slot SWEEP + 3 slot SOAK = 84 slot
struct JamSlot
{
  uint8_t ch;     // kanal (0..80 = 2400..2480 MHz)
  uint8_t dwell;  // berapa ms "teriak" di kanal ini
};
JamSlot jamSlots[84];
int jamCount = 0;

int lastChannel = 0;
bool ledState = false;

void buildJamPattern()
{
  // TAHAP 1 - SWEEP : seluruh rentang 2.4 GHz secara merata
  for (int ch = JAM_CH_FIRST; ch <= JAM_CH_LAST; ch++)
  {
    jamSlots[jamCount].ch = ch;
    jamSlots[jamCount].dwell = JAM_SWEEP_MS;
    jamCount++;
  }
  // TAHAP 2 - SOAK : kanal iklan BLE (tempat pairing) dijam lama
  jamSlots[jamCount].ch = 37; jamSlots[jamCount].dwell = JAM_SOAK_MS; jamCount++;
  jamSlots[jamCount].ch = 38; jamSlots[jamCount].dwell = JAM_SOAK_MS; jamCount++;
  jamSlots[jamCount].ch = 39; jamSlots[jamCount].dwell = JAM_SOAK_MS; jamCount++;
}

void setup()
{
  // Penanda boot di 74880 baud (kecepatan DEFAULT ESP8266) supaya
  // tetap terbaca walau baud rate Serial Monitor salah.
  Serial.begin(74880);
  delay(20);
  Serial.print("\r\n[MyJammer v2 boot]\r\n");

  Serial.begin(115200);
  delay(100);

  // Matikan Wi-Fi supaya tidak berebut RF 2.4 GHz
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);

  // ESP12 (ESP8266): 1 modul nRF24L01+ di bus SPI hardware
  // (SCK=GPIO13, MOSI=GPIO12, MISO=GPIO14 sesuai foto)
  radio = new RadioRF24(PIN_CE, PIN_CSN, JAM_CH_FIRST);
  radio->SetPinStatusLed(PIN_LED);
  ledState = false;
  digitalWrite(PIN_LED, HIGH);  // mulai dari mati (di NodeMCU LOW = nyala)

  buildJamPattern();

  Serial.println();
  Serial.println("=== MyJammer v2 (otonom, sapu seluruh 2.4 GHz) ===");
  if (radio->IsConnected())
  {
    Serial.println("Modul nRF24: MENJAWAB - jammer SUDAH AKTIF.");
  }
  else
  {
    Serial.println("Modul nRF24: TIDAK MENJAWAB - cek VCC 3V3 & kabelan!");
    Serial.println("Recovery otomatis tiap 10 detik.");
  }
  Serial.println("PA maksimum, SPI 10 MHz (sesuai datasheet modul).");
  Serial.println("Pola: sweep kanal 0-80 (1 ms/kanal) + soak kanal");
  Serial.println("iklan BLE 37/38/39 (25 ms/kanal = tempat pairing).");
  Serial.println("Bluetooth (AFH 79 kanal), BLE, dan WiFi 2.4 GHz ikut terganggu.");
  Serial.println("LED onboard: BERKEDIP CEPAT = modul OK; BERKEDIP LAMBAT =");
  Serial.println("modul belum menjawab; MATI = program tidak jalan.");
  Serial.println("Tidak perlu tombol. Untuk mati: cabut power.");
  Serial.println();
}

void loop()
{
  static uint32_t lastInfo = 0;
  static uint32_t lastLed = 0;
  static uint32_t lastCheck = 0;
  static int slot = 0;

  // "Teriak" (carrier konstan) di kanal ini selama dwell-nya,
  // lanjut ke slot berikutnya, terus berputar:
  //   sweep 0-80 (81 ms) -> soak 37/38/39 (75 ms) -> ulang.
  radio->SetChannel(jamSlots[slot].ch);
  lastChannel = jamSlots[slot].ch;
  delay(jamSlots[slot].dwell);
  slot++;
  if (slot >= jamCount)
    slot = 0;

  uint32_t now = millis();

  // LED heartbeat: SELALU berkedip = bukti program masih jalan
  // (berguna saat serial monitor tidak terbaca).
  // Cepat (500 ms) = modul OK; lambat (1500 ms) = modul belum menjawab.
  uint32_t ledPeriod = radio->IsConnected() ? 500 : 1500;
  if (now - lastLed >= ledPeriod)
  {
    lastLed = now;
    ledState = !ledState;
    digitalWrite(PIN_LED, ledState ? LOW : HIGH);
  }

  // Info tiap 5 detik: bukti jammer masih bekerja
  if (now - lastInfo >= 5000)
  {
    lastInfo = now;
    Serial.print("Jamming kanal ");
    Serial.print(lastChannel);
    Serial.print(" (");
    Serial.print(2400 + lastChannel);
    Serial.print(" MHz) ");
    Serial.println(radio->IsConnected() ? "[modul OK]" : "[MODUL TIDAK MENJAWAB]");
  }

  // Pengecekan modul tiap 10 detik + recovery otomatis
  if (now - lastCheck >= 10000)
  {
    lastCheck = now;
    if (!radio->Verify())
    {
      Serial.println("[!] Modul berhenti menjawab - mencoba recovery...");
      radio->Reinit(lastChannel);
    }
  }
}
