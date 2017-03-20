void fileSystemCheck()
{
  if (SPIFFS.begin())
  {
    fs::File f = SPIFFS.open("settings.dat", "r");
    if (!f)
    {
      ResetFactory();
    }
  }
}

void ResetFactory(void)
{


  // Direct Serial is allowed here, since this is only an emergency task.
  Serial.println(F("Resetting factory defaults..."));
  delay(1000);

  //always format on factory reset, in case of corrupt SPIFFS
  SPIFFS.end();
  Serial.println(F("formatting..."));
  SPIFFS.format();
  Serial.println(F("formatting done..."));
  if (!SPIFFS.begin())
  {
    Serial.println(F("FORMATTING SPIFFS FAILED!"));
    return;
  }


  fs::File f = SPIFFS.open("settings.dat", "w");
  if (f)
  {
    for (int x = 0; x < sizeof(struct SettingsStr); x++)
      f.write(0);
    f.close();
  }

  LoadSettings();
  // now we set all parameters that need to be non-zero as default value

  Settings.PID             = ESP_PROJECT_PID;
  Settings.Version         = VERSION;

#if DEFAULT_USE_STATIC_IP
  str2ip((char*)DEFAULT_IP, Settings.IP);
  str2ip((char*)DEFAULT_GW, Settings.Gateway);
  str2ip((char*)DEFAULT_SUBNET, Settings.Subnet);
  str2ip((char*)DEFAULT_DNS, Settings.DNS);
#endif

  strcpy_P(Settings.WifiSSID, PSTR(DEFAULT_SSID));
  strcpy_P(Settings.WifiKey, PSTR(DEFAULT_KEY));
  strcpy_P(Settings.WifiAPKey, PSTR(DEFAULT_AP_KEY));
  strcpy_P(Settings.Name, PSTR(DEFAULT_NAME));
  Settings.Password[0] = 0;

  SaveSettings();

  Serial.println("Factory reset succesful, rebooting...");
  delay(1000);
  WiFi.persistent(true); // use SDK storage of SSID/WPA parameters
  WiFi.disconnect(); // this will store empty ssid/wpa into sdk storage
  WiFi.persistent(false); // Do not use SDK storage of SSID/WPA parameters
  ESP.reset();
}

boolean SaveSettings(void)
{
  return SaveToFile((char*)"settings.dat", 0, (byte*)&Settings, sizeof(struct SettingsStr));
}
boolean LoadSettings()
{
  LoadFromFile((char*)"settings.dat", 0, (byte*)&Settings, sizeof(struct SettingsStr));
}

void LoadFromFile(char* fname, int index, byte* memAddress, int datasize)
{
  fs::File f = SPIFFS.open(fname, "r+");
  if (f)
  {
    f.seek(index, fs::SeekSet);
    byte *pointerToByteToRead = memAddress;
    for (int x = 0; x < datasize; x++)
    {
      *pointerToByteToRead = f.read();
      pointerToByteToRead++;// next byte
    }
    f.close();
  }
}

boolean SaveToFile(char* fname, int index, byte* memAddress, int datasize)
{
  boolean success = false;

  fs::File f = SPIFFS.open(fname, "r+");
  if (f)
  {
    f.seek(index, fs::SeekSet);
    byte *pointerToByteToSave = memAddress;
    for (int x = 0; x < datasize ; x++)
    {
      f.write(*pointerToByteToSave);
      pointerToByteToSave++;
    }
    f.close();
    Serial.println("FILE : File saved");
    success = true;
  }
  return success;
}



