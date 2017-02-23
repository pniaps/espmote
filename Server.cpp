/**
   http://stackoverflow.com/a/28921280/2115411
*/
//function urldecode(s)
//  s = s:gsub('+', ' ')
//       :gsub('%%(%x%x)', function(h)
//                           return string.char(tonumber(h, 16))
//                         end)
//  return s
//end
//
//function parseurl(s)
//  s = s:match('%s+(.+)')
//  local ans = {}
//  for k,v in s:gmatch('([^&=?]-)=([^&=?]+)' ) do
//    ans[ k ] = urldecode(v)
//  end
//  return ans
//end

#include "Server.h"
#define LED_PIN 2

void checkPetition()
{
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while (!client.available()) {
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  String response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
  // Match the request
  if (req.indexOf("/enable") != -1) {
    digitalWrite(LED_PIN, LOW);
    response += "{\"enabled\":true}";
  } else if (req.indexOf("/disable") != -1) {
    digitalWrite(LED_PIN, HIGH);
    response += "{\"enabled\":false}";
  }else{
    response += "{\"error\":\"invalid request\"}";
  }

  client.flush();

  // Send the response to the client
  client.print(response);
  client.stop();
  delay(1);
  Serial.println("Client disonnected");
}

WiFiServer server(80);
