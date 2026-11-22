# -*- coding: utf-8 -*-
"""Baca output boot JAMMER 2.4GHz dari ESP8266 (COM9, 115200).
Reset chip via urutan DTR/RTS (seperti esptool) lalu membaca serial ~12 detik."""
import sys
import time
import serial

PORT = "COM9"
BAUD = 115200
DURASI = 12

s = serial.Serial(PORT, BAUD, timeout=0.2)
# Seperti esptool di Windows: RTS/DTR di-nonaktifkan = EN high (idle),
# GPIO0 low (boot normal dari flash, bukan mode download).
s.rts = False
s.dtr = False
time.sleep(0.3)
s.reset_input_buffer()

# --- hard reset ala esptool (esp_pylib.hard_reset, jalur UART bridge):
#     setRTS(True)  = EN LOW  (chip di-reset)
#     setRTS(False) = EN HIGH (lepas reset -> boot dari flash)
try:
    s.rts = True
    time.sleep(0.1)
    s.rts = False
except Exception as e:
    print("(reset RTS gagal: %s)" % e)
s.reset_input_buffer()

end = time.time() + DURASI
print("== membaca serial %d detik ==" % DURASI)
while time.time() < end:
    chunk = s.read(512)
    if chunk:
        sys.stdout.buffer.write(chunk)
        sys.stdout.buffer.flush()
s.close()
print("\n== selesai ==")
