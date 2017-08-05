#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "FS.h"
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
extern "C" {
#include "user_interface.h"
}


#define GPIO_PIN 0
#define LED_PIN 2 //NodeMCU
//#define LED_PIN 13 //Sonoff


void changeLED() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
}


#include <Ticker.h>
Ticker ticker;
Ticker tickerPing;

#define DEFAULT_NAME        "newdevice"    // Enter your device friendly name
#define DEFAULT_SSID        ""             // Enter your network SSID
#define DEFAULT_KEY         ""             // Enter your network WPA key
#define DEFAULT_PASSWORD    "moteconfig"   // Enter network WPA key for AP (config) mode

#define DEFAULT_USE_STATIC_IP   false           // true or false enabled or disabled set static IP
#define DEFAULT_IP          "192.168.0.50"      // Enter your IP address
#define DEFAULT_DNS         "192.168.0.1"       // Enter your DNS
#define DEFAULT_GW          "192.168.0.1"       // Enter your gateway
#define DEFAULT_SUBNET      "255.255.255.0"     // Enter your subnet

#define ESP_PROJECT_PID 2016110801L
#define VERSION 170723

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

  char          Name[32];
  char          Password[26];

  int8_t       chi[3]; //channel input
  int8_t       cho[3]; //channel output
} Settings;

#include <ESP8266WebServer.h>
ESP8266WebServer server (80);

boolean wifiSetup = false;
boolean tryToConnect = false;
/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;
#include <DNSServer.h>
// Setup DNS, only used if the ESP has no valid WiFi config
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
DNSServer dnsServer;

#include <ESP8266HTTPClient.h>
HTTPClient http;

/********************************************************
  /*  Debug Print                                         *
  /********************************************************/
void dbg_printf ( const char *format, ... )
{

  static char sbuf[1400];                                                     // For debug lines
  va_list varArgs;                                                            // For variable number of params

  va_start ( varArgs, format );                                               // Prepare parameters
  vsnprintf ( sbuf, sizeof ( sbuf ), format, varArgs );                       // Format the message
  va_end ( varArgs );                                                         // End of using parameters

  Serial.print ( sbuf );

}

WiFiEventHandler onConnectedHandler;
WiFiEventHandler onGotIPHandler;
WiFiEventHandler onDisconnectedHandler;

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  ticker.attach(0.2, changeLED);

  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting...");
  Serial.println( "Compiled: " __DATE__ ", " __TIME__);
  Serial.print("Flash size: ");
  Serial.println(ESP.getFlashChipRealSize());

  Serial.print(F("Version: "));
  Serial.println(VERSION);

  fileSystemCheck();
  LoadSettings();

  if (Settings.Version != VERSION || Settings.PID != ESP_PROJECT_PID)
  {
    Serial.println(F("INIT : Incorrect PID or version!"));
    delay(1000);
    ResetFactory();
  }

  if (strcasecmp(Settings.WifiSSID, "ssid") == 0)
    wifiSetup = true;


  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters

  char hostString[16] = {0};
  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  ArduinoOTA.setHostname(hostString);

  // setup ssid for AP Mode when needed
  WiFi.softAP(hostString, Settings.Password);
  dbg_printf ("[AP] Configured, Host: %s, Password: %s\n", hostString, Settings.Password);

  // We start in STA mode
  Serial.printf("Wi-Fi mode set to WIFI_STA %s\n", WiFi.mode(WIFI_STA) ? "" : "Failed!");

  if (WifiConnect(3)) {
    ticker.detach();
    WiFi.setAutoReconnect(false);
  } else {
    delay(2000);
    WifiAPMode(true);
    tryToConnect = true;
  }


  //Serial.print(__DATE__);
  //Serial.println("\n");

  //enable MDNS
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    MDNS.addService("espmote", "tcp", 80);
  }

  OTAConfigure();

  // Start the server
  setupServer();

  // Turn led off and set interrupt
  pinMode(GPIO_PIN, INPUT);
  attachInterrupt(GPIO_PIN, buttonChange, CHANGE);

  //  ticker.detach();
  digitalWrite(LED_PIN, HIGH);

  // Start DNS, only used if the ESP has no valid WiFi config
  // It will reply with it's own address on all DNS requests
  // (captive portal concept)
  if (wifiSetup)
    dnsServer.start(DNS_PORT, "*", apIP);

