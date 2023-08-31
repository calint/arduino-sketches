#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "secrets.h"  // defines STASSID and STAPSK

#define TIME_SERVER_URL "http://worldtimeapi.org/api/ip"
#define ASTROS_URL "http://api.open-notify.org/astros.json"

void setup() {
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
}

void print_astronauts_in_space_right_now() {
  HTTPClient http;
  if (!http.begin(ASTROS_URL)) {
    Serial.printf("unable to connect to %s\n", ASTROS_URL);
    return;
  }
  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return;
  }
  const String json_str = http.getString();

  http.end();

  DynamicJsonDocument json_doc(8 * 1024);
  const DeserializationError error = deserializeJson(json_doc, json_str);
  if (error) {
    Serial.printf("json parsing failed: %s\n", error.c_str());
    return;
  }

  auto json_people = json_doc["people"];
  const unsigned n = json_doc["number"].as<unsigned>();
  for (unsigned i = 0; i < n; i++) {
    Serial.printf("%s\n", json_people[i]["name"].as<const char*>());
  }
}

void print_current_time_based_on_ip() {
  HTTPClient http;
  if (!http.begin(TIME_SERVER_URL)) {
    Serial.printf("unable to connect to %s\n", TIME_SERVER_URL);
    return;
  }
  const int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("GET failed, error: %s\n", http.errorToString(httpCode).c_str());
    http.end();
    return;
  }

  const String json_str = http.getString();

  http.end();

  DynamicJsonDocument json_doc(1024);
  const DeserializationError error = deserializeJson(json_doc, json_str);
  if (error) {
    Serial.printf("json parsing failed: %s\n", error.c_str());
    return;
  }
  //  Serial.println(jsonstr);
  String date_time = json_doc["datetime"];
  //  Serial.println(date_time);

  //  "2023-08-31T16:32:47.653086+02:00" to "2023-08-31 16:32:47"
  String date_time_fmt = date_time.substring(0, 10) + " " + date_time.substring(11, 19);
  Serial.println(date_time_fmt);
}

void loop() {
  Serial.println("\ncurrent date time based on ip:");
  print_current_time_based_on_ip();

  Serial.println("\nastronauts in space right now:");
  print_astronauts_in_space_right_now();

  delay(10000);
}
