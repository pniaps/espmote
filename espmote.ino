#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "OTA.h"
#include "Server.h"

#define GPIO_PIN 0
#define LED_PIN 2
#define VERSION 170305
#define MDNS_DEBUG_ERR
#define MDNS_DEBUG_TX
#define MDNS_DEBUG_RX
#include <Ticker.h>
Ticker ticker;

#define DEFAULT_NAME        "newdevice"         // Enter your device friendly name
#define DEFAULT_SSID        "ssid"              // Enter your network SSID
#define DEFAULT_KEY         "wpakey"            // Enter your network WPA key
#define DEFAULT_AP_KEY      "configesp"         // Enter network WPA key for AP (config) mode

#define DEFAULT_USE_STATIC_IP   false           // true or false enabled or disabled set static IP
#define DEFAULT_IP          "192.168.0.50"      // Enter your IP address
#define DEFAULT_DNS         "192.168.0.1"       // Enter your DNS
#define DEFAULT_GW          "192.168.0.1"       // Enter your gateway
#define DEFAULT_SUBNET      "255.255.255.0"     // Enter your subnet

#define ESP_PROJECT_PID           2016110801L
#define VERSION                             2

struct SettingsStr
{
  unsigned long PID;
  int           Version;
  byte          IP[4];
  byte          Gateway[4];
  byte          Subnet[4];
  byte          DNS[4];
  char          WifiSSID[32];
  char          WifiKey[64];
  char          WifiSSID2[32];
  char          WifiKey2[64];
  char          WifiAPKey[64];
  char          Name[26];
  char          Password[26];
} Settings;



//#include <WiFiManager.h>

//const char* ssid     = "Einp";
//const char* password = "968208854";
char hostString[16] = {0};

//gets called when WiFiManager enters configuration mode
//void configModeCallback (WiFiManager *myWiFiManager) {
//  Serial.println("Entered config mode");
//  Serial.println(WiFi.softAPIP());
//  if you used auto generated SSID, print it
//  Serial.println(myWiFiManager->getConfigPortalSSID());
//  entered config mode, make led toggle faster
//  ticker.attach(0.2, changeLED);
//}

boolean wifiSetup = false;
boolean wifiSetupConnect = false;
#include <DNSServer.h>
// Setup DNS, only used if the ESP has no valid WiFi config
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;


void enableAP()
{
  //create AP with password
  Serial.print("Setting soft-AP ... ");
  Serial.print(WiFi.hostname().c_str());
  Serial.println(WiFi.softAP("NUEVO", "12345678") ? " Ready" : " Failed!");

  delay(500); // Without delay I've seen the IP address blank
  Serial.print(F("Started access point. IP address: "));
  Serial.println(WiFi.softAPIP());
}