//  WiFi.onEvent(eventWiFi);
  onConnectedHandler = WiFi.onStationModeConnected(&onConnected);
  onGotIPHandler = WiFi.onStationModeGotIP(&onGotIP);
  onDisconnectedHandler = WiFi.onStationModeDisconnected(&onDisconnected);
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

  if (tryToConnect)
  {
    // try to connect for setup wizard
    if (millis() > (lastConnectTry + 15000) ) {
      Serial.print("Loop connecting... ");
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      lastConnectTry = millis();
      if (WifiConnect(1)) {
        WiFi.setAutoReconnect(false);
        Serial.print("Loop conected ... ");
        ticker.detach();


        WifiAPMode(false);
        tryToConnect = false;
      }
    }
  }

  backgroundtasks();
}

void backgroundtasks()
{
  // process DNS, only used if the ESP has no valid WiFi config
  if (wifiSetup)
    dnsServer.processNextRequest();

  server.handleClient();

  sendPing();

  yield();
}


/********************************************************
  /*  Handle WiFi events                                  *
  /********************************************************/
void eventWiFi(WiFiEvent_t event) {

  switch (event) {
    case WIFI_EVENT_STAMODE_CONNECTED:
      dbg_printf("[WiFi] %d, Connected\n", event);
      break;

    case WIFI_EVENT_STAMODE_DISCONNECTED:
      dbg_printf("[WiFi] %d, Disconnected - Status %d, %s\n", event, WiFi.status(), connectionStatus( WiFi.status() ).c_str() );
      break;

    case WIFI_EVENT_STAMODE_AUTHMODE_CHANGE:
      dbg_printf("[WiFi] %d, AuthMode Change\n", event);
      break;

    case WIFI_EVENT_STAMODE_GOT_IP:
      sendPingFlag();
      tickerPing.attach(300, sendPingFlag);
      dbg_printf("[WiFi] %d, Got IP\n", event);
      //      setupOTA();
      break;

    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
      dbg_printf("[WiFi] %d, DHCP Timeout\n", event);
      break;

    case WIFI_EVENT_SOFTAPMODE_STACONNECTED:
      dbg_printf("[AP] %d, Client Connected\n", event);
      break;

    case WIFI_EVENT_SOFTAPMODE_STADISCONNECTED:
      dbg_printf("[AP] %d, Client Disconnected\n", event);
      break;

    case WIFI_EVENT_SOFTAPMODE_PROBEREQRECVED:
      //      dbg_printf("[AP] %d, Probe Request Recieved\n", event);
      break;
  }

}

/********************************************************
  /*  WiFi Connection Status                              *
  /********************************************************/
String connectionStatus ( int which )
{
  switch ( which )
  {
    case WL_CONNECTED:
      return "Connected";
      break;

    case WL_NO_SSID_AVAIL:
      return "Network not availible";
      break;

    case WL_CONNECT_FAILED:
      return "Wrong password";
      break;

    case WL_IDLE_STATUS:
      return "Idle status";
      break;

    case WL_DISCONNECTED:
      return "Disconnected";
      break;

    default:
      return "Unknown";
      break;
  }
}

void onConnected(const WiFiEventStationModeConnected& evt) {
  Serial.print("---Station connected: ");
  Serial.println(evt.ssid.c_str());
}

void onDisconnected(const WiFiEventStationModeDisconnected& evt) {
  Serial.print("---Station disconnected: ");
  Serial.print(evt.ssid.c_str());
  Serial.print(" reason: ");
  Serial.println(evt.reason);
//  if (evt.reason == WIFI_DISCONNECT_REASON_AUTH_LEAVE) {
    if (!tryToConnect) {
      dbg_printf("tryToConnect\r\n");
      tickerPing.detach();
      ticker.attach(0.2, changeLED);
      tryToConnect = true;
    }
//  }
}

void onGotIP(WiFiEventStationModeGotIP ipInfo) { // As soon WiFi is connected, start NTP Client
  tryToConnect = false;
  Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString().c_str());
  sendPingFlag();
  tickerPing.attach(300, sendPingFlag);
  ticker.detach();
  digitalWrite(LED_PIN, HIGH);
}
