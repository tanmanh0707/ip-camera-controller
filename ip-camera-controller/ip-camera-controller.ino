#include "configs.h"

/***********************************************
 * Variables
 ***********************************************/
String _wifiSsid;
String _wifiPassword;

String _camUsername;
String _camPassword;
String _camServer;
String _camUri = IPCAM_URI;

WebServer _apServer(80);

static WiFiUDP _ntpUDP;
static NTPClient _timeClient(_ntpUDP, "pool.ntp.org", TIMEZONE_OFFSET, 60000);
static struct tm *_currentTime;
static unsigned long _timeUpdate = 0;

HumaButtonStates_e _btn1 = HUMA_RELEASED;
HumaButtonStates_e _btn2 = HUMA_RELEASED;
unsigned long _btn1PressedTime = 0;
unsigned long _btn2PressedTime = 0;
unsigned long _sendLogTime = 0;

/***********************************************
 * Functions
 ***********************************************/
void LocalGetDateTime(void);
void LocalSendSnapshot(uint8_t *buffer, size_t len);
void LocalSendLog(void);
void CAMERA_GetSnapshot(uint8_t *buffer, size_t &len);
void WIFI_AccessPoint(void);
String LocalGetDateTimeString(void);
size_t WIFI_ClientReadBytes(WiFiClient *stream, uint8_t *buffer, size_t len);

String exractParam(String& authReq, const String& param, const char delimit) {
  int _begin = authReq.indexOf(param);
  if (_begin == -1) { return ""; }
  return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

String getCNonce(const int len) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
  String s = "";

  for (int i = 0; i < len; ++i) { s += alphanum[rand() % (sizeof(alphanum) - 1)]; }

  return s;
}

String getDigestAuth(String& authReq, const String& username, const String& password, const String& method, const String& uri, unsigned int counter) {
  // extracting required parameters for RFC 2069 simpler Digest
  String realm = exractParam(authReq, "realm=\"", '"');
  String nonce = exractParam(authReq, "nonce=\"", '"');
  String cNonce = getCNonce(8);

  char nc[9];
  snprintf(nc, sizeof(nc), "%08x", counter);

  // parameters for the RFC 2617 newer Digest
  MD5Builder md5;
  md5.begin();
  md5.add(username + ":" + realm + ":" + password);  // md5 of the user:realm:user
  md5.calculate();
  String h1 = md5.toString();

  md5.begin();
  md5.add(method + ":" + uri);
  md5.calculate();
  String h2 = md5.toString();

  md5.begin();
  md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
  md5.calculate();
  String response = md5.toString();

  String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce + "\", uri=\"" + uri + "\", algorithm=\"MD5\", qop=auth, nc=" + String(nc) + ", cnonce=\"" + cNonce + "\", response=\"" + response + "\"";
  Serial.println(authorization);

  return authorization;
}

void setup() {
  randomSeed(10);
  Serial.begin(115200);

  _camServer = DB_GetCameraIpAddress();
  if (_camServer.length() == 0) {
     _camServer = IPCAM_SERVER;
  } else {
    _camServer = "http://" + _camServer;
  }

  DB_GetCameraAuth(_camUsername, _camPassword);
  if (_camUsername.length() == 0) {
    _camUsername = IPCAM_USERNAME;
  }
  if (_camPassword.length() == 0) {
    _camPassword = IPCAM_PASSWORD;
  }

  DB_GetWifiCredentials(_wifiSsid, _wifiPassword);
  if (_wifiSsid.length() == 0) {
    _wifiSsid = WIFI_SSID;
  }
  if (_wifiPassword.length() == 0) {
    _wifiPassword = WIFI_PASS;
  }

  Serial.printf("%s:%s - %s%s\n", _camUsername.c_str(), _camPassword.c_str(), _camServer.c_str(), _camUri.c_str());
  Serial.printf("WiFi: %s - %s\n", _wifiSsid.c_str(), _wifiPassword.c_str());

  Huma_Buttons.add(BUTTON_1);
  Huma_Buttons.add(BUTTON_2);
  Huma_Buttons.add(BUTTON_CONFIG);

  WiFi.mode(WIFI_STA);
  WiFi.begin(_wifiSsid, _wifiPassword);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  _timeClient.begin();
}

void loop()
{
  /* Handle Access Point */
  if (WiFi.getMode() == WIFI_AP) {
    _apServer.handleClient();
    return;
  }
  
  HumaButtonStates_e btn1 = Huma_Buttons.state(BUTTON_1);
  HumaButtonStates_e btn2 = Huma_Buttons.state(BUTTON_2);

  if (Huma_Buttons.hold(BUTTON_CONFIG, BUTTON_CONFIG_HOLD_TIME)) {
    if (WiFi.getMode() == WIFI_STA) {
      Serial.println("Button config long pressed");
      WIFI_AccessPoint();
    }
  }

  if (btn1 != _btn1) {
    _btn1 = btn1;
    if (btn1 == HUMA_PRESSED) {
      Serial.println("Button 1 pressed");
      _btn1PressedTime = millis();

      uint8_t *snapshot = nullptr;
      size_t snapshot_len = 0;
      CAMERA_GetSnapshot(snapshot, snapshot_len);
      if (snapshot) {
        LocalSendSnapshot(snapshot, snapshot_len);
        free(snapshot);
        snapshot = nullptr;
      }
    } else {
      Serial.println("Button 1 released");
    }
  }

  if (btn2 != _btn2) {
    _btn2 = btn2;
    if (btn2 == HUMA_PRESSED) {
      Serial.println("Button 2 pressed");
      _btn2PressedTime = millis();
    } else {
      Serial.println("Button 2 released");
    }
  }

  /* Both button off */
  if (btn1 == HUMA_RELEASED && btn2 == HUMA_RELEASED) {
    if (millis() - _sendLogTime >= SEND_LOG_DURATION) {
      LocalSendLog();
      _sendLogTime = millis();
    }
  }

  /* Time */
  if (millis() - _timeUpdate >= 1000) {
    LocalGetDateTime();
    _timeUpdate = millis();
  }
}

