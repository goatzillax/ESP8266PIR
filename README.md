# ESP8266PIR
ESP8266 Based PIR Sensor

Currently being developed on a Wemos D1 Mini + PIR + OLED + SHT3.0 shield.

Eventual target will drop the OLED and SHT3.0 shields; it's just a conglomeration of like 5 different examples for my own amusement.

Targeted Features: 

* WiFi communication
* Deep sleep for long battery operation
* Report results to a Google Spreadsheet

This will require some hardware modifications; the examples I've found online range from incomplete to flat out wrong.

Mostly you have to send a rising edge to the RST pin in order to wake the ESP8266 from deep sleep.

And you know, if another pulse came in while you were in the middle of processing the previous event, it would be neat if it didn't wreck your shit.
