
#include <memory>

const char HTTP_HEAD[] PROGMEM            = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/><title>{v}</title>";
const char HTTP_STYLE[] PROGMEM           = "<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>";
const char HTTP_SCRIPT[] PROGMEM          = "<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>";
const char HTTP_HEAD_END[] PROGMEM        = "</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>";
const char HTTP_PORTAL_OPTIONS[] PROGMEM  = "<form action=\"/wifi\" method=\"get\"><button>Configure WiFi</button></form><br/><form action=\"/0wifi\" method=\"get\"><button>Configure WiFi (No Scan)</button></form><br/><form action=\"/i\" method=\"get\"><button>Info</button></form><br/><form action=\"/r\" method=\"post\"><button>Reset</button></form>";
const char HTTP_ITEM[] PROGMEM            = "<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>";
const char HTTP_FORM_START[] PROGMEM      = "<form method=\"get\" action=\"wifisave\"><input id=\"s\" name=\"ssid\" length=\"32\" placeholder=\"SSID\"><br/><input id=\"p\" name=\"password\" length=\"64\" type=\"password\" placeholder=\"password\"><br/>";
const char HTTP_FORM_PARAM[] PROGMEM      = "<br/><input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}' {c}>";
const char HTTP_FORM_END[] PROGMEM        = "<br/><button type='submit'>save</button></form>";
const char HTTP_SCAN_LINK[] PROGMEM       = "<br/><div class=\"c\"><a href=\"/wifi\">Scan</a></div>";
const char HTTP_SAVED[] PROGMEM           = "<div>Credentials Saved<br />Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>";
const char HTTP_END[] PROGMEM             = "</div></body></html>";

int getRSSIasQuality(int RSSI) {
  int quality = 0;

  if (RSSI <= -100) {
    quality = 0;
  } else if (RSSI >= -50) {
    quality = 100;
  } else {
    quality = 2 * (RSSI + 100);
  }
  return quality;
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send ( 404, "text/plain", message );
}

void handleReset() {
  String message = "Resetting factory...\n\n";

  server.send ( 200, "text/plain", message );

  ResetFactory();
}

void handleWifi(boolean scan) {

  //  String page = FPSTR(HTTP_HEAD);
  //  page.replace("{v}", "Config ESP");
  //  page += FPSTR(HTTP_SCRIPT);
  //  page += FPSTR(HTTP_STYLE);
  //  //  page += _customHeadElement;
  //  page += FPSTR(HTTP_HEAD_END);
  //
  String page = "";
  addHeader(true, page);

  page += F("<div class='full'><div class='black'>Connect to wifi</div></div><br />");
  if (scan) {
    int n = WiFi.scanNetworks();
    Serial.println(F("Scan done"));
    if (n == 0) {
      Serial.println(F("No networks found"));
      page += F("No networks found. Refresh to scan again.");
    } else {

      //sort networks
      int indices[n];
      for (int i = 0; i < n; i++) {
        indices[i] = i;
      }

      // RSSI SORT

      // old sort
      for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
          if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
            std::swap(indices[i], indices[j]);
          }
        }
      }

      /*std::sort(indices, indices + n, [](const int & a, const int & b) -> bool
        {
        return WiFi.RSSI(a) > WiFi.RSSI(b);
        });*/

      //display networks in page
      for (int i = 0; i < n; i++) {
        if (indices[i] == -1) continue; // skip dups
        Serial.println(WiFi.SSID(indices[i]));
        Serial.println(WiFi.RSSI(indices[i]));
        int quality = getRSSIasQuality(WiFi.RSSI(indices[i]));

        String item = FPSTR(HTTP_ITEM);
        String rssiQ;
        rssiQ += quality;
        item.replace("{v}", WiFi.SSID(indices[i]));
        item.replace("{r}", rssiQ);
        if (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE) {
          item.replace("{i}", "l");
        } else {
          item.replace("{i}", "");
        }
        //Serial.println(item);
        page += item;
        delay(0);

      }
      page += "<br/>";
    }
  }

  page += F("<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>");
  page += F("<form method='post' action='wifisave'><input id='s' name='ssid' length='31' placeholder='SSID'><input id='p' name='password' length='63' type='password' placeholder='password'><div class='full' style=\"text-align:center;\"><button class=\"button\">Submit</button></div></form>");


  addFooter(page);

  server.send(200, "text/html", page);


  Serial.println(F("Sent config page"));
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  Serial.println(F("WiFi save"));

  String ssid = server.arg(F("ssid"));
  String password = server.arg(F("password"));
  if (ssid.length() != 0 && password.length())
  {
    strncpy(Settings.WifiSSID, ssid.c_str(), sizeof(Settings.WifiSSID));
    strncpy(Settings.WifiKey, password.c_str(), sizeof(Settings.WifiKey));
    tryToConnect = true;
  }

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Credentials Saved");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  //  page += _customHeadElement;
  page += FPSTR(HTTP_HEAD_END);
  page += FPSTR(HTTP_SAVED);
  page += FPSTR(HTTP_END);

  server.send(200, "text/html", page);
  SaveSettings();
}


