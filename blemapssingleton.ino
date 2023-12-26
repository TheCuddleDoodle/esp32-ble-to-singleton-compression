#include <TFT_eSPI.h>
#include "Latin_Hiragana_24.h"
#include "NotoSansBold15.h"
#include "NotoSansMonoSCB20.h"
#include <WiFi.h>
#include "time.h"
#include <HTTPClient.h>
#define FS_NO_GLOBALS
#include <FS.h>
#include "SPIFFS.h"
#include <BLEDevice.h>
#include <TJpg_Decoder.h>
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <ArduinoJson.h>
#include <Base64.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite sprite = TFT_eSprite(&tft);

// BLE SECTION
BLEServer *pServer = NULL;

BLECharacteristic *message_characteristic = NULL;
BLECharacteristic *box_characteristic = NULL;
BLECharacteristic *maptext_characteristic = NULL;

String boxValue = "0";
String maptextValue = "0";
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

#define MESSAGE_CHARACTERISTIC_UUID "6d68efe5-04b6-4a85-abc4-c2670b7bf7fd"
#define BOX_CHARACTERISTIC_UUID "f27b53ad-c63d-49a0-8c0f-9f297e6cc520"
#define MAPTEXT_CHARACTERISTIC_UUID "da66bf2e-05b8-4579-8672-7594312cda24"

#define BUTTON_PIN 16
const char *ssid = "VAIBHAV";
const char *password = "12345678";

#define latin Latin_Hiragana_24
#define small NotoSansBold15
#define digits NotoSansMonoSCB20

#define maingreen 0x5a6 // google maps green
#define c1 0xE73C       // whitea
#define c2 0x18C3       // gray
#define c3 0x9986       // red
#define c4 0x2CAB       // green
#define c5 0xBDEF       // gold
#define bggreen 0x241   // bggreen

int fromTop = 34;
int left = 40;
int width = 320;
int heigth = 170;

uint16_t grays[60] = {0};
uint16_t lines[11] = {0};
int sec = 0;
int pos = 0;
int drawn = 0;

int digit1 = 0;
int digit2 = 0;
int digit3 = 0;
int digit4 = 0;

long start = 0;
long end = 0;
int counter = 0;
double fps = 0;

char timeHour[3];
char timeMin[3];
char timeSec[3];

void maptextrender(String text,String subtext)
{
     sprite.fillSprite(bggreen);
    
    //setting frontal
     sprite.fillSmoothRoundRect(5,4,width-10,heigth-8,28,maingreen);
    
    //init font
     sprite.loadFont(digits);
     sprite.setTextColor(c1);
     if(text.length() > 20 ){
       sprite.drawString(text.substring(0, text.length()/2),240/1.2,170/3.5);
       sprite.drawString(text.substring(text.length()/2, text.length()),240/1.2,170/2.4);       
     }
     else{
        sprite.drawString(text,240/1.2,170/3.5);
     }
     
     sprite.setTextColor(c1);
      if(text.length() > 20 ){
       sprite.drawString(subtext.substring(0, subtext.length()/2),240/1.2,170/1.6);
       sprite.drawString(subtext.substring(subtext.length()/2, subtext.length()),240/1.2,170/1.2);       
     }
     else{
        sprite.drawString(subtext,240/1.2,170/1.5);
     }
     sprite.pushSprite(0, 0);
           
}

void generateIcon(String encodedString,int xt, int yt){
  
  String hold_num = "";
  int xlimit = xt+60; 
  int ylimit = yt+60; 
  int x = xt+0;
  int y = yt+0;
  for (int i = 0 ; i < encodedString.length() ;i++ ){
    if(isDigit(encodedString[i])){
      hold_num += encodedString[i];
    }
    else{
      int count = hold_num.toInt();
           for (int j = 0; j < count; j++) {
            if (y == ylimit) {
                x++;
                y = yt;                
            }
            if(encodedString[i] == 'N'){
              sprite.drawPixel(y , x, 0xFFFF);          
            }
            y++;
    }
    hold_num = "";
  }
  }
  sprite.pushSprite(0, 0);
  hold_num="";
}


class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    Serial.println("Connected");
  };

  void onDisconnect(BLEServer *pServer)
  {
    Serial.println("Disconnected");
  }
};






class CharacteristicsCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    Serial.print("Value Written ");
    Serial.println(pCharacteristic->getValue().c_str());

    if(pCharacteristic == box_characteristic){
      delay(200);
      generateIcon(pCharacteristic->getValue().c_str(), 50, 20);      
      boxValue = pCharacteristic->getValue().c_str();
      box_characteristic->setValue(const_cast<char *>(boxValue.c_str()));
      box_characteristic->notify();
    }
    if(pCharacteristic == message_characteristic){
      StaticJsonDocument<200> doc;
      DeserializationError error = deserializeJson(doc, pCharacteristic->getValue().c_str());
 
      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed with code "));
        Serial.println(error.f_str());
        return;
      }
      
      
      const char* text = doc["text"]; 
      const char* subtext = doc["bigtext"]; 
      maptextrender(text, subtext);
      maptextValue = pCharacteristic->getValue().c_str();
      message_characteristic->setValue(const_cast<char *>(maptextValue.c_str()));
      message_characteristic->notify();
    }
  }
};


void setup()
{
    //   pinMode(BUTTON_PIN,INPUT_PULLUP);
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    Serial.begin(9600);
    pinMode(15, OUTPUT);
    digitalWrite(15, 1);
    sprite.createSprite(320, 170);
    sprite.setTextDatum(4);


    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }

    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }


  //Bluetooth setup 
  BLEDevice::init("NotificationServer");
  BLEDevice::setMTU(500);
  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  delay(100);

  // Create a BLE Characteristic
  message_characteristic = pService->createCharacteristic(
      MESSAGE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  box_characteristic = pService->createCharacteristic(
      BOX_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  
  maptext_characteristic = pService->createCharacteristic(
      MAPTEXT_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE |
          BLECharacteristic::PROPERTY_NOTIFY |
          BLECharacteristic::PROPERTY_INDICATE);

  // Start the BLE service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();

  message_characteristic->setValue("Message one");
  message_characteristic->setCallbacks(new CharacteristicsCallbacks());

  box_characteristic->setValue("0");
  box_characteristic->setCallbacks(new CharacteristicsCallbacks());

  maptext_characteristic->setValue("0");
  maptext_characteristic->setCallbacks(new CharacteristicsCallbacks());

  Serial.println("Waiting for a client connection to notify...");
  Serial.println("Got it started");
    
 
}







void loop()
{
 
    
   
}
