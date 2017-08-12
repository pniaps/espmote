
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

  server.send ( 404, "text/plain", message );

  ResetFactory();
}

void handleWifi(boolean scan) {

  String page = FPSTR(HTTP_HEAD);
  page.replace("{v}", "Config ESP");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  //  page += _customHeadElement;
  page += FPSTR(HTTP_HEAD_END);

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

  page += FPSTR(HTTP_FORM_START);
  char parLength[2];
  // add the extra parameters to the form
  /*for (int i = 0; i < _paramsCount; i++) {
    if (_params[i] == NULL) {
      break;
    }

    String pitem = FPSTR(HTTP_FORM_PARAM);
    if (_params[i]->getID() != NULL) {
      pitem.replace("{i}", _params[i]->getID());
      pitem.replace("{n}", _params[i]->getID());
      pitem.replace("{p}", _params[i]->getPlaceholder());
      snprintf(parLength, 2, "%d", _params[i]->getValueLength());
      pitem.replace("{l}", parLength);
      pitem.replace("{v}", _params[i]->getValue());
      pitem.replace("{c}", _params[i]->getCustomHTML());
    } else {
      pitem = _params[i]->getCustomHTML();
    }

    page += pitem;
    }
    if (_params[0] != NULL) {
    page += "<br/>";
    }*/

  /*if (_sta_static_ip) {

    String item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "ip");
    item.replace("{n}", "ip");
    item.replace("{p}", "Static IP");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_ip.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "gw");
    item.replace("{n}", "gw");
    item.replace("{p}", "Static Gateway");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_gw.toString());

    page += item;

    item = FPSTR(HTTP_FORM_PARAM);
    item.replace("{i}", "sn");
    item.replace("{n}", "sn");
    item.replace("{p}", "Subnet");
    item.replace("{l}", "15");
    item.replace("{v}", _sta_static_sn.toString());

    page += item;

    page += "<br/>";
    }*/

  page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_SCAN_LINK);

  page += FPSTR(HTTP_END);

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

  //parameters
  /*for (int i = 0; i < _paramsCount; i++) {
    if (_params[i] == NULL) {
      break;
    }
    //read parameter
    String value = server->arg(_params[i]->getID()).c_str();
    //store it in array
    value.toCharArray(_params[i]->_value, _params[i]->_length);
    Serial.println(F("Parameter"));
    Serial.println(_params[i]->getID());
    Serial.println(value);
    }*/

  /*if (server->arg("ip") != "") {
    Serial.println(F("static ip"));
    Serial.println(server->arg("ip"));
    //_sta_static_ip.fromString(server->arg("ip"));
    String ip = server->arg("ip");
    optionalIPFromString(&_sta_static_ip, ip.c_str());
    }
    if (server->arg("gw") != "") {
    Serial.println(F("static gateway"));
    Serial.println(server->arg("gw"));
    String gw = server->arg("gw");
    optionalIPFromString(&_sta_static_gw, gw.c_str());
    }
    if (server->arg("sn") != "") {
    Serial.println(F("static netmask"));
    Serial.println(server->arg("sn"));
    String sn = server->arg("sn");
    optionalIPFromString(&_sta_static_sn, sn.c_str());
    }*/

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
  server.on("/enable", []() {
    digitalWrite(LED_PIN, LOW);
    server.send ( 200, "application/json", "{\"enabled\":true}");
  });

  server.on("/disable", []() {
    digitalWrite(LED_PIN, HIGH);
    server.send ( 200, "application/json", "{\"enabled\":false}");
  });

  server.on("/status", handle_status);
  server.on("/", []() {
    String message = "";
    if (digitalRead(LED_PIN) == LOW) {
      message = "{\"enabled\":true}";
    } else {
      message = "{\"enabled\":false}";
    }
    server.send ( 200, "application/json", message);
  });

  server.on("/wifi", std::bind(&handleWifi, true));
  server.on("/0wifi", std::bind(&handleWifi, false));
  server.on("/wifisave", std::bind(&handleWifiSave));

  server.on("/scan", []() {
    String response = "{";
    int n = WiFi.scanNetworks();
    response = "{\"total\":";
    response += n;
    response += ",\"networks\":[";
    if (n == 0) {
      //      Serial.println(F("No networks found"));
      //      page += F("No networks found. Refresh to scan again.");
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
  });

  server.on("/reset", handleReset);

  server.on("/info", []() {
    char body[1024] = {0};

    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();
    FlashMode_t ideMode = ESP.getFlashChipMode();
    IPAddress ip = server.client().localIP();

    sprintf(body, "{\"Version\":\"%d\",\"id\":\"%06X\",\"Flash id\":\"%08X\",\"Flash real size\":\"%u\",\"Flash ide size\":\"%u\",\"Flash size\":\"%s\",\"Sketch Size\":\"%u\",\"Free Sketch Space\":\"%u\",\"Client IP\":\"%u.%u.%u.%u\"}",
            VERSION,
            ESP.getChipId(),
            ESP.getFlashChipId(),
            realSize,
            ideSize,
            (ideSize != realSize ? "Flash Chip configuration wrong!" : "Flash Chip configuration ok"),
            ESP.getSketchSize(),
            ESP.getFreeSketchSpace(),
            ip[0], ip[1], ip[2], ip[3]
           );
    server.send ( 200, "application/json", body);
  });


  server.onNotFound ( handleNotFound );

  server.on("/config", handle_config);
  //  server.on("/hardware", handle_hardware);

  server.begin();
  dbg_printf ("[SERVER] Ready\n");
}

