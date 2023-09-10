// intended for 'arduino nano esp32'
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "secrets.h"  // defines WiFi login info 'secret_wifi_network' and 'secret_wifi_password'

constexpr char const* url_time_server = "http://worldtimeapi.org/api/ip";
constexpr char const* url_astros = "http://api.open-notify.org/astros.json";
constexpr char const* url_jokes = "https://v2.jokeapi.dev/joke/Programming";

WiFiServer web_server(80);

char const* lookup_wifi_status_to_cstr(wl_status_t const status) {
  switch (status) {
    case WL_CONNECTED: return "connected";
    case WL_NO_SHIELD: return "no shield";
    case WL_IDLE_STATUS: return "idle";
    case WL_NO_SSID_AVAIL: return "no wifi network available";
    case WL_SCAN_COMPLETED: return "scan completed";
    case WL_CONNECT_FAILED: return "connect failed";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED: return "disconnected";
    default: return "unknown";
  }
}

void hang() {
  while (true) delay(10000);
}

// code to run on second core
TaskHandle_t task_second_core;
void func_second_core(void* vpParameter) {
  while (true) {
    loop1();
  }
}

// setup first core
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  while (!Serial && millis() < 10000)
    delay(100);  // wait max 10 seconds for serial over usb

  Serial.printf("\nconnecting to '%s' with '%s'\n", secret_wifi_network, secret_wifi_password);

  WiFi.mode(WIFI_STA);
  WiFi.begin(secret_wifi_network, secret_wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    switch (WiFi.status()) {
      case WL_CONNECT_FAILED:
        Serial.println("\n*** connection to wifi failed");
        hang();
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("\n*** network not found or wrong password");
        hang();
        break;
      default: break;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.print("\nconnected\nip: ");
  Serial.println(WiFi.localIP().toString().c_str());
  Serial.print("signal strength: ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.print("auto-reconnect: ");
  Serial.println(WiFi.getAutoReconnect() ? "yes" : "no");
  // todo: check status and reconnect if not WL_CONNECTED
  //       turning off wifi base station gives WL_NO_SSID_AVAIL
  digitalWrite(LED_BUILTIN, HIGH);

  Preferences preferences;
  preferences.begin("store", false);
  unsigned boot_count = preferences.getUInt("boot_count", 0);
  boot_count++;
  Serial.printf("boot count: %u\n", boot_count);
  preferences.putUInt("boot_count", boot_count);
  preferences.end();

  // setup second core
  web_server.begin();

  // start second core
  xTaskCreatePinnedToCore(func_second_core, "core1", 64 * 1024, NULL, 1, &task_second_core, !ARDUINO_RUNNING_CORE);
}

// returns true if request succeeded or false if something went wrong
bool read_url_to_json_doc(char const* url, JsonDocument& json_doc) {
  HTTPClient http_client;
  http_client.useHTTP10();
  http_client.setConnectTimeout(10000);
  http_client.setTimeout(10000);

  if (!http_client.begin(url)) {
    Serial.printf("*** unable to connect to %s\n", url);
    return false;
  }
  auto const http_code = http_client.GET();
  if (http_code != HTTP_CODE_OK) {
    Serial.printf("*** GET error: %d: %s\n", http_code, http_client.errorToString(http_code).c_str());
    http_client.end();
    return false;
  }
  auto const json_error = deserializeJson(json_doc, http_client.getStream());
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
  if (!read_url_to_json_doc(url_astros, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  auto const people = json_doc["people"].as<JsonArray>();
  for (auto const& p : people) {
    os.println(p["name"].as<char const*>());
  }
  //const unsigned n = json_doc["number"].as<unsigned>();
  //for (unsigned i = 0; i < n; i++) {
  //  Serial.println(json_people[i]["name"].as<char const*>());
  //}
}

void print_current_time_based_on_ip(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  StaticJsonDocument<1024> json_doc;  // memory allocated on the stack
  if (!read_url_to_json_doc(url_time_server, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  auto const date_time_raw = json_doc["datetime"].as<String>();
  //  "2023-08-31T16:32:47.653086+02:00" to "2023-08-31 16:32:47"
  auto const date_time = date_time_raw.substring(0, 10) + " " + date_time_raw.substring(11, 19);
  os.println(date_time);
}

void print_random_programming_joke(Stream& os) {
  digitalWrite(LED_BUILTIN, LOW);
  StaticJsonDocument<1024> json_doc;  // memory allocated on the stack
  if (!read_url_to_json_doc(url_jokes, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  if (json_doc["type"].as<String>() == "single") {
    os.println(json_doc["joke"].as<char const*>());
  } else {
    os.println(json_doc["setup"].as<char const*>());
    os.println(json_doc["delivery"].as<char const*>());
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
  os.print(lookup_wifi_status_to_cstr(WiFi.status()));
  os.print(" ");
  os.print(WiFi.RSSI());
  os.println(" dBm");
}

void print_heap_info(Stream& os) {
  os.print("used: ");
  // os.printf("total: %u B\n", ESP.getHeapSize());
  // os.printf(" free: %u B\n", ESP.getFreeHeap());
  // os.printf(" used: %u B\n", ESP.getHeapSize() - ESP.getFreeHeap());
  os.print(ESP.getHeapSize() - ESP.getFreeHeap());
  os.println(" B");
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

  os.println("\nheap info:");
  print_heap_info(os);
}

// serve "/"
void handle_web_server_root(String const& path, String const& query, std::vector<String> const& headers, Stream& os) {
  os.print("<pre>path: ");
  os.println(path);
  os.print("query: ");
  os.println(query);
  for (auto const& s : headers) {
    os.println(s);
  }

  print_heap_info(os);
}

// serve "/status"
void handle_web_server_status(String const& path, String const& query, std::vector<String> const& headers, Stream& os) {
  os.print("<pre>path: ");
  os.println(path);
  os.print("query: ");
  os.println(query);
  for (auto const& s : headers) {
    os.println(s);
  }

  print_output_to_stream(os);
}

// serve "/rgbled"
void handle_web_server_rgbled(String const& path, String const& query, std::vector<String> const& headers, Stream& os) {
  bool const r = query.indexOf("r=1") != -1;
  bool const g = query.indexOf("g=1") != -1;
  bool const b = query.indexOf("b=1") != -1;

  digitalWrite(LED_RED, r ? LOW : HIGH);
  digitalWrite(LED_GREEN, g ? LOW : HIGH);
  digitalWrite(LED_BLUE, b ? LOW : HIGH);

  os.println("<!doctype html><meta name=viewport content=\"width=device-width,initial-scale=1\"><meta charset=utf-8><title>RGB Led</title>");
  os.print("<form>RGB Led: ");
  
  os.print("<input type=checkbox name=r value=1 ");
  if (r) os.print("checked");
  os.print("> red ");

  os.print("<input type=checkbox name=g value=1 ");
  if (g) os.print("checked");
  os.print("> green ");

  os.print("<input type=checkbox name=b value=1 ");
  if (b) os.print("checked");
  os.print("> blue ");

  os.print("<input type=submit value=apply>");
  os.print("</form>");
}

// returns true if a request was serviced or false if no client available
bool handle_web_server() {
  WiFiClient client = web_server.available();
  if (!client)
    return false;

  //  digitalWrite(LED_GREEN, LOW);  // turn on green led

  // read first request line
  auto const method = client.readStringUntil(' ');
  auto const uri = client.readStringUntil(' ');
  auto const version = client.readStringUntil('\r');
  if (client.read() != '\n') {
    Serial.println("*** malformed http request");
    // digitalWrite(LED_GREEN, HIGH);
    return false;
  }

  auto const query_start_ix = uri.indexOf("?");
  auto const path = query_start_ix == -1 ? uri : uri.substring(0, query_start_ix);
  auto const query = query_start_ix == -1 ? "" : uri.substring(query_start_ix + 1);

  std::vector<String> headers;
  while (true) {
    auto const line = client.readStringUntil('\r');
    if (client.read() != '\n') {
      Serial.println("*** malformed http request");
      // digitalWrite(LED_GREEN, HIGH);
      return false;
    }
    if (line.length() == 0)
      break;
    headers.push_back(line);
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();

  if (path == "/") {
    handle_web_server_root(path, query, headers, client);
  } else if (path == "/status") {
    handle_web_server_status(path, query, headers, client);
  } else if (path == "/rgbled") {
    handle_web_server_rgbled(path, query, headers, client);
  } else {
    client.print("unknown path '");
    client.print(path);
    client.println("'");
  }

  client.stop();

  // digitalWrite(LED_GREEN, HIGH); // turn off green led

  return true;
}

// loop on first core
void loop() {
  print_output_to_stream(Serial);
  // delay(10000);
}

// loop on second core
void loop1() {
  while (handle_web_server())
    ;
  delay(100);  // slightly less busy wait
}