void disableAP()
{
}

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  ticker.attach(0.2, changeLED);

  Serial.begin(115200);
  Serial.println();
  Serial.println();

  fileSystemCheck();
  LoadSettings();

  if (strcasecmp(Settings.WifiSSID, "ssid") == 0)
    wifiSetup = true;


  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters

  char hostString[16] = {0};
  sprintf(hostString, "ESP_%06X_SETUP", ESP.getChipId());
  WiFi.hostname(hostString);

  // setup ssid for AP Mode when needed
  WiFi.softAP(hostString, Settings.WifiAPKey);
  // We start in STA mode
  WiFi.mode(WIFI_STA);

  if (!WifiConnect(true, 3))
    WifiConnect(false, 3);


  //  unsigned long _configPortalStart = millis();
  //  Serial.println("Connecting to station ...");
  //  WiFi.begin("Einp", "968208854");
  //  while (WiFi.status() != WL_CONNECTED) {
  //    delay(250);
  //    Serial.print(".");
  //    if (millis() > _configPortalStart + 10000) {
  //      //tiemout de 5 segundos
  //      break;
  //    }
  //  }
  //  Serial.println("");
  //  if (millis() > _configPortalStart + 10000) {
  //    enableAP();
  //  } else {
  //    Serial.print("Connected to ");
  //    Serial.println(WiFi.SSID());
  //    Serial.print("IP address: ");
  //    Serial.println(WiFi.localIP());
  //  }

  //  Serial.println("\n");
  //  Serial.println("\n");
  //  Serial.println("setup");
  //  Serial.print("Version: ");
  //  Serial.print(VERSION);
  //  Serial.print(" ");
  //Serial.print(__DATE__);
  //Serial.println("\n");
  //Serial.print("Chip ID: 0x");
  //Serial.println(ESP.getChipId(), HEX);
  //Serial.print("FlashChipId: 0x");
  //Serial.println(ESP.getFlashChipId(), HEX);
  //Serial.print("FlashChipSize: ");
  //  Serial.println(ESP.getFlashChipSize());
  //  Serial.print("aFlashChipRealSize: ");
  //  Serial.println(ESP.getFlashChipRealSize());
  //  Serial.print("getFlashChipSizeByChipId: 0x");
  //  Serial.println(ESP.getFlashChipSizeByChipId(), HEX);

  //Generate Hostname
  //  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  //  Serial.print("Hostname => ");
  //  Serial.println(WiFi.hostname().c_str());
  //  WiFi.hostname(hostString);

  //  WiFiManager wifiManager;
  //  wifiManager.setAPCallback(configModeCallback);
  //  if (!wifiManager.autoConnect()) {
  //    Serial.println("failed to connect and hit timeout");
  //reset and try again, or maybe put it to deep sleep
  //    ESP.reset();
  //    delay(1000);
  //  }


  //Connect to wifi
  //  WiFi.begin(ssid, password);
  //  while (WiFi.status() != WL_CONNECTED) {
  //    delay(250);
  //    Serial.print(".");
  //  }
  //  Serial.println("");
  //  Serial.print("Connected to ");
  //  Serial.println(WiFi.SSID());
  //  Serial.print("IP address: ");
  //  Serial.println(WiFi.localIP());

  //enable MDNS
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.begin(hostString);
  MDNS.addService("http", "tcp", 80); // Announce esp tcp service on port 8080
  MDNS.addService("espmote", "tcp", 80); // Announce esp tcp service on port 8080

  OTAConfigure();

  // Start the server
  setupServer();
  Serial.println("Server started");

  // Turn led off and set interrupt
  pinMode(GPIO_PIN, INPUT);
  attachInterrupt(GPIO_PIN, buttonChange, CHANGE);

  ticker.detach();
  digitalWrite(BUILTIN_LED, LOW);

  // Start DNS, only used if the ESP has no valid WiFi config
  // It will reply with it's own address on all DNS requests
  // (captive portal concept)
  if (wifiSetup)
    dnsServer.start(DNS_PORT, "*", apIP);

}

void changeLED() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}

int buttonState = 0;     // current state of the button
int startPressed = 0;    // the time button was pressed
int endPressed = 0;      // the time button was released
int timeHold = 0;        // the time button is hold
int timeReleased = 0;    // the time button is released

void buttonChange() {
  delay(250);

  buttonState = digitalRead(GPIO_PIN);
  if (buttonState == LOW) {
    startPressed = millis();
    Serial.println("Button pressed");
  } else {
    endPressed = millis();
    timeHold = endPressed - startPressed;
    Serial.println("Button released");

    if (timeHold >= 3000) {
      //change configuration portal
      Serial.println("Button hold for three seconds or more");
      bool isEnabled = ((WiFi.getMode() & WIFI_AP) != 0);
      if (isEnabled) {
        Serial.print("Disabling soft-AP ... ");
        Serial.println(WiFi.enableAP(false) ? " OK" : " Failed!");
        delay(500);
      } else {
        Serial.print("Enabling soft-AP ... ");
        Serial.println(WiFi.enableAP(true) ? " OK" : " Failed!");
        //        Serial.print(WiFi.hostname().c_str());
        //        Serial.println(WiFi.softAP(WiFi.hostname().c_str(), "12345678") ? " Ready" : " Failed!");
        delay(500);
      }
    } else {
      Serial.println("Button hold for no mor than three seconds");
      digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    }

  }
}


void loop()
{
  ArduinoOTA.handle();

  if (wifiSetupConnect)
  {
    // try to connect for setup wizard
    WifiConnect(true,1);
    wifiSetupConnect = false;
  }

  backgroundtasks();
}

void backgroundtasks()
{
  // process DNS, only used if the ESP has no valid WiFi config
  if (wifiSetup)
    dnsServer.processNextRequest();

  server.handleClient();

  yield();
}