void addPinStateSelect(String& str, String name,  int value)
{
  String options[13];
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
  int values[13];
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

  str += F("<select name='");
  str += name;
  str += "'>";
  for (int x = 0; x < 13; x++)
  {
    str += F("<option value='");
    str += values[x];
    str += "'";
    if (value == x)
      str += F(" selected");
    str += ">";
    str += options[x];
    str += F("</option>");
  }
  str += F("</select>");
}

void handle_config() {
  char tmpString[64];

  String name = server.arg("name");
  String password = server.arg("password");
  String ssid = server.arg("ssid");
  String key = server.arg("key");
  String espip = server.arg("espip");
  String espgateway = server.arg("espgateway");
  String espsubnet = server.arg("espsubnet");
  String espdns = server.arg("espdns");

  if (ssid[0] != 0)
  {
    strncpy(Settings.Name, name.c_str(), sizeof(Settings.Name));
    strncpy(Settings.Password, password.c_str(), sizeof(Settings.Password));
    strncpy(Settings.WifiSSID, ssid.c_str(), sizeof(Settings.WifiSSID));
    strncpy(Settings.WifiKey, key.c_str(), sizeof(Settings.WifiKey));

    espip.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.IP);
    espgateway.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.Gateway);
    espsubnet.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.Subnet);
    espdns.toCharArray(tmpString, 26);
    str2ip(tmpString, Settings.DNS);
    SaveSettings();
  }

  String reply = "";
  addHeader(true, reply);

  reply += F("<form method='post'>");

  reply += F("<div class='grid'>");

  reply += F("<div class='full black'>Main Settings</div>");

  reply += F("<div><label>Name</label></div>");
  reply += F("<div><input type='text' name='name' maxlength='31' required value='");
  Settings.Name[25] = 0;
  reply += Settings.Name;
  reply += F("'></div>");

  reply += F("<div><label>Password</label></div>");
  reply += F("<div><input type='text' name='password' pattern='.{8,63}' required value='");
  //  Settings.Password[63] = 0;
  //  reply += Settings.Password;
  reply += F("'></div>");

  reply += F("<div class='full black'>Optional Settings</div>");

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

  reply += F("<div class='full black'>GPIO configuration</div>");
  reply += F("<div><label>Channel 1:</label></div><div>");
  addPinStateSelect(reply, "chi0", Settings.cho[0]);
  reply += F("</div>");

  reply += F("<div><label>Channel 2:</label></div><div>");
  addPinStateSelect(reply, "chi1", Settings.cho[1]);
  reply += F("</div>");

  reply += F("<div><label>Channel 3:</label></div><div>");
  addPinStateSelect(reply, "chi2", Settings.cho[2]);
  reply += F("</div>");

  reply += F("<div class='full' style=\"text-align:center;\"><button class=\"button\">Submit</button></div>");

  reply += F("</div>");

  reply += F("</form>");

/*
  reply += F("<form name='frmselect' method='post'><table style=\"border-collapse: collapse;border-spacing: 0;\">");
  reply += F("<tr><th>Main Settings<th><tr><td>Name:<td><input type='text' name='name' maxlength='31' required value='");
  Settings.Name[25] = 0;
  reply += Settings.Name;
  reply += F("'><tr><td>Password:<td><input type='text' name='password' pattern='.{8,63}' required value='");
  //  SecuritySettings.Password[25] = 0;
  //  reply += SecuritySettings.Password;
  //  reply += F("'><tr><td>SSID:<td><input type='text' name='ssid' value='");
  //  reply += Settings.WifiSSID;
  //  reply += F("'><tr><td>WPA Key:<td><input type='password' maxlength='63' name='key' value='");
  //  reply += Settings.WifiKey;

  reply += F("'>");



  reply += F("<tr><th>Optional Settings<th>");

  reply += F("<tr><td>ESP IP:<td><input type='text' name='espip' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.IP[0], Settings.IP[1], Settings.IP[2], Settings.IP[3]);
  reply += str;

  reply += F("'><tr><td>ESP GW:<td><input type='text' name='espgateway' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.Gateway[0], Settings.Gateway[1], Settings.Gateway[2], Settings.Gateway[3]);
  reply += str;

  reply += F("'><tr><td>ESP Subnet:<td><input type='text' name='espsubnet' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.Subnet[0], Settings.Subnet[1], Settings.Subnet[2], Settings.Subnet[3]);
  reply += str;

  reply += F("'><tr><td>ESP DNS:<td><input type='text' name='espdns' value='");
  sprintf_P(str, PSTR("%u.%u.%u.%u"), Settings.DNS[0], Settings.DNS[1], Settings.DNS[2], Settings.DNS[3]);
  reply += str;
  reply += F("'>");

  reply += F("<tr><th>GPIO configuration<th>");
  reply += F("<tr><td>Channel 1:<td>");
  addPinStateSelect(reply, "chi0", Settings.cho[0]);
  reply += F("<tr><td>Channel 2:<td>");
  addPinStateSelect(reply, "chi1", Settings.cho[1]);
  reply += F("<tr><td>Channel 3:<td>");
  addPinStateSelect(reply, "chi2", Settings.cho[2]);

  reply += F("<tr><TD colspan=\"2\" style=\"text-align:center;\"><input class=\"button-link\" type='submit' value='Submit'>");

  reply += F("</table></form>");
  */
  addFooter(reply);
  server.send(200, "text/html", reply);
}

