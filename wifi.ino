void WifiAPMode(boolean state)
{
//  ticker.detach();
  if (state)
  {
    //    Serial.printf("Enabling soft-AP %s\n", WiFi.mode(WIFI_AP_STA) ? "..." : "Failed!");
    Serial.printf("Enabling soft-AP\n");
    WiFi.enableAP(true);
    ticker.attach(0.5, changeLED);
  }
  else
  {
    //    Serial.printf("Disabling soft-AP %s\n", WiFi.mode(WIFI_STA) ? "..." : "Failed!");
    Serial.printf("Disabling soft-AP\n");
    WiFi.enableAP(false);
  }
  delay(500); // Without delay I've seen the IP address blank
  MDNS.notifyAPChange();
}


boolean WifiConnect(byte connectAttempts)
{
  String log = "";

//  char hostName[sizeof(Settings.Name)];
//  strcpy(hostName, Settings.Name);
//  for (byte x = 0; x < sizeof(hostName); x++)
//    if (hostName[x] == ' ')
//      hostName[x] = '-';
//  wifi_station_set_hostname(hostName);

  if (Settings.IP[0] != 0 && Settings.IP[0] != 255)
  {
    char str[20];
    sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.IP[0], Settings.IP[1], Settings.IP[2], Settings.IP[3]);
    Serial.println("IP   : Static IP :");
    IPAddress ip = Settings.IP;
    IPAddress gw = Settings.Gateway;
    IPAddress subnet = Settings.Subnet;
    IPAddress dns = Settings.DNS;
    WiFi.config(ip, gw, subnet, dns);
  }


  if (WiFi.status() != WL_CONNECTED)
  {
    if ((Settings.WifiSSID[0] != 0)  && (strcasecmp(Settings.WifiSSID, "ssid") != 0))
    {
      for (byte tryConnect = 1; tryConnect <= connectAttempts; tryConnect++)
      {
        Serial.print("WIFI : Connecting... ");
        Serial.println(tryConnect);

        if (tryConnect == 1)
        {
          WiFi.begin(Settings.WifiSSID, Settings.WifiKey);
        }
        else
          WiFi.begin();

        for (byte x = 0; x < 20; x++)
        {
          if (WiFi.status() != WL_CONNECTED)
          {
            delay(500);
          }
          else
            break;
        }
        if (WiFi.status() == WL_CONNECTED)
        {
          Serial.print("WIFI : Connected! IP: ");
          IPAddress ip = WiFi.localIP();
          char str[20];
          sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
          Serial.println(str);

          break;
        }
        else
        {
          Serial.println("WIFI : Disconnecting!");
          ETS_UART_INTR_DISABLE();
          wifi_station_disconnect();
          ETS_UART_INTR_ENABLE();
          delay(1000);
        }
      }

    }
    else
    {
      Serial.println("WIFI : No SSID!");
    }
  }

  if (WiFi.status() == WL_CONNECTED)
    return true;

  return false;
}
