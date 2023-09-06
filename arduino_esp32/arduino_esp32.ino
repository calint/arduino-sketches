#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "secrets.h"  // defines WIFI_NETWORK and WIFI_PASSWORD used for WiFi login

#define TIME_SERVER_URL "http://worldtimeapi.org/api/ip"
#define ASTROS_URL "http://api.open-notify.org/astros.json"
#define JOKES_URL "https://v2.jokeapi.dev/joke/Programming"

WiFiServer web_server(80);

const char* lookup_wifi_status_to_text(const wl_status_t status) {
  switch (status) {
    case WL_CONNECTED:
      return "connected";
    case WL_NO_SHIELD:
      return "no shield";
    case WL_IDLE_STATUS:
      return "idle";
    case WL_NO_SSID_AVAIL:
      return "no SSID available";
    case WL_SCAN_COMPLETED:
      return "scan completed";
    case WL_CONNECT_FAILED:
      return "connect failed";
    case WL_CONNECTION_LOST:
      return "connection lost";
    case WL_DISCONNECTED:
      return "disconnected";
    default:
      return "Unknown";
  }
}

#ifdef ARDUINO_NANO_ESP32
TaskHandle_t task_loop1;
void esploop1(void* vpParameter) {
  setup1();
  while (true) {
    loop1();
  }
}
#endif

// setup first core
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  while (!Serial && millis() < 10000)
    ;  // wait for serial over usb for 10 seconds
  Serial.printf("\nconnecting to '%s' with '%s'\n", WIFI_NETWORK, WIFI_PASSWORD);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("\n*** connection to wifi failed");
      while (true) delay(10000);
    }
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nconnected\nip: ");
  Serial.println(WiFi.localIP().toString().c_str());
  Serial.print("signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  digitalWrite(LED_BUILTIN, HIGH);

#ifdef ARDUINO_NANO_ESP32
  xTaskCreatePinnedToCore(esploop1, "loop1", 64 * 1024, NULL, 1, &task_loop1, !ARDUINO_RUNNING_CORE);
#endif
}

// setup second core
void setup1() {
  web_server.begin();
}

// returns true if request succeeded or false if something went wrong
bool read_url_to_json_doc(const char* url, JsonDocument& json_doc) {
  HTTPClient http_client;
  http_client.useHTTP10(true);

#ifdef RASPBERRYPI_PICO
  if (!strncmp(url, "https://", 8)) {  // 8 characters in "https://"
    // todo: https implementation does not seem to be thread safe
    //       running on two cores hangs the Raspberry Pico W
    http_client.setInsecure();
  }
#endif

  if (!http_client.begin(url)) {
    Serial.printf("*** unable to connect to %s\n", url);
    return false;
  }
  const auto http_code = http_client.GET();
  if (http_code != HTTP_CODE_OK) {
    Serial.printf("*** GET error: %d: %s\n", http_code, http_client.errorToString(http_code).c_str());
    http_client.end();
    return false;
  }
  const auto json_error = deserializeJson(json_doc, http_client.getStream());
  http_client.end();
  if (json_error) {
    Serial.printf("*** json parsing failed: %s\n", json_error.c_str());
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
  for (const auto& p : people) {
    os.println(p["name"].as<const char*>());
  }
  //const unsigned n = json_doc["number"].as<unsigned>();
  //for (unsigned i = 0; i < n; i++) {
  //  Serial.println(json_people[i]["name"].as<const char*>());
  //}
}

void print_current_time_based_on_ip(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  StaticJsonDocument<1024> json_doc;  // memory allocated on the stack
  if (!read_url_to_json_doc(TIME_SERVER_URL, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  const auto date_time_raw = json_doc["datetime"].as<String>();
  //  "2023-08-31T16:32:47.653086+02:00" to "2023-08-31 16:32:47"
  const auto date_time = date_time_raw.substring(0, 10) + " " + date_time_raw.substring(11, 19);
  os.println(date_time);
}

void print_random_programming_joke(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  StaticJsonDocument<1024> json_doc;  // memory allocated on the stack
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
  WiFiUDP ntp_udp;
  NTPClient ntp_client(ntp_udp);  // default 'pool.ntp.org', 60 seconds update interval, no offset
  digitalWrite(LED_BUILTIN, LOW);
  ntp_client.update();
  digitalWrite(LED_BUILTIN, HIGH);
  os.println(ntp_client.getFormattedTime());
}

void print_web_server_ip(Stream& os) {
  os.println(WiFi.localIP().toString().c_str());
}

void print_wifi_status(Stream& os) {
  os.print(lookup_wifi_status_to_text(WiFi.status()));
  os.print(" ");
  os.print(WiFi.RSSI());
  os.println(" dBm");
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

  os.println("\nwifi status: ");
  print_wifi_status(os);
}

// serve "/"
void handle_web_server_root(const String& query, const std::vector<String>& headers, Stream& os) {
  os.print("<pre>query: ");
  os.println(query);
  for (const auto& s : headers) {
    os.println(s);
  }
}

// serve "/status"
void handle_web_server_status(const String& query, const std::vector<String>& headers, Stream& os) {
  os.print("<pre>query: ");
  os.println(query);
  for (const auto& s : headers) {
    os.println(s);
  }

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

  os.println("\nwifi status: ");
  print_wifi_status(os);
}

// returns true if a request was serviced or false if no client available
bool handle_web_server() {
  WiFiClient client = web_server.available();
  if (!client)
    return false;

  // read first request line
  const auto method = client.readStringUntil(' ');
  const auto uri = client.readStringUntil(' ');
  const auto version = client.readStringUntil('\r');
  if (client.read() != '\n') {
    Serial.println("*** malformed http request");
    return false;
  }

  const auto query_start_ix = uri.indexOf("?");
  const auto path = query_start_ix == -1 ? uri : uri.substring(0, query_start_ix);
  const auto query = query_start_ix == -1 ? "" : uri.substring(query_start_ix + 1);

  std::vector<String> headers;
  while (true) {
    const auto line = client.readStringUntil('\r');
    if (client.read() != '\n') {
      Serial.println("*** malformed http request");
      return false;
    }
    if (line.length() == 0)
      break;
    headers.push_back(line);
  }

  // Serial.println("\nwebserver request: ");
  // Serial.println("-------------------------------------------------");
  // Serial.print("path: ");
  // Serial.println(path);
  // Serial.print("query: ");
  // Serial.println(query);
  // for (const auto& s : headers)
  //   Serial.println(s);
  // Serial.println("-------------------------------------------------");

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

// loop on first core
void loop() {
  print_output_to_stream(Serial);
  // delay(10'000);
}

// loop on second core
void loop1() {
  while (handle_web_server())
    ;
  delay(100);  // slightly less busy wait
}