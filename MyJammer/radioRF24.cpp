#include "radioRF24.h"

RadioRF24::RadioRF24()
{
}

// SPI 10 MHz = batas maksimum RESMI nRF24L01+ (datasheet).
// Versi lama memakai 16 MHz (di ATAS spek modul). Di hardware yang
// kurang baik (modul clone, kabel panjang, power goyah) itu bisa
// membuat state machine SPI ESP12 macet - CPU berputar di busy-wait
// tanpa timeout sampai watchdog (5 detik) meriset chip: output jadi
// acak/gabutan dan program seperti "stuck". 10 MHz aman & cukup cepat.
RadioRF24::RadioRF24(int cePin, int csnPin, int channel)
  : radio(cePin, csnPin, 10000000)
{
  // ESP12 (ESP8266): bus SPI hardware-nya adalah objek global `SPI`.
  // WAJIB dipanggil sebelum radio.begin() karena RF24::begin()
  // tidak memanggil spi->begin() sendiri.
  SPI.begin();

  // Modul baru dinyalakan: VCC & PLL butuh waktu stabil
  // (datasheet: hingga ~100 ms). Tanpa jeda ini pembacaan register
  // pertama bisa 0x00/0xFF -> modul dianggap mati padahal hidupnya.
  delay(150);

  busOk = radio.begin(&SPI);
  if (!busOk)
  {
    Serial.println("SPI tidak terhubung !!!");
    return;
  }

  // begin() saja BUKAN bukti modul hidup - library tidak pernah
  // mengecek modulnya. Coba verifikasi beberapa kali karena modul
  // kadang baru "siap" beberapa ratus ms setelah power.
  for (int i = 0; i < 6; i++)
  {
    if (Verify())
      break;
    if (i < 5)
      delay(100);
  }

  if (verified)
  {
    radio.setAutoAck(false);
    radio.stopListening();
    radio.setRetries(0, 0);
    radio.setPALevel(RF24_PA_MAX, true);
    radio.setDataRate(RF24_2MBPS);
    radio.setCRCLength(RF24_CRC_DISABLED);
    // PASTIKAN bit PWR_UP menyala: library melewatkan powerUp()
    // kalau pembacaan register CONFIG pertama tidak sempurna.
    // Tanpa PWR_UP, carrier TIDAK terpancar sama sekali.
    radio.powerUp();
    radio.startConstCarrier(RF24_PA_MAX, channel);
    currentChannel = channel;
    connected = true;
    Serial.println("SPI terhubung - modul nRF24 MENJAWAB !!!");
    radio.printPrettyDetails();
  }
  else
  {
    connected = false;
    Serial.println("SPI terpasang, TAPI modul nRF24 TIDAK MENJAWAB!");
    Serial.println("  -> cek VCC (harus 3V3, BUKAN 5V) & semua kabel");
    Serial.println("  -> pasang kapasitor 100-470uF di VCC-GND modul");
    // Coba teriak juga (tidak ada ruginya), recovery tiap 10 detik.
    radio.startConstCarrier(RF24_PA_MAX, channel);
    currentChannel = channel;
  }
}

RadioRF24::~RadioRF24()
{
}

void RadioRF24::SetPinStatusLed(int pin)
{
  if (pin >= 0)
  {
    pinMode(pin, OUTPUT);
    pinLed = pin;
  }
}

bool RadioRF24::IsConnected()
{
  return connected && verified;
}

// Uji apakah modul BENAR-benar hidup & menjawab:
//  1) baca register SETUP_AW (isChipConnected) - chip hidup = 3
//  2) tulis kanal 63  -> baca harus 63
//  3) tulis kanal 23  -> baca harus 23
// Chip mati / MISO putus / kabel salah -> salah satu pasti gagal
// (chip mati umumnya membaca 0x00 atau 0xFF).
bool RadioRF24::Verify()
{
  if (!busOk)
  {
    verified = false;
    return false;
  }

  if (!radio.isChipConnected())
  {
    verified = false;
    return false;
  }

  radio.setChannel(63);
  if (radio.getChannel() != 63)
  {
    verified = false;
    return false;
  }

  radio.setChannel(23);
  if (radio.getChannel() != 23)
  {
    verified = false;
    return false;
  }

  verified = true;
  return true;
}

// Recovery penuh: matikan modul, beri waktu pulih, lalu konfigurasi
// ulang dari awal dan nyalakan carrier konstan di kanal `channel`.
void RadioRF24::Reinit(int channel)
{
  if (!busOk)
  {
    SPI.begin();
    delay(150);
    busOk = radio.begin(&SPI);
  }

  radio.stopListening();
  radio.powerDown();
  delay(100);

  for (int i = 0; i < 6; i++)
  {
    if (Verify())
    {
      radio.setAutoAck(false);
      radio.stopListening();
      radio.setRetries(0, 0);
      radio.setPALevel(RF24_PA_MAX, true);
      radio.setDataRate(RF24_2MBPS);
      radio.setCRCLength(RF24_CRC_DISABLED);
      radio.powerUp();
      radio.startConstCarrier(RF24_PA_MAX, channel);
      currentChannel = channel;
      connected = true;
      Serial.println("[+] modul nRF24 PULIH - jamming lanjut !!!");
      return;
    }
    if (i < 5)
      delay(100);
  }

  connected = false;
  verified = false;
  Serial.println("[-] modul tetap tidak menjawab - cek VCC 3V3 & kabelan!");
}

// Pindah kanal carrier konstan (0..80 = 2400..2480 MHz).
// Ringan: hanya 1 register SPI (~2,4 us di 10 MHz), aman dipanggil
// tiap 1 ms.
void RadioRF24::SetChannel(int channel)
{
  if (channel >= 0 && channel <= 80)
  {
    currentChannel = channel;
    radio.setChannel(currentChannel);
  }
}
