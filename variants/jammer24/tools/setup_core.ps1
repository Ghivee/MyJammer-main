$ErrorActionPreference = 'Stop'
$cli = 'C:\Program Files\Arduino CLI\arduino-cli.exe'
$idx = 'https://arduino.esp8266.com/stable/package_esp8266com_index.json'
& $cli core update-index --additional-urls $idx
& $cli core install esp8266:esp8266 --additional-urls $idx
& $cli core list
