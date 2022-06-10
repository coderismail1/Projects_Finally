
// MILLI DEVELOPER - OTOMATİCK + MANUEL SMART HOME
/**********************************************************************************
 *  TITLE: Google + Alexa + Manual Switch/Button control 4 Relays using NodeMCU & Sinric Pro (Real time feedback)
 *  (flipSwitch can be a tactile button or a toggle switch) (code taken from Sinric Pro examples then modified)
 *  Click on the following links to learn more. 
 *  YouTube Video: https://youtu.be/gpB4600keWA
 *  Related Blog : https://iotcircuithub.com/esp8266-projects/
 *  by Tech StudyCell
 *  Preferences--> Aditional boards Manager URLs : 
 *  https://dl.espressif.com/dl/package_esp32_index.json, http://arduino.esp8266.com/stable/package_esp8266com_index.json
 *  
 *  Download Board ESP8266 NodeMCU : https://github.com/esp8266/Arduino
 *  Download the libraries
 *  ArduinoJson Library: https://github.com/bblanchon/ArduinoJson
 *  arduinoWebSockets Library: https://github.com/Links2004/arduinoWebSockets
 *  SinricPro Library: https://sinricpro.github.io/esp8266-esp32-sdk/
 *  
 *  If you encounter any issues:
 * - check the readme.md at https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md
 * - ensure all dependent libraries are installed
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#arduinoide
 *   - see https://github.com/sinricpro/esp8266-esp32-sdk/blob/master/README.md#dependencies
 * - open serial monitor and check whats happening
 * - check full user documentation at https://sinricpro.github.io/esp8266-esp32-sdk
 * - visit https://github.com/sinricpro/esp8266-esp32-sdk/issues and check for existing issues or open a new one
 **********************************************************************************/

// Uncomment the following line to enable serial debug output
//#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
       #define DEBUG_ESP_PORT Serial
       #define NODEBUG_WEBSOCKETS
       #define NDEBUG
#endif 

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include <map>

#define WIFI_SSID         "AndroidAPAEC1"    
#define WIFI_PASS         "bzau7300"
#define APP_KEY           "c9cf1eab-2113-4d9c-be7e-f8a370e7b1c8"      // Sinric Pro Bilgilerimizi Giriyoruz.
#define APP_SECRET        "e511181c-8bce-466c-857e-f32b83556f25-a3a69c5f-8620-47ba-91fc-2185a8b47e36"   

//Cihaz kimliklerini buraya girin
#define device_ID_1   "62a33827ff368211543c1ddd"
#define device_ID_2   ""
#define device_ID_3   ""
#define device_ID_4   ""

// Röleler ve anahtarlara bağlı Girişleri tanımlayın

#define RelayPin1 5  //D1
#define RelayPin2 4  //D2
#define RelayPin3 14 //D5
#define RelayPin4 12 //D6

#define SwitchPin1 10  //SD3
#define SwitchPin2 0   //D3 
#define SwitchPin3 13  //D7
#define SwitchPin4 3   //RX

#define wifiLed   16   //D0


#define BAUD_RATE   9600

#define DEBOUNCE_TIME 250

typedef struct {      // struct for the std::map below
  int relayPIN;
  int flipSwitchPIN;
} deviceConfig_t;


// Röle Pin girişlerimiz Ve Switch butonlarımızı Eşleştiriyoruz .

std::map<String, deviceConfig_t> devices = {
    //{deviceId, {relayPIN,  flipSwitchPIN}}
    {device_ID_1, {  RelayPin1, SwitchPin1 }},
    {device_ID_2, {  RelayPin2, SwitchPin2 }},
    {device_ID_3, {  RelayPin3, SwitchPin3 }},
    {device_ID_4, {  RelayPin4, SwitchPin4 }}     
};

typedef struct {     
  String deviceId;
  bool lastFlipSwitchState;
  unsigned long lastFlipSwitchChange;
} flipSwitchConfig_t;

std::map<int, flipSwitchConfig_t> flipSwitches;    // Bu arada , flipSwitch PIN'lerini deviceId ile eşleştirmek ve geri dönme ve son flipSwitch durum kontrollerini işlemek için kullanıyoruz .
                                                 

