[![Build Status](https://travis-ci.org/kylegordon/homie-h801.svg?branch=master)](https://travis-ci.org/kylegordon/homie-h801)

# homie-h801

A work in progress for the H801 WiFi LED controller

Uses the [Homie](https://github.com/marvinroger/homie-esp8266/releases) framework, so you don't need to worry about wireless connectivity, wireless configuration persistence, and all that. Simply compile and upload, and configure using the Homie configuration tool.
All future flashes will not overwrite the wireless configuration.

Best used with PlatformIO. Simply git clone, pio run -t upload, watch the dependencies download and compile, and then if required do the initial Homie configuration with the tool for Homie 2 at http://setup.homie-esp8266.marvinroger.fr/

Can be OTA updated using MQTT, or homie-ota
```./ota_updater.py -l 172.24.32.13 -t devices/ -i a020a61aafea /home/kyleg/Projects/homie-h801/.pioenvs/esp01_1m/firmware.bin ```
