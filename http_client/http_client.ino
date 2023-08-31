#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "secrets.h"  // defines STASSID and STAPSK

#define TIME_SERVER_URL "http://worldtimeapi.org/api/ip"
#define ASTROS_URL "http://api.open-notify.org/astros.json"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while (!Serial)
    ;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("RasberryPicoW");
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

void print_astronauts_in_space_right_now() {
  digitalWrite(LED_BUILTIN, LOW);
  
  HTTPClient http;
  if (!http.begin(ASTROS_URL)) {
    Serial.printf("unable to connect to %s\n", ASTROS_URL);
    return;
  }
  const auto http_code = http.GET();
  if (http_code != HTTP_CODE_OK) {
    Serial.printf("GET failed, error: %s\n", http.errorToString(http_code).c_str());
    return;
  }

  const auto json_str = http.getString();

  http.end();
  
  digitalWrite(LED_BUILTIN, HIGH);
  
  DynamicJsonDocument json_doc(8 * 1024);
  const auto json_error = deserializeJson(json_doc, json_str);
  if (json_error) {
    Serial.printf("json parsing failed: %s\n", json_error.c_str());
    return;
  }

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

  HTTPClient http_client;
  if (!http_client.begin(TIME_SERVER_URL)) {
    Serial.printf("unable to connect to %s\n", TIME_SERVER_URL);
    return;
  }
  const auto http_code = http_client.GET();
  if (http_code != HTTP_CODE_OK) {
    Serial.printf("GET failed, error: %s\n", http_client.errorToString(http_code).c_str());
    return;
  }

  const auto json_str = http_client.getString();

  http_client.end();

  digitalWrite(LED_BUILTIN, HIGH);

  DynamicJsonDocument json_doc(1024);
  const auto json_error = deserializeJson(json_doc, json_str);
  if (json_error) {
    Serial.printf("json parsing failed: %s\n", json_error.c_str());
    return;
  }
  //  Serial.println(jsonstr);
  const auto date_time = json_doc["datetime"].as<String>();
  //  Serial.println(date_time);

  //  "2023-08-31T16:32:47.653086+02:00" to "2023-08-31 16:32:47"
  const auto date_time_fmt = date_time.substring(0, 10) + " " + date_time.substring(11, 19);
  Serial.println(date_time_fmt);
}

void loop() {
  Serial.println("\ncurrent date time based on ip:");
  print_current_time_based_on_ip();

  Serial.println("\nastronauts in space right now:");
  print_astronauts_in_space_right_now();

  delay(10000);
}
