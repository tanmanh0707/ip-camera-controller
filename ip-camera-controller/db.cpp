#include <Preferences.h>

#define PREF_NAME_IP_CAMERA                         "ipcamera"
#define PREF_KEY_WIFI_SSID                          "ssid"
#define PREF_KEY_WIFI_PASSWORD                      "password"
#define PREF_KEY_CAM_USERNAME                       "camusername"
#define PREF_KEY_CAM_PASSWORD                       "campassword"
#define PREF_KEY_CAM_IP_ADDR                        "ipaddress"

#define PREF_READONLY                               true
#define PREF_READWRITE                              false

static Preferences preferences;

void DB_SetCameraIpAddress(String &address)
{
  preferences.begin(PREF_NAME_IP_CAMERA, PREF_READWRITE);
  preferences.putString(PREF_KEY_CAM_IP_ADDR, address);
  preferences.end();
}

String DB_GetCameraIpAddress(void)
{
  String ipaddr;
  preferences.begin(PREF_NAME_IP_CAMERA, PREF_READONLY);
  ipaddr = preferences.getString(PREF_KEY_CAM_IP_ADDR);
  preferences.end();
  return ipaddr;
}

void DB_SetCameraAuth(String &username, String &password)
{
  preferences.begin(PREF_NAME_IP_CAMERA, PREF_READWRITE);
  preferences.putString(PREF_KEY_CAM_USERNAME, username);
  preferences.putString(PREF_KEY_CAM_PASSWORD, password);
  preferences.end();
}

void DB_GetCameraAuth(String &username, String &password)
{
  preferences.begin(PREF_NAME_IP_CAMERA, PREF_READONLY);
  username = preferences.getString(PREF_KEY_CAM_USERNAME);
  password = preferences.getString(PREF_KEY_CAM_PASSWORD);
  preferences.end();
}

void DB_GetWifiCredentials(String &ssid, String &password)
{
  preferences.begin(PREF_NAME_IP_CAMERA, PREF_READONLY);
  ssid = preferences.getString(PREF_KEY_WIFI_SSID);
  password = preferences.getString(PREF_KEY_WIFI_PASSWORD);
  preferences.end();
}

void DB_SetWifiCredentials(String &ssid, String &password)
{
  String db_ssid, db_password;
  DB_GetWifiCredentials(db_ssid, db_password);
  
  if (db_ssid != ssid || db_password != password)
  {
    preferences.begin(PREF_NAME_IP_CAMERA, PREF_READWRITE);
    preferences.putString(PREF_KEY_WIFI_SSID, ssid);
    preferences.putString(PREF_KEY_WIFI_PASSWORD, password);
    preferences.end();
    log_i("WiFi Credentials Saved: %s - %s", ssid.c_str(), password.c_str());
  }
}
