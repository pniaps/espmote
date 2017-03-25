bool sendping_flag = false;

void sendPingFlag() {
  sendping_flag = true;

}
void sendPing()
{
  if (sendping_flag) {
//    Serial.println(F("Enviando ping ..."));

    char body[255] = {0};
    sprintf(body, "{\"version\":\"%d\",\"id\":\"%06X\",\"build\":\"%s\"}", VERSION, ESP.getChipId(), __DATE__);
    Serial.println(body);

    http.begin("http://iot.pnia.es/api/ping");
//    http.begin("http://192.168.1.3/iot.pnia.es/public/api/ping?XDEBUG_SESSION_START=1");
    http.setTimeout(5000);
    http.addHeader("Content-Type","application/json");
    int httpCode = http.PUT(body);
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
    Serial.println(http.getString());
    http.end();
//    Serial.println(F("Ping enviado"));
    sendping_flag = false;
  }
}
