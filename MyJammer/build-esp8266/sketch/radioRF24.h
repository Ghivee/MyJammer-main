#line 1 "E:\\02_Projek\\MyJammer-main\\MyJammer\\radioRF24.h"
#ifndef RADIO_RF24_H
#define RADIO_RF24_H

#include "RF24.h"
#include <SPI.h>

// KHUSUS ESP12 (ESP8266).
// ESP8266 punya SATU bus SPI hardware saja:
//   SCK  = GPIO13 (D7)
//   MOSI = GPIO12 (D6)
//   MISO = GPIO14 (D5)
// jadi dipakai objek SPI global - tidak ada pilihan bus lain.
class RadioRF24
{
  private:
    RF24 radio;

  protected:
    bool busOk = false;      // bus SPI berhasil di-inisialisasi
    bool verified = false;   // modul BENAR-benar menjawab (uji baca-tulis)
    bool connected = false;  // modul terdeteksi DAN terkonfigurasi

    int pinLed = -1;

    int currentChannel = 0;

  public:
    RadioRF24();

    // cePin/csnPin : pin CE & CSN (lihat MyJammer.ino)
    // channel      : kanal awal carrier konstan (0..80 = 2400..2480 MHz)
    RadioRF24(int cePin, int csnPin, int channel);

    ~RadioRF24();

    void SetPinStatusLed(int pin);

    // true jika modul terdeteksi (SPI OK + modul menjawab).
    bool IsConnected();

    // Uji modul BENAR-benar hidup: baca SETUP_AW + tulis-baca kanal.
    // Dipakai saat boot (dengan retry) dan pengecekan berkala tiap 10 detik.
    bool Verify();

    // Recovery penuh: power-down, tunggu, konfigurasi ulang,
    // nyalakan lagi carrier konstan di kanal `channel`.
    void Reinit(int channel);

    // Pindah kanal carrier konstan (0..80 = 2400..2480 MHz).
    // Selama CE tetap HIGH dan CONT_WAVE aktif, frekuensi carrier
    // ikut pindah sesuai register kanal (PLL re-lock ~mikrodetik).
    void SetChannel(int channel);
};
#endif
