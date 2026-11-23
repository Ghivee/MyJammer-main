# MyJammer

Firmware for an autonomous 2.4 GHz RF jammer, built for an ESP8266 (ESP-12) with an nRF24L01+ radio module. It sweeps all 81 channels (0 to 80, 2400 to 2480 MHz) so it covers both Bluetooth's AFH hopping (79 channels) and BLE's advertising channels. It starts transmitting as soon as it is powered and has no off switch other than cutting power.

[Bahasa Indonesia](README.id.md)

## Legal notice

Operating a radio jammer is illegal in most countries, including Indonesia, because it disrupts a shared public spectrum rather than a single targeted device: it also affects 2.4 GHz WiFi and other people's Bluetooth devices in range, not just a chosen target. This firmware was built for personal hardware experimentation. Check your local telecommunications law before powering the device on, and only test it in a shielded enclosure or otherwise isolated from other people's equipment.

## Contents

| Path | What it holds |
| --- | --- |
| `MyJammer/MyJammer.ino` | Main sketch: a two-stage pattern, a fast sweep over all channels and a longer dwell on channels used by Bluetooth and WiFi |
| `MyJammer/radioRF24.cpp`, `.h` | Low-level nRF24L01+ register access |
| `MyJammer/build-esp8266/` | Arduino CLI build output |
| `variants/jammer24/`, `variants/scanner24/` | Other sketch variants: a plain jammer and a channel scanner |

## Building

Arduino CLI with the ESP8266 board core, and the RF24 library for the nRF24L01+ module. Flash `MyJammer/MyJammer.ino` to an ESP-12/ESP8266 wired to the nRF24L01+ over SPI.
