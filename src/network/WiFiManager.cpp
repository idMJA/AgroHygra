#include "WiFiManager.h"

WiFiManager::WiFiManager() : savedSSID(""), savedPassword(""), apMode(false) {
}

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
}

void WiFiManager::loadCredentials() {
  preferences.begin("wifi", true);
  savedSSID = preferences.getString("ssid", "");
  savedPassword = preferences.getString("password", "");
  preferences.end();

  if (savedSSID.length() > 0) {
    Serial.println("📡 Loaded WiFi credentials from flash:");
    Serial.println("   SSID: " + savedSSID);
  } else {
    Serial.println("⚠️  No saved WiFi credentials found");
  }
}

void WiFiManager::saveCredentials(String ssid, String password) {
  preferences.begin("wifi", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.end();
  savedSSID = ssid;
  savedPassword = password;
  Serial.println("✅ WiFi credentials saved to flash");
}

void WiFiManager::clearCredentials() {
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  savedSSID = "";
  savedPassword = "";
  Serial.println("🗑️  WiFi credentials cleared");
}

bool WiFiManager::connect() {
  if (savedSSID.length() == 0) {
    Serial.println("❌ No WiFi credentials configured");
    return false;
  }

  Serial.println("📡 Connecting to WiFi: " + savedSSID);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(); // Ensure clean state
  WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

  unsigned long startTime = millis();
  int attempts = 0;
  
  // Wait for connection with improved timeout handling (15 seconds max)
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) {
    delay(500);
    Serial.print(".");
    attempts++;
    
    // Check for permanent failures
    if (WiFi.status() == WL_CONNECT_FAILED || WiFi.status() == WL_NO_SSID_AVAIL) {
      Serial.println();
      Serial.println("❌ WiFi connection failed (invalid SSID or credentials)!");
      return false;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    apMode = false;
    Serial.println("\n✅ WiFi connected!");
    Serial.print("   IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("   Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    delay(500); // Allow time for IP configuration to stabilize
    return true;
  } else {
    Serial.println("\n❌ WiFi connection timeout");
    return false;
  }
}

void WiFiManager::startAPMode() {
  Serial.println("🔷 Starting WiFi Access Point Mode...");
  WiFi.mode(WIFI_AP);
  WiFi.softAP("AgroHygra-Setup", "agrohygra123");
  
  IPAddress IP = WiFi.softAPIP();
  apMode = true;
  
  Serial.println("✅ Access Point started");
  Serial.print("   SSID: AgroHygra-Setup\n");
  Serial.print("   Password: agrohygra123\n");
  Serial.print("   IP address: ");
  Serial.println(IP);
  Serial.println("   Connect to this AP and visit http://192.168.4.1");
}

String WiFiManager::scanNetworks() {
  Serial.println("🔍 Scanning WiFi networks...");
  int n = WiFi.scanNetworks();
  String networks = "";

  if (n == 0) {
    networks = "No networks found";
  } else {
    Serial.printf("Found %d networks\n", n);
    for (int i = 0; i < n; ++i) {
      networks += "<option value=\"" + WiFi.SSID(i) + "\">";
      networks += WiFi.SSID(i);
      networks += " (" + String(WiFi.RSSI(i)) + " dBm)";
      networks += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " [Open]" : " [Secured]";
      networks += "</option>";
    }
  }
  return networks;
}