void setupServer()
{
  server.on("/", handle_root);
  server.on("/setup", handle_setup);
  server.on("/wifi", std::bind(&handleWifi, true));
  server.on("/0wifi", std::bind(&handleWifi, false));
  server.on("/wifisave", std::bind(&handleWifiSave));

  //JSON
  server.on("/enable", handle_enable);
  server.on("/disable", handle_disable);
  server.on("/status", handle_status);
  server.on("/scan", handle_scan);

  server.on("/reset", handleReset);

  server.onNotFound ( handleNotFound );

  server.on("/config", handle_config);
  //  server.on("/hardware", handle_hardware);

  server.begin();
  dbg_printf ("[SERVER] Ready\n");
}

void handle_root()
{
  // if Wifi setup, launch setup wizard
  if (WiFi.status() != WL_CONNECTED && server.client().localIP() == WiFi.softAPIP())
  {
    server.send(200, "text/html", F("<meta HTTP-EQUIV='REFRESH' content='0; url=http://192.168.4.1/setup'>"));
    return;
  }

  if (server.hasArg("channel")) {
    setOutputStatus(server.arg("channel").toInt(), server.arg("enable").toInt());
    server.sendHeader("Location", String("http://") + server.client().localIP().toString(), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
  }

  String reply = "";
  addHeader(true, reply);

  reply += F("<div class='grid'>");
  reply += F("<div class='full'><div class='black'>System Info</div></div>");

  reply += F("<div>Version:</div><div>");
  reply += VERSION;
  reply += F("</div>");

  reply += F("<div>Build:</div><div>");
  reply += F( __DATE__ ", " __TIME__);
  reply += F("</div>");

  reply += F("<div>Uptime:</div>");
  char strUpTime[40];
  int minutes = millis() / 1000 / 60;
  int days = minutes / 1440;
  minutes = minutes % 1440;
  int hrs = minutes / 60;
  minutes = minutes % 60;
  sprintf_P(strUpTime, PSTR("<div>%d days %d hours %d minutes</div>"), days, hrs, minutes);
  reply += strUpTime;

  reply += F("<div>Free Mem:</div><div>");
  reply += ESP.getFreeHeap();
  reply += F(" (");
  reply += lowestRAM;
  reply += F(")</div>");

  reply += F("<div>IP:</div><div>");
  char str[20];
  IPAddress ip = WiFi.localIP();
  sprintf_P(str, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
  reply += str;
  reply += F("</div>");

  reply += F("<div>GW:</div><div>");
  IPAddress gw = WiFi.gatewayIP();
  sprintf_P(str, PSTR("%u.%u.%u.%u"), gw[0], gw[1], gw[2], gw[3]);
  reply += str;
  reply += F("</div>");

  reply += F("<div>Wifi SSID:</div><div>");
  if (WiFi.status() == WL_CONNECTED) {
    reply += WiFi.SSID();
  } else {
    reply += F("Not connected");
  }
  reply += F("</div>");

  if (WiFi.status() == WL_CONNECTED)
  {
    reply += F("<div>Wifi RSSI:</div><div>");
    reply += WiFi.RSSI();
    reply += F(" dB");
    reply += F("</div>");
  }

  reply += F("<div>STA MAC:</div><div>");
  uint8_t mac[] = {0, 0, 0, 0, 0, 0};
  uint8_t* macread = WiFi.macAddress(mac);
  char macaddress[20];
  sprintf_P(macaddress, PSTR("%02x:%02x:%02x:%02x:%02x:%02x"), macread[0], macread[1], macread[2], macread[3], macread[4], macread[5]);
  reply += macaddress;
  reply += F("</div>");

  reply += F("<div>AP MAC:</div><div>");;
  macread = WiFi.softAPmacAddress(mac);
  sprintf_P(macaddress, PSTR("%02x:%02x:%02x:%02x:%02x:%02x"), macread[0], macread[1], macread[2], macread[3], macread[4], macread[5]);
  reply += macaddress;
  reply += F("</div>");

  reply += F("<div>ESP Chip ID:</div><div>");
  char ChipId[10];
  sprintf(ChipId, "%06X", ESP.getChipId());
  reply += ChipId;
  reply += F("</div>");

  reply += F("<div>Flash Chip ID:</div><div>");
  sprintf(ChipId, "%06X", ESP.getFlashChipId());
  reply += ChipId;
  reply += F("</div>");

  reply += F("<div>Flash Size:</div><div>");
  reply += ESP.getFlashChipRealSize() / 1024; //ESP.getFlashChipSize();
  reply += F(" kB</div>");

  reply += F("<div>Sketch Size/Free:</div><div>");
  reply += ESP.getSketchSize() / 1024;
  reply += F(" kB / ");
  reply += ESP.getFreeSketchSpace() / 1024;
  reply += F(" kB</div>");

  if (!isOutputDisabled(0)) {
    reply += F("<div>Channel 0:</div><div><a href='/?channel=0&amp;enable=");
    reply += getOutputStatus(0) ? F("0") : F("1");
    reply += F("'>");
    reply += getOutputStatus(0) ? F("Disable") : F("Enable");
    reply += F("</a></div>");
  }

  if (!isOutputDisabled(1)) {
    reply += F("<div>Channel 1:</div><div><a href='/?channel=1&amp;enable=");
    reply += getOutputStatus(1) ? F("0") : F("1");
    reply += F("'>");
    reply += getOutputStatus(1) ? F("Disable") : F("Enable");
    reply += F("</a></div>");
  }

  if (!isOutputDisabled(2)) {
    reply += F("<div>Channel 2:</div><div><a href='/?channel=2&amp;enable=");
    reply += getOutputStatus(2) ? F("0") : F("1");
    reply += F("'>");
    reply += getOutputStatus(2) ? F("Disable") : F("Enable");
    reply += F("</a></div>");
  }

  reply += F("</div>");
  addFooter(reply);
  server.send(200, "text/html", reply);
}

void handle_setup()
{

  String reply = "";
  addHeader(false, reply);

  if (WiFi.status() == WL_CONNECTED && server.client().localIP() == WiFi.softAPIP())
  {
    SaveSettings();
    IPAddress ip = WiFi.localIP();
    char host[20];
    sprintf_P(host, PSTR("%u.%u.%u.%u"), ip[0], ip[1], ip[2], ip[3]);
    reply += F("<BR>ESP is connected and using IP Address: ");
    reply += host;
    reply += F("<BR><BR>Connect your laptop / tablet / phone back to your main Wifi network and ");
    reply += F("<a class=\"button-menu\" href='http://");
    reply += host;
    reply += F("/config'>Proceed to main config</a>");
    addFooter(reply);
    server.send(200, "text/html", reply);
    wifiSetup = false;
    WifiAPMode(false);
    return;
  }

  static byte status = 0;
  static int n = 0;
  static byte refreshCount = 0;
  String ssid = server.arg(F("ssid"));
  String other = server.arg(F("other"));
  String password = server.arg(F("pass"));

  if (other.length() != 0)
  {
    ssid = other;
  }

  // if ssid config not set and params are both provided
  if (status == 0 && ssid.length() != 0 && password.length() != 0)
  {
    strncpy(Settings.WifiKey, password.c_str(), sizeof(Settings.WifiKey));
    strncpy(Settings.WifiSSID, ssid.c_str(), sizeof(Settings.WifiSSID));
    //    wifiSetupConnect = true;
    status = 1;
    refreshCount = 0;
  }

  reply += F("<h1>Wifi Setup wizard</h1><BR>");
  reply += F("<form name='frmselect' method='post'>");

  if (status == 0)  // first step, scan and show access points within reach...
  {
    if (n == 0)
      n = WiFi.scanNetworks();

    if (n == 0)
      reply += F("No Access Points found");
    else
    {
      for (int i = 0; i < n; ++i)
      {
        reply += F("<input type='radio' name='ssid' value='");
        reply += WiFi.SSID(i);
        reply += F("'");
        if (WiFi.SSID(i) == ssid)
          reply += F(" checked ");
        reply += F(">");
        reply += WiFi.SSID(i);
        reply += F("</input><br>");
      }
    }

    reply += F("<input type='radio' name='ssid' id='other_ssid' value='other' >other SSID:</input>");
    reply += F("<input type ='text' name='other' value='");
    reply += other;
    reply += F("'><br><br>");
    reply += F("Password: <input type ='text' name='pass' value='");
    reply += password;
    reply += F("'><br>");

    reply += F("<input type='submit' value='Connect'>");
  }

  if (status == 1)  // connecting stage...
  {

    int wait = 20;
    if (refreshCount != 0)
      wait = 3;
    reply += F("Please wait for <h1 id=\"countdown\">20..</h1>");
    reply += F("<script type=\"text/JavaScript\">");
    reply += F("function timedRefresh(timeoutPeriod) {");
    reply += F("   var timer = setInterval(function() {");
    reply += F("   if (timeoutPeriod > 0) {");
    reply += F("       timeoutPeriod -= 1;");
    reply += F("       document.getElementById(\"countdown\").innerHTML = timeoutPeriod + \"..\" + \"<br />\";");
    reply += F("   } else {");
    reply += F("       clearInterval(timer);");
    reply += F("            window.location.href = window.location.href;");
    reply += F("       };");
    reply += F("   }, 1000);");
    reply += F("};");
    reply += F("timedRefresh(");
    reply += wait;
    reply += F(");");
    reply += F("</script>");
    reply += F("seconds while trying to connect");

    refreshCount++;
  }

  reply += F("</form>");
  addFooter(reply);
  server.send(200, "text/html", reply);
  delay(10);
}

void addPinStateSelect(String& str, String name,  int value)
{
  String options[15];
  options[0] = F("Disabled");
  options[1] = F("GPIO4 Low");
  options[2] = F("GPIO4 High");
  options[3] = F("GPIO5 Low");
  options[4] = F("GPIO5 High");
  options[5] = F("GPIO12 Low");
  options[6] = F("GPIO12 High");
  options[7] = F("GPIO13 Low");
  options[8] = F("GPIO13 High");
  options[9] = F("GPIO14 Low");
  options[10] = F("GPIO14 High");
  options[11] = F("GPIO16 Low");
  options[12] = F("GPIO16 High");
  options[13] = F("GPIO2 Low");
  options[14] = F("GPIO2 High");
  int values[15];
  values[0] = 0;
  values[1] = -4;
  values[2] = 4;
  values[3] = -5;
  values[4] = 5;
  values[5] = -12;
  values[6] = 12;
  values[7] = -13;
  values[8] = 13;
  values[9] = -14;
  values[10] = 14;
  values[11] = -16;
  values[12] = 16;
  values[13] = -2;
  values[14] = 2;

  str += F("<select name='");
  str += name;
  str += "'>";
  for (int x = 0; x < 15; x++)
  {
    str += F("<option value='");
    str += values[x];
    str += "'";
    if (value == values[x])
      str += F(" selected");
    str += ">";
    str += options[x];
    str += F("</option>");
  }
  str += F("</select>");
}

void handle_config()
{
  char tmpString[64];

  String name = server.arg("name");
  String password = server.arg("password");
  String espip = server.arg("espip");
  String espgateway = server.arg("espgateway");
  String espsubnet = server.arg("espsubnet");
  String espdns = server.arg("espdns");

  if (name.length() != 0)
  {
    strncpy(Settings.Name, name.c_str(), sizeof(Settings.Name));
    if (password.length() != 0) {
      strncpy(Settings.Password, password.c_str(), sizeof(Settings.Password));
    }

    espip.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.IP);
    espgateway.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.Gateway);
    espsubnet.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.Subnet);
    espdns.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.DNS);

    // TODO Checkbox led
    int8_t cho0 = Settings.cho[0], cho1 = Settings.cho[1], cho2 = Settings.cho[2];
    Settings.cho[0]  =  server.arg("cho0").toInt();
    Settings.cho[1]  =  server.arg("cho1").toInt();
    Settings.cho[2]  =  server.arg("cho2").toInt();
    if (cho0 != Settings.cho[0] || cho1 != Settings.cho[1] || cho2 != Settings.cho[2]) {
      configureOutputs();
    }


    server.sendHeader("Location", String("http://") + server.client().localIP().toString() + String("/config?saved=") + String(SaveSettings() ? 1 : 0), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
  }

  String reply = "";
  addHeader(true, reply);

  if (server.arg("saved").toInt() == 1) {
    reply += F("<h3 style=\"color:#00cc00\">Configuration Saved!</h3>");
  } else if (server.hasArg("saved")) {
    reply += F("<h3 style=\"color:red\">Error saving to flash!</h3>");
  }


  reply += F("<form method='post'>");

  reply += F("<div class='grid'>");

  reply += F("<div class='full'><div class='black'>Main Settings</div></div>");

  reply += F("<div><label>Name</label></div>");
  reply += F("<div><input type='text' name='name' maxlength='31' required value='");
  Settings.Name[25] = 0;
  reply += Settings.Name;
  reply += F("'/></div>");

  reply += F("<div><label>Password</label></div>");
  reply += F("<div><input type='text' name='password' pattern='.{8,64}' value='");
  //  Settings.Password[63] = 0;
  //  reply += Settings.Password;
  reply += F("'/></div>");
  //  reply += F("<div class='full'>Enter passsword to change it.</div>");

  reply += F("<div class='full'><div class='black'>Optional Settings</div></div>");

  char str[20];
  reply += F("<div><label>ESP IP:</label></div><div><input type='text' name='espip' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.IP[0], Settings.IP[1], Settings.IP[2], Settings.IP[3]);
  reply += str;
  reply += F("'></div>");

  reply += F("<div><label>ESP GW:</label></div><div><input type='text' name='espgateway' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.Gateway[0], Settings.Gateway[1], Settings.Gateway[2], Settings.Gateway[3]);
  reply += str;
  reply += F("'></div>");

  reply += F("<div><label>ESP Subnet:</label></div><div><input type='text' name='espsubnet' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.Subnet[0], Settings.Subnet[1], Settings.Subnet[2], Settings.Subnet[3]);
  reply += str;
  reply += F("'></div>");

  reply += F("<div><label>ESP DNS:</label></div><div><input type='text' name='espdns' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.DNS[0], Settings.DNS[1], Settings.DNS[2], Settings.DNS[3]);
  reply += str;
  reply += F("'></div>");

  reply += F("<div class='full'><div class='black'>GPIO configuration</div></div>");
  reply += F("<div><label>Channel 1:</label></div><div>");
  addPinStateSelect(reply, "cho0", Settings.cho[0]);
  reply += F("</div>");

  reply += F("<div><label>Channel 2:</label></div><div>");
  addPinStateSelect(reply, "cho1", Settings.cho[1]);
  reply += F("</div>");

  reply += F("<div><label>Channel 3:</label></div><div>");
  addPinStateSelect(reply, "cho2", Settings.cho[2]);
  reply += F("</div>");

  reply += F("<div class='full'><div style=\"text-align:center;\"><button class=\"button\">Submit</button></div></div>");

  reply += F("</div>");

  reply += F("</form>");

  addFooter(reply);
  server.send(200, "text/html", reply);
}


void handle_enable() {
  int channel = server.arg("channel").toInt();
  Serial.printf("Enable channel %d\r\n", channel);
  setOutputStatus(channel, true);
  handle_status();
}
void handle_disable() {
  int channel = server.arg("channel").toInt();
  Serial.printf("Disable channel %d\r\n", channel);
  setOutputStatus(channel, false);
  handle_status();
}
void handle_status() {
  String message = "{";
  message += F("\"channel0\":");
  message += isOutputDisabled(0) ? F("null") : (getOutputStatus(0) ? F("true") : F("false"));
  message += F(",\"channel1\":");
  message += isOutputDisabled(1) ? F("null") : (getOutputStatus(1) ? F("true") : F("false"));
  message += F(",\"channel2\":");
  message += isOutputDisabled(2) ? F("null") : (getOutputStatus(2) ? F("true") : F("false"));
  message += F("}");
  server.send ( 200, "application/json", message);
}
void handle_scan() {
  String response = "{";
  int n = WiFi.scanNetworks();
  response = "{\"total\":";
  response += n;
  response += ",\"networks\":[";
  //sort networks
  int indices[n];
  for (int i = 0; i < n; i++) {
    indices[i] = i;
  }

  // RSSI SORT

  // old sort
  for (int i = 0; i < n; i++) {
    for (int j = i + 1; j < n; j++) {
      if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
        std::swap(indices[i], indices[j]);
      }
    }
  }

  //display networks in page
  for (int i = 0; i < n; i++) {
    if (indices[i] == -1) continue; // skip dups
    if (i) {
      response += ",";
    }
    response += "{\"ssid\":\"";
    response += WiFi.SSID(indices[i]);
    response += "\",\"quality\":";
    response += getRSSIasQuality(WiFi.RSSI(indices[i]));
    response += ",\"encrypted\":";
    response += (WiFi.encryptionType(indices[i]) != ENC_TYPE_NONE ? "true" : "false");
    response += "}";
    delay(0);
  }
  response += "]}";

  server.send ( 200, "application/json", response);

}
void addHeader(boolean showMenu, String& str)
{
  boolean cssfile = false;

  str += F("<!doctype html><html lang=en><head><meta charset=utf-8><meta name='viewport' content='width=device-width, initial-scale=1'><title>");
  str += Settings.Name;
  str += F("</title>");

  str += F("<style>");
  str += F("html{box-sizing:border-box}*,*:before,*:after{box-sizing:inherit}html,body,h1,h2,h3,h4,h5,h6,p,ol,ul,li,dl,dt,dd,blockquote,address{margin:0;padding:0}");
  str += F("body{padding:10px;font-family:sans-serif; font-size:12pt;}");
  str += F("h1 {font-size:16pt; color:black;}");
  str += F("h6 {font-size:10pt; color:black; text-align:center;}");
  str += F(".button-menu {background-color:#ffffff; color:blue; margin: 10px; text-decoration:none}");
  str += F(".button-link {padding:5px 15px; background-color:#0077dd; color:#fff; border:solid 1px #fff; text-decoration:none}");
  str += F(".button-menu:hover {background:#ddddff;}");
  str += F(".button-link:hover {background:#369;}");
  str += F(".full > div {padding:10px;}.black{ background-color:black; color:#ffffff;}");
  str += F("td {padding:7px;}");
  str += F("table {color:black;}");
  str += F(".div_l {float: left;}");
  str += F(".div_r {float: right; margin: 2px; padding: 1px 10px; border-radius: 7px; background-color:#080; color:white;}");
  str += F(".div_br {clear: both;}");

  str += F(".grid{margin-left:-10px;}.grid>div{padding: 5px 0 5px 10px;;margin:0;float:left;width: 100%}");
  str += F("@media (min-width:360px){.grid>div:not(.full){width: 50%}}");

  str += F(".container{margin:0 auto;max-width: 400px;}");
  str += F("label,input,select{width:100%;display:inline-block;height:30px;line-height:30px;}input,select{padding: 0 5px;}");
  str += F(".button{border: 0;border-radius: 0.3rem;background-color: #1fa3ec;color: #fff;line-height: 2.4rem;font-size: 1.2rem;width: 100%;}");
  str += F("@media (max-width:359px){label{height: auto;line-height:normal;}}");
  str += F(".menu{margin: 10px 0}.menu>a{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:30px;height:30px;font-size:1.2rem;padding:0 5px;text-decoration:none;display:inline-block;}.menu>a+a{margin-left:10px}");
  str += F(".q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}");
  str += F("</style>");

  str += F("<script><!--");
  str += F("function dept_onchange(frmselect) {frmselect.submit();}\n");
  str += F("//--></script>");

  str += F("</head><body><div class='container'>");

  str += F("<h1>ESPMote: ");
  str += Settings.Name;
  str += F("</h1>");

  if (showMenu)
  {
    str += F("<div class=\"menu\">");
    str += F("<a href=\"/\">Main</a>");
    str += F("<a href=\"config\">Config</a>");
    //    str += F("<a href=\"hardware\">Hardware</a>");
    str += F("<a href=\"wifi\">Wifi</a>");
    //    str += F("<a href=\"reset\">Factory reset</a>");
    str += F("</div>");
  }
}
void addFooter(String& str)
{
  str += F("<br /><h6>espmote.pnia.es</h6></div></body></html>");
}