void setupRelays() { 
  for (auto &device : devices) {           // her cihaz için (röle, flipSwitch kombinasyonu)yapıyoruz .
    int relayPIN = device.second.relayPIN; // Röle Pinini Alıyoruz 
    pinMode(relayPIN, OUTPUT);             // Rölemizi Çıkış Olarak Tanımladık
    digitalWrite(relayPIN, HIGH);         // Rölemizi Tetikliyoruz
  }
}

void setupFlipSwitches() {
  for (auto &device : devices)  {                     // her cihaz için (röle, flipSwitch kombinasyonu)yapıyoruz .
    flipSwitchConfig_t flipSwitchConfig;              // Yeni flipSwitch configuration Oluşturuyoruz

    flipSwitchConfig.deviceId = device.first;         // Cİhaz Kimliklerini Oluşturuyoruz
    flipSwitchConfig.lastFlipSwitchChange = 0;        // geri dönme süresini ayarlıyoruz

    flipSwitchConfig.lastFlipSwitchState = true;     // lastFlipSwitchState'i false (DÜŞÜK) olarak ayarlıyoruz .

    int flipSwitchPIN = device.second.flipSwitchPIN;  // flipSwitchPINi ayarlıyoruz

    flipSwitches[flipSwitchPIN] = flipSwitchConfig;   // flipSwitch yapılandırmasını flipSwitches haritasına kaydedin
    pinMode(flipSwitchPIN, INPUT_PULLUP);                   // flipSwitch pinlerimizi İnput olarak Ayarlıyoruz
  }
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");
  int relayPIN = devices[deviceId].relayPIN; 
  digitalWrite(relayPIN, !state);             
  return true;
}

void handleFlipSwitches() {
  unsigned long actualMillis = millis();                                         
  for (auto &flipSwitch : flipSwitches) {                                         
    unsigned long lastFlipSwitchChange = flipSwitch.second.lastFlipSwitchChange;  //flipSwitch'e en son basıldığında zaman damgasını al . (olayları geri döndürmek / sınırlamak için kullanılır))

    if (actualMillis - lastFlipSwitchChange > DEBOUNCE_TIME) {                    // eğer zaman > geri dönme zamanı ise...


      int flipSwitchPIN = flipSwitch.first;                                       //konfigürasyondan flipSwitch pinini alıyoruz.
      bool lastFlipSwitchState = flipSwitch.second.lastFlipSwitchState;           // ÖncekiFlipSwitchState'i al
      bool flipSwitchState = digitalRead(flipSwitchPIN);                          // mevcut flipSwitch durumunu oku
      if (flipSwitchState != lastFlipSwitchState) {                               // flipSwitchState değiştiyse...
#ifdef TACTILE_BUTTON
        if (flipSwitchState) {                                                    // dokunsal düğmeye basılırsa 
#endif      
          flipSwitch.second.lastFlipSwitchChange = actualMillis;                  // lastFlipSwitchDeğiştirme zamanı güncelleme
          String deviceId = flipSwitch.second.deviceId;                           // config'den deviceId'yi al
          int relayPIN = devices[deviceId].relayPIN;                              // config'den rölePIN'i al

          bool newRelayState = !digitalRead(relayPIN);                            // yeni röle Durumunu ayarla
          digitalWrite(relayPIN, newRelayState);                                  // relay'i yeni duruma ayarla

          SinricProSwitch &mySwitch = SinricPro[deviceId];                        // SinricPro'dan Switch cihazını al
          mySwitch.sendPowerStateEvent(!newRelayState);                            // etkinliği gönder

#ifdef TACTILE_BUTTON
        }
#endif      
        flipSwitch.second.lastFlipSwitchState = flipSwitchState;                  // lastFlipSwitchState'i güncelle
      }
    }
  }
}

void setupWiFi()
{
  Serial.printf("\r\n[Wifi]: Connecting");
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.printf(".");
    delay(250);
  }
  digitalWrite(wifiLed, LOW);
  Serial.printf("connected!\r\n[WiFi]: IP-Address is %s\r\n", WiFi.localIP().toString().c_str());
}

void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

void setup()
{
  Serial.begin(BAUD_RATE);

  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, HIGH);

  setupRelays();
  setupFlipSwitches();
  setupWiFi();
  setupSinricPro();
}

void loop()
{
  SinricPro.handle();
  handleFlipSwitches();
}
