#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

enum state {
  IDLE = 0, 
  CONNECTED
};

int currentState = IDLE;
String command = "";
String ADA_username = "";
String AIO_key = "";
boolean isSetupADA = false;
String getValue = "";
int count = 0;
int isStartOrEnd = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  //connectWifi("C0703", "19871988");
  //setupADA("quang3103", "aio_lKCl30rLPNMsXP4HacHgbKr3Kl21");
}

void readCommand() {
  
  while (Serial.available()) {
    char c = char(Serial.read());
    
    if (c == '!') {
      isStartOrEnd = 1;
      command += c;
    } else if (c == '#') {
      if (isStartOrEnd == 1) command += c;
      isStartOrEnd = 0;
    } else if (isStartOrEnd == 1) command += c;
  }
  //Serial.println("");
}

void returnResponseBody(String body) {
  if (body == "") body = "EMPTY";
  Serial.print(body + '#');
  delay(100);
  //Serial.println("");
}

void returnWifiRessult(boolean isConnected) {
  if (isConnected) {
    Serial.print("SUCC#");
    //Serial.println("");
  } else {
    Serial.print("FAIL#");
    //Serial.println("");
  }
}

boolean connectWifi(String account, String password) {
  WiFi.begin(account, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(100);
    //Serial.print('.');
    if (WiFi.status() == WL_CONNECT_FAILED) return false;
  }
  return true;
}

String createPOSTBodyADA(String value, String API_key) {
  
  return ("value=" + value + "&X-AIO-Key=" + API_key);
}

String createServerNameADA(String user, String feed, boolean isPost) {
  //"http://io.adafruit.com/api/v2/quang3103/feeds/led/data";
  if (isPost) return ("http://io.adafruit.com/api/v2/" + user + "/feeds/" + feed + "/data");
  else return ("http://io.adafruit.com/api/v2/" + user + "/feeds/" + feed);
}

void httpGetThingSpeak(String serverName) {
  
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName.c_str());
  
  boolean isRequestSuccessful = false;
  int httpResponseCode = 404;
  
  for (int i = 0; i < 3; i++) {
    // Send HTTP GET request
    httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
      isRequestSuccessful = true;
      String payload = http.getString();
      returnResponseBody(payload + "SEND OK");
      break;
    } 
  }
  if (!isRequestSuccessful) returnResponseBody(String(httpResponseCode) + "FAIL");

  http.end(); //
}

void httpPost(String serverName, String body) {
  
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName.c_str());

  // Specify content-type header
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  // Data to send with HTTP POST
  String httpRequestData = body;

  boolean isRequestSuccessful = false;
  int httpResponseCode = 404;
  
  for (int i = 0; i < 3; i++) {
    // Send HTTP POST request
    httpResponseCode = http.POST(httpRequestData);
  
    if (httpResponseCode == 200) {
      isRequestSuccessful = true;
      returnResponseBody("SEND OK");
      break;
    }
  }
  if (!isRequestSuccessful) returnResponseBody(String(httpResponseCode) + "FAIL");

  http.end();
}

void httpGetADA(String serverName) {
  
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverName.c_str());
  
  boolean isRequestSuccessful = false;
  int httpResponseCode = 404;
  
  for (int i = 0; i < 3; i++) {
    // Send HTTP GET request
    httpResponseCode = http.GET();
    
    if (httpResponseCode == 200) {
      isRequestSuccessful = true;
      String payload = http.getString();
      int startPos = payload.indexOf("last_value");
      int endPos = payload.indexOf(',', startPos);
      String result = "EMPTY";
      if ((startPos > 0) && (startPos + 13 < endPos - 1)) result = payload.substring(startPos + 13, endPos - 1);
      returnResponseBody(result);
      break;
    }
  }
  if (!isRequestSuccessful) returnResponseBody(String(httpResponseCode) + "FAIL");
  http.end();
  
}

void setupADA(String username, String key) {
  isSetupADA = true;
  ADA_username = username;
  AIO_key = key;
  //returnResponseBody("SEND OK");
}

void processCommand() {
  String requestOrAccount = "", data = "";
  int startPos = command.indexOf('!');
  int sep = command.indexOf(':');
  int endPos = command.indexOf('#');
  
  if (WiFi.status() == WL_CONNECTION_LOST) {
    currentState = IDLE;
    returnWifiRessult(false);
    return;
  }
  
  while ((startPos < sep) && (sep < endPos)) {
    requestOrAccount = command.substring(startPos + 1, sep);
    data = command.substring(sep + 1, endPos);
    if (endPos == command.length() - 1) {
      command = "";
    }
    else {
      command = command.substring(endPos + 1);
    }
    if (requestOrAccount == "GET") {
      if (currentState = IDLE) {
        returnWifiRessult(false);
        return;
      }
      httpGetThingSpeak(data);
    } 
    else if (requestOrAccount == "POST_ADA") {
      if (currentState = IDLE) {
        returnWifiRessult(false);
        return;
      }
      if (!isSetupADA) {
        returnResponseBody("ADA");
        return;
      }
      int pos = data.indexOf(':');

      String serverName = createServerNameADA(ADA_username, data.substring(0, pos), true);
      String body = createPOSTBodyADA(data.substring(pos + 1), AIO_key);
      httpPost(serverName, body);
    } 
    else if (requestOrAccount == "GET_ADA") {
      if (currentState = IDLE) {
        returnWifiRessult(false);
        return;
      }
      if (!isSetupADA) {
        returnResponseBody("ADA");
        return;
      }

      String serverName = createServerNameADA(ADA_username, data, false);
      httpGetADA(serverName);
      
    } 
    else if (requestOrAccount == "ADA") {
      int pos = data.indexOf(':');
      setupADA(data.substring(0, pos), data.substring(pos + 1));
    }
    else {
      returnWifiRessult(connectWifi(requestOrAccount, data)); 
      currentState = CONNECTED;
    }
    if (command == "") break;
    startPos = command.indexOf('!');
    sep = command.indexOf(':');
    endPos = command.indexOf('#'); 
    if (WiFi.status() == WL_CONNECTION_LOST) {
      currentState = IDLE;
      returnWifiRessult(false);
      return;
    }
  }
}

void process() {
  if (Serial.available()) readCommand();
  if (command != "") {
    //Serial.println("Actual command: " + command);
    //command = "";
    processCommand();
  }
}

void loop() {
  process();
  delay(50);
//  Serial.println(String(++count) + " time:");
//  int value = random(25, 35);
//  Serial.print("POST to ADA: " + String(value) + " ==> ");
//  httpPost(createServerNameADA("quang3103", "led", true), createPOSTBodyADA(String(value), AIO_key));
//  Serial.print("Get from ADA: ");
//  httpGetADA(createServerNameADA("quang3103", "led", false));
//  Serial.print("Post to THINGSPEAK: " + getValue);
//  Serial.print(" | Current number of points on chart: ");
//  httpGetThingSpeak("http://api.thingspeak.com/update?api_key=0324U6WNIBX28W4G&field1=" + getValue);
//  delay(20000);
}
