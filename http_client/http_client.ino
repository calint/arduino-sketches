#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <NTPClient.h>

#include <vector>

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

// returns true if request succeeded or false if something went wrong
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

void print_astronauts_in_space_right_now(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  DynamicJsonDocument json_doc(8 * 1024);
  if (!read_url_to_json_doc(ASTROS_URL, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  const auto people = json_doc["people"].as<JsonArray>();
  for (const auto p : people) {
    os.println(p["name"].as<const char*>());
  }
  //const unsigned n = json_doc["number"].as<unsigned>();
  //for (unsigned i = 0; i < n; i++) {
  //  Serial.println(json_people[i]["name"].as<const char*>());
  //}
}

void print_current_time_based_on_ip(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  DynamicJsonDocument json_doc(1024);
  if (!read_url_to_json_doc(TIME_SERVER_URL, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  const auto date_time_raw = json_doc["datetime"].as<String>();
  //  "2023-08-31T16:32:47.653086+02:00" to "2023-08-31 16:32:47"
  const auto date_time = date_time_raw.substring(0, 10) + " " + date_time_raw.substring(11, 19);
  os.println(date_time);
}

void print_random_programming_joke(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  DynamicJsonDocument json_doc(4 * 1024);
  if (!read_url_to_json_doc(JOKES_URL, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  if (json_doc["type"].as<String>() == "single") {
    os.println(json_doc["joke"].as<const char*>());
  } else {
    os.println(json_doc["setup"].as<const char*>());
    os.println(json_doc["delivery"].as<const char*>());
  }
}

void print_current_time_from_ntp(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  ntp_client.update();
  digitalWrite(LED_BUILTIN, HIGH);
  os.println(ntp_client.getFormattedTime());
}

void print_web_server_ip(Stream& os) {
  os.println(WiFi.localIP().toString().c_str());
}

void print_output_to_stream(Stream& os) {
  os.println("\ncurrent time based on ip:");
  print_current_time_based_on_ip(os);

  os.println("\ncurrent time in utc from ntp:");
  print_current_time_from_ntp(os);

  os.println("\nastronauts in space right now:");
  print_astronauts_in_space_right_now(os);

  os.println("\nprogramming joke:");
  print_random_programming_joke(os);

  os.println("\nweb server ip:");
  print_web_server_ip(os);
}

void handle_web_server_root(const String& query, const std::vector<String>& headers, Stream& os) {
  os.print("<pre>query: ");
  os.println(query);
  for (const auto& s : headers)
    os.println(s);
}

void handle_web_server_status(const String& query, const std::vector<String>& headers, Stream& os) {
  os.print("<pre>query: ");
  os.println(query);
  for (const auto& s : headers)
    os.println(s);
  os.println();
  print_output_to_stream(os);
}

// returns true if a request was serviced or false if no client available
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

  const int query_start_ix = uri.indexOf("?");
  const String query = query_start_ix == -1 ? "" : uri.substring(query_start_ix + 1);
  const String path = query_start_ix == -1 ? uri : uri.substring(0, query_start_ix);

  std::vector<String> headers;
  headers.reserve(1024);
  while (true) {
    const String line = client.readStringUntil('\r');
    if (client.read() != '\n') {
      Serial.println("*** malformed http request");
      return false;
    }
    //Serial.printf("%d: %s\r\n", line.length(), line.c_str());
    if (line.length() == 0)
      break;
    headers.push_back(line);
  }

  Serial.println("\nwebserver request: ");
  Serial.println("-------------------------------------------------");
  Serial.println(path);
  Serial.println(query);
  for (const auto& s : headers)
    Serial.println(s);
  Serial.println("-------------------------------------------------");

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  if (path == "/") {
    handle_web_server_root(query, headers, client);
  } else if (path == "/status") {
    handle_web_server_status(query, headers, client);
  } else {
    client.print("unknown path ");
    client.println(path);
  }
  client.stop();
  return true;
}

void loop() {
  print_output_to_stream(Serial);
  delay(10000);
}

void loop1() {
  if (!handle_web_server()) {
    delay(100);  // slightly less busy wait
  }
}