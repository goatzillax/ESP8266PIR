#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WEMOS_SHT3X.h>
#include <ESP8266WiFi.h>
#include <Font4x7Fixed.h>
#include <HTTPSRedirect.h>

#include "secrets.h"

SHT3X sht30(0x45);

float value1 = 3.2f;  //  Fill in with voltage once I wire it up.

#define OLED_RESET -1  // Reset pin is not on a GPIO on Wemos D1 OLED Shield
Adafruit_SSD1306 display(OLED_RESET);

unsigned long displayTimeOutStart = 0;
#define displayInterval 2000

void displayRefresh() {
  displayTimeOutStart = millis() - displayInterval;
}

const char *GScriptId = GSCRIPTID;

// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"PIR\", \"values\": ";
String payload = "";

const char* host = "script.google.com";
const int httpsPort = 443;
const char* fingerprint = "";
String url = String("/macros/s/") + GScriptId + "/exec?cal";

HTTPSRedirect* client = nullptr;

const int PIR = D3;
int PIRState = 0;  //  this mostly reflects the hardware state
int PIREvent = 0;  //  this reflects "dosumboutit"
unsigned long failberts = 0;  // Todo:  store failberts in RTC for deep sleep.
unsigned long pushStartTime;
unsigned long pushInterval;
#define START_PUSH_INTERVAL 1000

void ICACHE_RAM_ATTR handlePIR();

void handlePIR() {
  PIRState = digitalRead(PIR);
  if (PIRState == 1) {
     //  reset any timeouts and force a push
     PIREvent = 1;
     pushStartTime = millis() - START_PUSH_INTERVAL;
     pushInterval = START_PUSH_INTERVAL;
  }
  displayRefresh();
}

void setup() {
  Serial.begin(115200);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  WiFi.persistent(false);
  WiFi.hostname(WIFI_HOSTNAME);
  WiFi.mode(WIFI_STA);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  //  Not really sure if I give a shit if it connects right now
#if 0
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
#endif

  //  Vaguely useful for debugging
#if 0
  pinMode(BUILTIN_LED, OUTPUT);
  // set initial state, LED off
  digitalWrite(BUILTIN_LED, HIGH);
#endif

  pinMode(PIR, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR),handlePIR, CHANGE);
}

void loop() {
  unsigned long rightMeow = millis();

#if 0
  digitalWrite(BUILTIN_LED, digitalRead(PIR) ? LOW : HIGH);
#endif

  if (rightMeow - displayTimeOutStart >= displayInterval) {
    displayTimeOutStart = rightMeow;

    display.clearDisplay();
    display.setTextColor(WHITE);

    display.setFont(&Font4x7Fixed);
    display.setTextWrap(false);
    display.setTextSize(1);
    display.setCursor(0, 7);  //  custom fonts go "up"?
    display.println(WiFi.localIP());

    display.setFont();  //  return to system font
    display.setTextWrap(true);
    display.setTextSize(1);
    display.setCursor(0, 8);

    if(sht30.get()==0){
      display.println(String(sht30.cTemp)+"C");
      display.println(String(sht30.humidity)+"%");
    }
    else
    {
      display.println("Error!");
    }

    if (PIRState == 1) {
       display.println("PIR");
    }

    display.display();
  }

  if (PIREvent && (rightMeow - pushStartTime >= pushInterval)) {
    PIREvent = 0;		//  immediately "clear" it so we have less chance of missing an edge
    pushStartTime = rightMeow;	//  same reason; if another edge comes in during failure it can override.
    pushInterval *= 2;		//  increase push interval for each successive fail

    //  oh yeah, this is totally way better than a simple printf and not at all a pile of gratuitous typing
    //  "but but printf is wrong for everybody everywhere but but but!"
    payload = payload_base + "\"" + WIFI_HOSTNAME + "," + value1 + "," + failberts + "\"}";


    Serial.println("Publishing data");

    if (client == nullptr) {
      client = new HTTPSRedirect(httpsPort);
      client->setInsecure();
      client->setPrintResponseBody(true);
      client->setContentTypeHeader("application/json");
    }

    if (!client->connected()) {
      if (1 != client->connect(host, httpsPort)) {
        Serial.println("error connecting");
        goto fail;
      }
    }

    if (!client->POST(url, host, payload)) {
       Serial.println("error posting");
       goto fail;
    }
    failberts = 0;
    goto out;

fail:
    failberts++;
    PIREvent = 1;       //  re-set the event in the case of failure.  I WANT A DAMN SEMAPHORE.
  }
out:
  //  Need a scheduler, not this loopy trash.
  delay(100);
}
