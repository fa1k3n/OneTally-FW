# One Tally
Open source unified tally system 

## Supported switchers 
* Osee GoStream Deck
* Osee GoStream Duet

## supported devices 
with supported devices is meant the FW has been loaded and tested in them . but any esp32s based board should work 
* esp32s nodemcu devboard eg https://www.amazon.se/AZDelivery-NodeMCU-ESP32-modulutvecklingskort-Microcontrol/dp/B09PLC4Z3V?source=ps-sl-shoppingads-lpcontext&ref_=fplfs&psc=1&smid=A1X7QLRQH87QA3
* esp32s wroom devboard eg https://www.amazon.se/AZDelivery-ESP32-NodeMCU-modul-Development-efterföljande/dp/B074RGW2VQ/ref=mp_s_a_1_3?crid=2VNG1WCUJ81AQ&dib=eyJ2IjoiMSJ9.ioCT7K7AYSZMmvDpx9DPuhVU8inh0IAg8V5D1NJxtDVGLsSIPFFfJriPPCF6YENarqquXwjHtqF6Qro0XOgHcos-pZlNQ3A_I5qFpt4zbRbTayAdRQUB66T2ebU9S_zKroeOKN0xeBEzu-LE7eMTzgbW99ughrVx7dgm79kT4pHY8AY-dZoPFVMupBRVXep6O2_p4djBadT5hPEY3obMSQ.aV7nwnCWMrEDucMBjgfVj_aPvXK8QBFnQbtYMDutFr4&dib_tag=se&keywords=esp32+azdelivery+wroom&qid=1747151668&sprefix=azdelivery+wroom%2Caps%2C95&sr=8-3

## Needed parts 
* devboard from supported (or ekvivalent device) 
* 2 LEDs either normal led or WS2812 based RGB leds 
* soldering iron 

## Flashing the device
To flash firmware to your ESP32 board there are two choices.

* Use the online tool found here https://esp.huhn.me/
* Download the official espressif flasher https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html

Regardless of method above there is a bin file attached to each release. Use this bin file and flash to address 0x0, thats it 
