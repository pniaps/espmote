
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
    wifiSetupConnect = true;
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

  server.on("/status", []() {
    String message = "";
    if (digitalRead(LED_PIN) == LOW) {
      message = "{\"enabled\":true}";
    } else {
      message = "{\"enabled\":false}";
    }
    server.send ( 200, "application/json", message);
  });
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

    sprintf(body, "{\"Version\":\"%d\",\"id\":\"%06X\",\"Flash real id\":\"%08X\",\"Flash real size\":\"%u\",\"Flash ide size\":\"%u\",\"Flash size\":\"%s\",\"Sketch Size\":\"%u\",\"Free Sketch Space\":\"%u\"}",
            VERSION,
            ESP.getChipId(),
            ESP.getFlashChipId(),
            realSize,
            ideSize,
            (ideSize != realSize ? "Flash Chip configuration wrong!" : "Flash Chip configuration ok"),
            ESP.getSketchSize(),
            ESP.getFreeSketchSpace()
           );
    server.send ( 200, "application/json", body);
  });


  server.onNotFound ( handleNotFound );

  server.begin();
  dbg_printf ("[SERVER] Ready\n");
}

