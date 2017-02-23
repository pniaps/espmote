#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "OTA.h"
#include "Server.h"

#define GPIO_PIN 0 
#define LED_PIN 2

const char* ssid     = "Einp";
const char* password = "968208854";
char hostString[16] = {0};

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  Serial.begin(115200);
  Serial.println("setup");

  Serial.println("\r\n");
  Serial.print("Chip ID: 0x");
  Serial.println(ESP.getChipId(), HEX);
  Serial.print("FlashChipId: 0x");
  Serial.println(ESP.getFlashChipId(), HEX);
  Serial.print("FlashChipSize: ");
  Serial.println(ESP.getFlashChipSize());
  Serial.print("tFlashChipRealSize: ");
  Serial.println(ESP.getFlashChipRealSize());

  //Generate Hostname
  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  Serial.print("Hostname: ");
  Serial.println(hostString);
  WiFi.hostname(hostString);

  //Connect to wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //enable MDNS
  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80); // Announce esp tcp service on port 8080
  MDNS.addService("espmote", "tcp", 80); // Announce esp tcp service on port 8080

  OTAConfigure();

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Turn led off and set interrupt
  pinMode(GPIO_PIN, INPUT);
  attachInterrupt(GPIO_PIN, changeLED, FALLING);
}

void changeLED(){
  digitalWrite(2, !digitalRead(2));
  Serial.println("buttonChange");
}


void loop()
{
  ArduinoOTA.handle();

  checkPetition();
}