//void handle_hardware() {
//  String reply = "";
//  addHeader(true, reply);
//
//  reply += F("<form  method='post'><table><th>Hardware Settings<th><tr><td>");
//
//
//
//  reply += F("<tr><td>GPIO boot states:<td>");
//  reply += F("<tr><td>Channel 1:<td>");
//  addPinStateSelect(reply, "chi0", Settings.cho[0]);
//  reply += F("<tr><td>Channel 2:<td>");
//  addPinStateSelect(reply, "chi1", Settings.cho[1]);
//  reply += F("<tr><td>Channel 3:<td>");
//  addPinStateSelect(reply, "chi2", Settings.cho[2]);
//
//  reply += F("<tr><td><td><input class=\"button-link\" type='submit' value='Submit'><tr><td>");
//
//  reply += F("</table></form>");
//  addFooter(reply);
//  server.send(200, "text/html", reply);
//}

void handle_status() {
  String message = "";
  if (digitalRead(LED_PIN) == LOW) {
    message = "{\"enabled\":true}";
  } else {
    message = "{\"enabled\":false}";
  }
  server.send ( 200, "application/json", message);
}

void addHeader(boolean showMenu, String& str)
{
  boolean cssfile = false;

  str += F("<!doctype html><html lang=en><head><meta charset=utf-8><meta name='viewport' content='width=device-width, initial-scale=1'><title>");
  str += Settings.Name;
  str += F("</title>");

  str += F("<style>");
  str += F("html{box-sizing:border-box}*,*:before,*:after{box-sizing:inherit}html,body,h1,h2,h3,h4,h5,h6,p,ol,ul,li,dl,dt,dd,blockquote,address{margin:0;padding:0}");
  str += F("body{padding:20px;font-family:sans-serif; font-size:12pt;}");
  str += F("h1 {font-size:16pt; color:black;}");
  str += F("h6 {font-size:10pt; color:black; text-align:center;}");
  str += F(".button-menu {background-color:#ffffff; color:blue; margin: 10px; text-decoration:none}");
  str += F(".button-link {padding:5px 15px; background-color:#0077dd; color:#fff; border:solid 1px #fff; text-decoration:none}");
  str += F(".button-menu:hover {background:#ddddff;}");
  str += F(".button-link:hover {background:#369;}");
  str += F("th,.full {padding:10px;}.black{ background-color:black; color:#ffffff;}");
  str += F("td {padding:7px;}");
  str += F("table {color:black;}");
  str += F(".div_l {float: left;}");
  str += F(".div_r {float: right; margin: 2px; padding: 1px 10px; border-radius: 7px; background-color:#080; color:white;}");
  str += F(".div_br {clear: both;}");

  str += F(".grid{margin-left:-10px;}.grid>div{padding-left:10px;margin:0;float:left;width: 100%}");
  str += F("@media (min-width:360px){.grid>div:not(.full){width: 50%}}");

  str += F(".container{margin:0 auto;max-width: 400px;}");
  str += F("label,input,select{width:100%;display:inline-block;height:30px;line-height:30px;margin:5px 0;}input,select{padding: 0 5px;}");
  str += F(".button{border: 0;border-radius: 0.3rem;background-color: #1fa3ec;color: #fff;line-height: 2.4rem;font-size: 1.2rem;width: 100%;}");
  str += F("@media (max-width:359px){label{height: auto;line-height:normal;}}");
  str += F(".menu{margin; 0 10px}.menu>a{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:30px;height:30px;font-size:1.2rem;padding:0 5px;text-decoration:none;display:inline-block;}.menu>a+a{margin-left:10px}");
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
    str += F("<a href=\"reset\">Factory reset</a>");
    str += F("</div>");
  }
}
void addFooter(String& str)
{
  str += F("<h6>espmote.pnia.es</h6></div></body></html>");
}
