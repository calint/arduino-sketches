// intended for: Arduino Nano ESP32
// developed in: Arduino IDE 2.2.1
//        board: Arduino ESP32 Boards 2.0.13
//    libraries: NTPClient 3.2.1
//               ArduinoJson 6.21.3
#include <WiFi.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define let auto const&
#define var auto
#define cstr const char*
#define fn auto

#include "secrets.h"  // defines WiFi login info 'secret_wifi_network' and 'secret_wifi_password'

constexpr cstr url_time_server = "http://worldtimeapi.org/api/ip";
constexpr cstr url_astros = "http://api.open-notify.org/astros.json";
constexpr cstr url_jokes = "https://v2.jokeapi.dev/joke/Programming";

WiFiServer web_server(80);

fn lookup_wifi_status_to_cstr(wl_status_t const status)->cstr {
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

fn hang()->void {
  while (true)
    delay(10000);
}

fn loop1()->void;

// code to run on second core
TaskHandle_t task_second_core;
fn func_second_core(void* vpParameter)->void {
  while (true)
    loop1();
}

// setup first core
fn setup()->void {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  while (!Serial && millis() < 10000)
    delay(100);  // wait max 10 seconds for serial over usb

  Serial.printf("\r\nconnecting to '%s' with '%s'\r\n", secret_wifi_network, secret_wifi_password);

  WiFi.mode(WIFI_STA);
  WiFi.begin(secret_wifi_network, secret_wifi_password);
  for (var sts = WiFi.status(); sts != WL_CONNECTED; sts = WiFi.status()) {
    switch (sts) {
      case WL_CONNECT_FAILED:
        Serial.println("\r\n*** connection to wifi failed");
        hang();
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println("\r\n*** network not found or wrong password");
        hang();
        break;
      default: break;
    }
    Serial.print(".");
    delay(500);
  }
  Serial.print("\r\nconnected\r\nip: ");
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
  var boot_count = preferences.getUInt("boot_count", 0);
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
fn read_url_to_json_doc(cstr url, JsonDocument& json_doc)->bool {
  HTTPClient http_client;
  http_client.useHTTP10();
  http_client.setConnectTimeout(10000);
  http_client.setTimeout(10000);

  if (!http_client.begin(url)) {
    Serial.printf("*** unable to connect to %s\n", url);
    return false;
  }
  let http_code = http_client.GET();
  if (http_code != HTTP_CODE_OK) {
    Serial.printf("*** GET error: %d: %s\n", http_code, http_client.errorToString(http_code).c_str());
    http_client.end();
    return false;
  }
  let json_error = deserializeJson(json_doc, http_client.getStream());
  http_client.end();
  if (json_error) {
    Serial.printf("*** json parsing failed: %s\n", json_error.c_str());
    return false;
  }
  return true;
}

fn print_astronauts_in_space_right_now(Stream& os)->void {
  digitalWrite(LED_BUILTIN, LOW);
  DynamicJsonDocument json_doc(8 * 1024);
  if (!read_url_to_json_doc(url_astros, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  let people = json_doc["people"].as<JsonArray>();
  for (let p : people) {
    os.println(p["name"].as<cstr>());
  }
}

fn print_current_time_based_on_ip(Stream& os)->void {
  digitalWrite(LED_BUILTIN, LOW);
  StaticJsonDocument<1024> json_doc;  // memory allocated on the stack
  if (!read_url_to_json_doc(url_time_server, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  let date_time_raw = json_doc["datetime"].as<String>();
  //  "2023-08-31T16:32:47.653086+02:00" to "2023-08-31 16:32:47"

  // note. the line below results in an empty string. why?
  // let date_time = date_time_raw.substring(0, 10) + " " + date_time_raw.substring(11, 19);

  let date_time = (date_time_raw.substring(0, 10) + " " + date_time_raw.substring(11, 19)).c_str();
  // let date_time = String(date_time_raw.substring(0, 10) + " " + date_time_raw.substring(11, 19));
  os.println(date_time);
}

fn print_random_programming_joke(Stream& os)->void {
  digitalWrite(LED_BUILTIN, LOW);
  StaticJsonDocument<1024> json_doc;  // memory allocated on the stack
  if (!read_url_to_json_doc(url_jokes, json_doc)) return;
  digitalWrite(LED_BUILTIN, HIGH);
  if (json_doc["type"].as<String>() == "single") {
    os.println(json_doc["joke"].as<cstr>());
  } else {
    os.println(json_doc["setup"].as<cstr>());
    os.println(json_doc["delivery"].as<cstr>());
  }
}

fn print_current_time_from_ntp(Stream& os)->void {
  WiFiUDP ntp_udp;
  NTPClient ntp_client(ntp_udp);  // default 'pool.ntp.org', 60 seconds update interval, no offset
  digitalWrite(LED_BUILTIN, LOW);
  ntp_client.update();
  digitalWrite(LED_BUILTIN, HIGH);
  os.println(ntp_client.getFormattedTime());
}

fn print_web_server_ip(Stream& os)->void {
  os.println(WiFi.localIP().toString());
}

fn print_wifi_status(Stream& os)->void {
  os.print(lookup_wifi_status_to_cstr(WiFi.status()));
  os.print(" ");
  os.print(WiFi.RSSI());
  os.println(" dBm");
}

fn print_heap_info(Stream& os)->void {
  os.print("used: ");
  // os.printf("total: %u B\n", ESP.getHeapSize());
  // os.printf(" free: %u B\n", ESP.getFreeHeap());
  // os.printf(" used: %u B\n", ESP.getHeapSize() - ESP.getFreeHeap());
  os.print(ESP.getHeapSize() - ESP.getFreeHeap());
  os.println(" B");
}

fn print_output_to_stream(Stream& os)->void {
  os.println("\r\ncurrent time based on ip:");
  print_current_time_based_on_ip(os);

  os.println("\r\ncurrent time in utc from ntp:");
  print_current_time_from_ntp(os);

  os.println("\r\nastronauts in space right now:");
  print_astronauts_in_space_right_now(os);

  os.println("\r\nprogramming joke:");
  print_random_programming_joke(os);

  os.println("\r\nweb server ip:");
  print_web_server_ip(os);

  os.println("\r\nwifi status: ");
  print_wifi_status(os);

  os.println("\r\nheap info:");
  print_heap_info(os);
}

// serve "/"
fn handle_web_server_root(String const& path, String const& query, std::vector<String> const& headers, Stream& os)->void {
  os.print("<pre>path: ");
  os.println(path);
  os.print("query: ");
  os.println(query);
  for (let s : headers) {
    os.println(s);
  }

  print_heap_info(os);
}

// serve "/status"
fn handle_web_server_status(String const& path, String const& query, std::vector<String> const& headers, Stream& os)->void {
  os.print("<pre>path: ");
  os.println(path);
  os.print("query: ");
  os.println(query);
  for (let s : headers) {
    os.println(s);
  }

  print_output_to_stream(os);
}

// serve "/rgbled"
fn handle_web_server_rgbled(String const& path, String const& query, std::vector<String> const& headers, Stream& os)->void {
  let r = query.indexOf("r=1") != -1;
  let g = query.indexOf("g=1") != -1;
  let b = query.indexOf("b=1") != -1;

  digitalWrite(LED_RED, r ? LOW : HIGH);
  digitalWrite(LED_GREEN, g ? LOW : HIGH);
  digitalWrite(LED_BLUE, b ? LOW : HIGH);

  os.println("<!doctype html><meta name=viewport content=\"width=device-width,initial-scale=1\"><meta charset=utf-8><style>*{font-family:monospace}</style><title>RGB Led</title>");
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
fn handle_web_server()->bool {
  var client = web_server.available();
  if (!client)
    return false;

  // read first request line
  let method = client.readStringUntil(' ');
  let uri = client.readStringUntil(' ');
  let version = client.readStringUntil('\r');
  if (client.read() != '\n') {
    Serial.println("*** malformed http request");
    return false;
  }

  let query_start_ix = uri.indexOf("?");
  let path = query_start_ix == -1 ? uri : uri.substring(0, query_start_ix);
  let query = query_start_ix == -1 ? "" : uri.substring(query_start_ix + 1);

  std::vector<String> headers;
  // var nheaders = 32;  // maximum number of headers
  // while (nheaders--) {
  while (true) {
    let line = client.readStringUntil('\r');
    if (client.read() != '\n') {
      Serial.println("*** malformed http request");
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

  return true;
}

// loop on first core
fn loop()->void {
  print_output_to_stream(Serial);
  delay(10000);
}

// loop on second core
fn loop1()->void {
  while (handle_web_server())
    ;
  delay(100);  // slightly less busy wait
}