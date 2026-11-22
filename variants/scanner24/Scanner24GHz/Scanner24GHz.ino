/*
  ============================================================
  Scanner Spektrum 2,4 GHz — RECEIVE-ONLY (PASSIVE)
  ESP32 + 2x NRF24L01+
  ------------------------------------------------------------
  Sketsa ini HANYA mendeteksi carrier (RX).
  Tidak ada pemancaran: tidak ada write(), startWrite(),
  startConstCarrier(), atau pengiriman payload apa pun.
  ============================================================
*/

#include <SPI.h>
#include <RF24.h>

// ---------- Pin SPI ESP32 (VSPI) ----------
const uint8_t PIN_SCK  = 18;
const uint8_t PIN_MISO = 19;
const uint8_t PIN_MOSI = 23;

// ---------- Modul NRF24 #1 ----------
const uint8_t CE1  = 4;
const uint8_t CSN1 = 5;

// ---------- Modul NRF24 #2 ----------
const uint8_t CE2  = 16;
const uint8_t CSN2 = 17;

RF24 radio1(CE1, CSN1);
RF24 radio2(CE2, CSN2);

// ---------- Parameter pemindaian ----------
const uint8_t  CH_MIN    = 0;    // 2400 MHz
const uint8_t  CH_MAX    = 125;  // 2525 MHz
const uint8_t  SAMPLES   = 40;   // sampel per kanal
const uint16_t SETTLE_US = 300;  // waktu RX sebelum baca RPD (min. 258 us)
const uint16_t GAP_US    = 50;   // jeda antar sampel

uint8_t occupancy[126];          // 0..100 (%)

const char* levelDari(uint8_t pct) {
  if (pct >= 70) return "SANGAT_RAMAI";
  if (pct >= 40) return "RAMAI";
  if (pct >= 10) return "SEDANG";
  return "SEPI";
}

int wifiDariNrf(int ch) {
  for (int w = 1; w <= 13; w++) {
    int center = 12 + (w - 1) * 5;
    if (ch == center) return w;
  }
  return -1;
}

int bleDariNrf(int ch) {
  if (ch == 2)  return 37;
  if (ch == 26) return 38;
  if (ch == 80) return 39;
  return -1;
}

void siapkanRadio(RF24 &r) {
  r.setDataRate(RF24_1MBPS);
  r.stopListening();
  r.flush_rx();
}

void pindaiRentang(RF24 &r, uint8_t dari, uint8_t sampai) {
  for (uint8_t ch = dari; ch <= sampai; ch++) {
    r.setChannel(ch);
    uint16_t kena = 0;

    for (uint8_t i = 0; i < SAMPLES; i++) {
      r.startListening();
      delayMicroseconds(SETTLE_US);
      if (r.testRPD()) kena++;
      r.stopListening();
      delayMicroseconds(GAP_US);
    }

    occupancy[ch] = (uint8_t)((kena * 100UL) / SAMPLES);
  }
}

void cetakHeaderCsv() {
  Serial.println("waktu_ms,nrf_ch,freq_mhz,wifi_ch,ble_ch,occupancy_pct,level");
}

void cetakCsv(unsigned long durasi_ms) {
  for (uint8_t ch = CH_MIN; ch <= CH_MAX; ch++) {
    Serial.print(millis());            Serial.print(',');
    Serial.print(ch);                  Serial.print(',');
    Serial.print(2400 + ch);           Serial.print(',');
    int w = wifiDariNrf(ch);
    if (w > 0) Serial.print(w);
    Serial.print(',');
    int b = bleDariNrf(ch);
    if (b > 0) Serial.print(b);
    Serial.print(',');
    Serial.print(occupancy[ch]);       Serial.print(',');
    Serial.println(levelDari(occupancy[ch]));
  }
  Serial.print("# durasi_sweep_ms=");
  Serial.println(durasi_ms);
}

void setup() {
  Serial.begin(115200);
  delay(600);

  Serial.println();
  Serial.println("==============================================");
  Serial.println(" Scanner 2,4 GHz — RECEIVE-ONLY (PASSIVE)");
  Serial.println(" ESP32 + 2x NRF24L01+");
  Serial.println(" Tidak memancarkan sinyal apa pun.");
  Serial.println("==============================================");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, -1);

  bool ok1 = radio1.begin(&SPI);
  bool ok2 = radio2.begin(&SPI);

  Serial.printf("Modul #1 (CE=%u, CSN=%u): %s\n", CE1, CSN1, ok1 ? "OK" : "GAGAL");
  Serial.printf("Modul #2 (CE=%u, CSN=%u): %s\n", CE2, CSN2, ok2 ? "OK" : "GAGAL");

  if (!ok1 || !ok2) {
    Serial.println("PERIKSA: VCC 3.3V, GND, SCK/MISO/MOSI, CE/CSN.");
    Serial.println("Pemindaian dihentikan.");
    while (true) delay(1000);
  }

  siapkanRadio(radio1);
  siapkanRadio(radio2);

  Serial.println("Mulai memindai 0..125 (2400..2525 MHz)...");
  Serial.println("Biarkan antena terpasang. Jangan menambahkan fungsi TX.");
  Serial.println();
  cetakHeaderCsv();
}

void loop() {
  unsigned long t0 = millis();

  // Keduanya tetap dalam mode RX; rentang dibagi agar lebih cepat.
  pindaiRentang(radio1, 0, 62);
  pindaiRentang(radio2, 63, 125);

  unsigned long durasi = millis() - t0;
  cetakCsv(durasi);
  delay(1500);
}
