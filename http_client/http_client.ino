#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>

#include "secrets.h"  // defines STASSID and STAPSK

#define TIME_SERVER_URL "http://worldtimeapi.org/api/ip"
#define ASTROS_URL "http://api.open-notify.org/astros.json"
#define JOKES_URL "https://v2.jokeapi.dev/joke/Programming"

WiFiUDP ntp_udp;
// default 'pool.ntp.org' is used with 60 seconds update interval and no offset
NTPClient ntp_client(ntp_udp);

WiFiServer web_server(80);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while (!Serial)
    ;
  WiFi.mode(WIFI_STA);
  //  WiFi.setHostname("RasberryPicoW");
  Serial.printf("connecting to '%s' with '%s'\n", STASSID, STAPSK);
  WiFi.begin(STASSID, STAPSK);
  while (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("\nconnection to wifi failed");
      while (true) delay(10000);
    }
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\nconnected to wifi\nip: %s\n", WiFi.localIP().toString().c_str());
  digitalWrite(LED_BUILTIN, HIGH);
}

void setup1() {
  web_server.begin();
}

bool read_url_to_json_doc(const char* url, DynamicJsonDocument& json_doc) {
  HTTPClient http_client;
  http_client.useHTTP10(true);
  if (!strncmp(url, "https://", 8)) {
    http_client.setInsecure();
  }
  if (!http_client.begin(url)) {
    Serial.printf("unable to connect to %s\n", url);
    return false;
  }
  const auto http_code = http_client.GET();
  if (http_code != HTTP_CODE_OK) {
    Serial.printf("get error: %d: %s\n", http_code, http_client.errorToString(http_code).c_str());
    http_client.end();
    return false;
  }
  const auto json_error = deserializeJson(json_doc, http_client.getStream());
  http_client.end();
  if (json_error) {
    Serial.printf("json parsing failed: %s\n", json_error.c_str());
    return false;
  }
  return true;
}

void print_astronauts_in_space_right_now() {
  digitalWrite(LED_BUILTIN, LOW);
  DynamicJsonDocument json_doc(8 * 1024);
  if (!read_url_to_json_doc(ASTROS_URL, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  const auto people = json_doc["people"].as<JsonArray>();
  for (const auto p : people) {
    Serial.println(p["name"].as<const char*>());
  }
  //const unsigned n = json_doc["number"].as<unsigned>();
  //for (unsigned i = 0; i < n; i++) {
  //  Serial.println(json_people[i]["name"].as<const char*>());
  //}
}

void print_current_time_based_on_ip() {
  digitalWrite(LED_BUILTIN, LOW);
  DynamicJsonDocument json_doc(1024);
  if (!read_url_to_json_doc(TIME_SERVER_URL, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  const auto date_time_raw = json_doc["datetime"].as<String>();
  //  "2023-08-31T16:32:47.653086+02:00" to "2023-08-31 16:32:47"
  const auto date_time = date_time_raw.substring(0, 10) + " " + date_time_raw.substring(11, 19);
  Serial.println(date_time);
}

void print_random_programming_joke() {
  digitalWrite(LED_BUILTIN, LOW);
  DynamicJsonDocument json_doc(4 * 1024);
  if (!read_url_to_json_doc(JOKES_URL, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  if (json_doc["type"].as<String>() == "single") {
    Serial.println(json_doc["joke"].as<const char*>());
  } else {
    Serial.println(json_doc["setup"].as<const char*>());
    Serial.println(json_doc["delivery"].as<const char*>());
  }
}

void print_current_time_from_ntp() {
  digitalWrite(LED_BUILTIN, LOW);
  ntp_client.update();
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println(ntp_client.getFormattedTime());
}

bool handle_web_server() {
  WiFiClient client = web_server.available();
  if (!client)
    return false;

  // read first request line
  const String method = client.readStringUntil(' ');
  const String uri = client.readStringUntil(' ');
  const String version = client.readStringUntil('\r');
  if (client.read() != '\n') {
    Serial.println("*** malformed http request");
    return false;
  }
  //Serial.printf("%s\n\n", uri.c_str());
  String req;
  req.reserve(1024);
  req.concat(method);
  req.concat(" ");
  req.concat(uri);
  req.concat(" ");
  req.concat(version);
  req.concat("\r\n");

  // read the rest of the request
  while (true) {
    const String line = client.readStringUntil('\r');
    if (client.read() != '\n') {
      Serial.println("*** malformed http request");
      return false;
    }
    //Serial.printf("%d: %s\r\n", line.length(), line.c_str());
    if (line.length() == 0)
      break;
    req.concat(line);
    req.concat("\r\n");
  }
  Serial.println("\nwebserver request:\n-------------------------------------------------");
  Serial.print(req);
  Serial.println("-------------------------------------------------");
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println();
  client.println(req);
  client.stop();
  return true;
}

void print_web_server_ip() {
  Serial.println(WiFi.localIP().toString().c_str());
}

void loop() {
  Serial.println("\ncurrent time based on ip:");
  print_current_time_based_on_ip();

  Serial.println("\ncurrent time in utc from ntp:");
  print_current_time_from_ntp();

  Serial.println("\nastronauts in space right now:");
  print_astronauts_in_space_right_now();

  Serial.println("\nprogramming joke:");
  print_random_programming_joke();

  Serial.println("\nweb server ip:");
  print_web_server_ip();

  delay(10000);
}

void loop1() {
  if (!handle_web_server())
    delay(1000);  // slightly less busy wait
}