void CAMERA_GetSnapshot(uint8_t *buffer, size_t &len)
{
  WiFiClient client;
  HTTPClient http;

  Serial.print("[HTTP] begin...\n");

  // configure traged server and url
  http.begin(client, String(_camServer) + String(_camUri));

  const char* keys[] = { "WWW-Authenticate" };
  http.collectHeaders(keys, 1);

  Serial.print("[HTTP] GET...\n");
  // start connection and send HTTP header
  int httpCode = http.GET();

  if (httpCode > 0) {
    String authReq = http.header("WWW-Authenticate");
    Serial.println(authReq);

    String authorization = getDigestAuth(authReq, String(_camUsername), String(_camPassword), "GET", String(_camUri), 1);

    http.end();
    http.begin(client, String(_camServer) + String(_camUri));

    http.addHeader("Authorization", authorization);

    int httpCode = http.GET();
    if (httpCode > 0) {
      if (httpCode == 200) {
        len = http.getSize();
        buffer = (uint8_t *)malloc(len);
        if (buffer) {
          WiFiClient *stream = http.getStreamPtr();
          size_t readBytes = WIFI_ClientReadBytes(stream, buffer, len);
          if (readBytes == len) {
            Serial.println("[HTTP] Download finished");
          } else {
            Serial.printf("[HTTP] Download failed! %d / %d\n", readBytes, len);
            free(buffer);
            buffer = nullptr;
          }
        }
      } else {
        String payload = http.getString();
        Serial.printf("[HTTP] Response error (%d): %s\n%s\n", httpCode, http.errorToString(httpCode).c_str(), payload.c_str());     
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

size_t WIFI_ClientReadBytes(WiFiClient *stream, uint8_t *buffer, size_t len)
{
  size_t readBytes = 0;
  int timeout_failures = 0;

  if (stream && buffer)
  {
    while ( ! readBytes) {
      readBytes = stream->readBytes(buffer, len);
      if (readBytes == 0) {
        timeout_failures++;
        if (timeout_failures >= (DOWNLOAD_READ_TIMEOUT / 100 /*delay(100)*/)) {
          break;
        }
        delay(100);
      }
    }
  }

  return readBytes;
}

void WIFI_AccessPoint()
{
  WiFi.mode(WIFI_OFF);
  WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASS);
  Serial.print("Access Point IP: "); Serial.println(WiFi.softAPIP());

  _apServer.on("/", []() {
    _apServer.send(200, "text/html", index_html);
  });
  _apServer.on("/settings", HTTP_POST, []() {
    String username = _apServer.arg("username");
    String password = _apServer.arg("password");
    String deviceIp = _apServer.arg("deviceIp");
    Serial.print("Username: "); Serial.println(username);
    Serial.print("Password: "); Serial.println(password);
    Serial.print("DeviceIP: "); Serial.println(deviceIp);

    DB_SetCameraIpAddress(deviceIp);
    DB_SetCameraAuth(username, password);

    _apServer.send(200, "text/plain", "Successful");
  });
  _apServer.begin();
}

void LocalSendSnapshot(uint8_t *buffer, size_t len)
{
  
}

void LocalSendLog(void)
{
  
}

String LocalGetDateTimeString()
{
  char timebuf[32] = {0};
  snprintf(timebuf, 32, "%02d/%02d/%04d %02d:%02d:%02d", 
                        _currentTime->tm_mday, _currentTime->tm_mon + 1, _currentTime->tm_year + 1900, 
                        _currentTime->tm_hour, _currentTime->tm_min, _currentTime->tm_sec);

  return String(timebuf);
}

void LocalGetDateTime()
{
  if (_timeClient.update()) {
    Serial.println("Time updated!");
    time_t epochTime = _timeClient.getEpochTime();
    _currentTime = localtime(&epochTime);
  }

  _currentTime->tm_sec += (millis() - _timeUpdate) / 1000;

  if (_currentTime->tm_sec > 59) {
    _currentTime->tm_sec = 0;
    _currentTime->tm_min++;

    if (_currentTime->tm_min > 59) {
      _currentTime->tm_min = 0;
      _currentTime->tm_hour++;

      if (_currentTime->tm_hour > 23) {
        _currentTime->tm_hour = 0;
      }
    }
  }

  Serial.print("Datetime: "); Serial.println(LocalGetDateTimeString());
}
