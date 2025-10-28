#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* ssid = "Vivo 9";
const char* password = "99999999";

// GitHub Configuration
const char* githubRepo = "YOUR_USERNAME/YOUR_REPO_NAME";
const char* firmwareURL = "https://raw.githubusercontent.com/YOUR_USERNAME/YOUR_REPO_NAME/main/firmware.bin";
const char* versionURL = "https://raw.githubusercontent.com/YOUR_USERNAME/YOUR_REPO_NAME/main/version.json";

// Web Server
WebServer server(80);

// Firmware Info
String currentVersion = "1.0.0";
bool updateAvailable = false;
String latestVersion = "";
String updateStatus = "No update available";
bool updateInProgress = false;
int updateProgress = 0;

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Setup web server routes
  setupWebServer();
  
  // Check for updates on startup
  checkForUpdates();
  
  Serial.println("Web Server started!");
  Serial.print("Visit: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  delay(10);
}

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

void setupWebServer() {
  // Serve main control page
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";
    html += "<title>ESP32 OTA Update</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; background: #f0f0f0; }";
    html += ".container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
    html += ".status-card { background: #f8f9fa; padding: 20px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #007bff; }";
    html += ".button { background: #007bff; border: none; color: white; padding: 12px 24px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 5px; cursor: pointer; border-radius: 5px; }";
    html += ".button:hover { background: #0056b3; }";
    html += ".button:disabled { background: #6c757d; cursor: not-allowed; }";
    html += ".button-success { background: #28a745; }";
    html += ".button-success:hover { background: #1e7e34; }";
    html += ".button-danger { background: #dc3545; }";
    html += ".button-danger:hover { background: #c82333; }";
    html += ".update-available { color: #28a745; font-weight: bold; }";
    html += ".update-not-available { color: #6c757d; }";
    html += ".progress-bar { width: 100%; background-color: #e9ecef; border-radius: 5px; margin: 10px 0; }";
    html += ".progress { width: 0%; height: 25px; background-color: #28a745; border-radius: 5px; transition: width 0.3s; text-align: center; line-height: 25px; color: white; font-weight: bold; }";
    html += ".log { background: #1a1a1a; color: #00ff00; padding: 15px; border-radius: 5px; height: 200px; overflow-y: auto; font-family: monospace; margin-top: 20px; font-size: 12px; }";
    html += ".info-box { background: #d1ecf1; border: 1px solid #bee5eb; color: #0c5460; padding: 15px; border-radius: 5px; margin: 15px 0; }";
    html += "</style>";
    html += "</head>";
    html += "<body>";
    html += "<div class='container'>";
    html += "<h1>🚀 ESP32 OTA Update System</h1>";
    
    html += "<div class='info-box'>";
    html += "<strong>📡 Device IP:</strong> " + WiFi.localIP().toString() + " | <strong>📶 WiFi Strength:</strong> " + String(WiFi.RSSI()) + " dBm";
    html += "</div>";
    
    html += "<div class='status-card'>";
    html += "<h2>📋 Version Information</h2>";
    html += "<p><strong>Current Version:</strong> <span id='currentVersion'>" + currentVersion + "</span></p>";
    html += "<p><strong>Latest Version:</strong> <span id='latestVersion'>" + latestVersion + "</span></p>";
    html += "<p><strong>Update Status:</strong> <span id='updateStatus'>" + updateStatus + "</span></p>";
    html += "</div>";

    html += "<div class='status-card'>";
    html += "<h2>🎮 Update Controls</h2>";
    html += "<button class='button' onclick='checkUpdate()' id='checkBtn'>🔍 Check for Updates</button>";
    html += "<button class='button button-success' onclick='installUpdate()' id='installBtn' disabled>📥 Install Update</button>";
    html += "<button class='button button-danger' onclick='restartDevice()'>🔄 Restart Device</button>";
    html += "</div>";

    html += "<div class='status-card' id='progressSection' style='display:none;'>";
    html += "<h2>📊 Update Progress</h2>";
    html += "<div class='progress-bar'>";
    html += "<div class='progress' id='updateProgress'>0%</div>";
    html += "</div>";
    html += "<div id='progressText'>Preparing update...</div>";
    html += "</div>";

    html += "<div class='status-card'>";
    html += "<h2>📝 Update Log</h2>";
    html += "<div class='log' id='updateLog'>";
    html += "System ready. Click 'Check for Updates' to begin.";
    html += "</div>";
    html += "</div>";
    html += "</div>";

    html += "<script>";
    html += "let logContent = 'System ready. Click Check for Updates to begin.\\n';";
    html += "function addToLog(message) {";
    html += "    logContent = new Date().toLocaleTimeString() + ' - ' + message + '\\n' + logContent;";
    html += "    document.getElementById('updateLog').innerText = logContent;";
    html += "}";
    html += "async function checkUpdate() {";
    html += "    addToLog('Checking for updates...');";
    html += "    document.getElementById('checkBtn').disabled = true;";
    html += "    try {";
    html += "        const response = await fetch('/check-update');";
    html += "        const data = await response.json();";
    html += "        document.getElementById('latestVersion').textContent = data.latestVersion;";
    html += "        document.getElementById('updateStatus').textContent = data.status;";
    html += "        if (data.updateAvailable) {";
    html += "            document.getElementById('installBtn').disabled = false;";
    html += "            document.getElementById('updateStatus').className = 'update-available';";
    html += "            addToLog('Update available! Version: ' + data.latestVersion);";
    html += "        } else {";
    html += "            document.getElementById('installBtn').disabled = true;";
    html += "            document.getElementById('updateStatus').className = 'update-not-available';";
    html += "            addToLog('Device is up to date.');";
    html += "        }";
    html += "    } catch (error) {";
    html += "        addToLog('Error checking for updates: ' + error);";
    html += "    }";
    html += "    document.getElementById('checkBtn').disabled = false;";
    html += "}";
    html += "async function installUpdate() {";
    html += "    if (!confirm('Are you sure you want to install the update?')) return;";
    html += "    addToLog('Starting update installation...');";
    html += "    document.getElementById('installBtn').disabled = true;";
    html += "    document.getElementById('checkBtn').disabled = true;";
    html += "    document.getElementById('progressSection').style.display = 'block';";
    html += "    try {";
    html += "        const response = await fetch('/install-update');";
    html += "        const reader = response.body.getReader();";
    html += "        const decoder = new TextDecoder();";
    html += "        while (true) {";
    html += "            const { value, done } = await reader.read();";
    html += "            if (done) break;";
    html += "            const chunk = decoder.decode(value);";
    html += "            const lines = chunk.split('\\\\n');";
    html += "            for (const line of lines) {";
    html += "                if (line.startsWith('PROGRESS:')) {";
    html += "                    const progress = line.split(':')[1];";
    html += "                    document.getElementById('updateProgress').style.width = progress + '%';";
    html += "                    document.getElementById('updateProgress').textContent = progress + '%';";
    html += "                    document.getElementById('progressText').textContent = 'Downloading: ' + progress + '%';";
    html += "                } else if (line.trim() !== '') {";
    html += "                    addToLog(line);";
    html += "                }";
    html += "            }";
    html += "        }";
    html += "    } catch (error) {";
    html += "        addToLog('Update failed: ' + error);";
    html += "    }";
    html += "}";
    html += "async function restartDevice() {";
    html += "    if (confirm('Are you sure you want to restart the device?')) {";
    html += "        addToLog('Restarting device...');";
    html += "        await fetch('/restart');";
    html += "        addToLog('Device restart command sent.');";
    html += "    }";
    html += "}";
    html += "setInterval(checkUpdate, 300000);"; // Auto-check every 5 minutes
    html += "window.onload = checkUpdate;";
    html += "</script>";
    html += "</body>";
    html += "</html>";
    
    server.send(200, "text/html", html);
  });

  // API endpoint to check for updates
  server.on("/check-update", HTTP_GET, []() {
    checkForUpdates();
    
    StaticJsonDocument<512> doc;
    doc["currentVersion"] = currentVersion;
    doc["latestVersion"] = latestVersion;
    doc["updateAvailable"] = updateAvailable;
    doc["status"] = updateStatus;
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
  });

  // API endpoint to install update
  server.on("/install-update", HTTP_GET, []() {
    server.sendHeader("Content-Type", "text/plain");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "");
    installUpdate();
  });

  // API endpoint to get update progress
  server.on("/update-progress", HTTP_GET, []() {
    String progress = String(updateProgress);
    server.send(200, "text/plain", progress);
  });

  // API endpoint to restart device
  server.on("/restart", HTTP_GET, []() {
    server.send(200, "text/plain", "Restarting device...");
    delay(1000);
    ESP.restart();
  });

  server.begin();
}

void checkForUpdates() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(versionURL);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        latestVersion = doc["version"].as<String>();
        
        Serial.println("Current version: " + currentVersion);
        Serial.println("Latest version: " + latestVersion);
        
        if (latestVersion != currentVersion) {
          updateAvailable = true;
          updateStatus = "Update available! Click 'Install Update' to install version " + latestVersion;
          Serial.println("Update available!");
        } else {
          updateAvailable = false;
          updateStatus = "Device is up to date";
          Serial.println("Device is up to date.");
        }
      } else {
        updateStatus = "Error parsing version info";
        Serial.println("JSON parsing error");
      }
    } else {
      updateStatus = "Failed to check for updates";
      Serial.println("Failed to check for updates. HTTP Code: " + String(httpCode));
    }
    
    http.end();
  } else {
    Serial.println("WiFi not connected");
    updateStatus = "WiFi not connected";
  }
}

void installUpdate() {
  updateInProgress = true;
  updateStatus = "Update in progress...";
  updateProgress = 0;
  
  Serial.println("Starting firmware update...");
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(firmwareURL);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();
      Serial.println("File size: " + String(contentLength));
      
      if (contentLength > 0) {
        bool canBegin = Update.begin(contentLength);
        
        if (canBegin) {
          Serial.println("Begin OTA update...");
          
          WiFiClient* stream = http.getStreamPtr();
          size_t written = 0;
          uint8_t buff[1024] = { 0 };
          
          while (http.connected() && written < contentLength) {
            size_t size = stream->available();
            
            if (size) {
              int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
              Update.write(buff, c);
              written += c;
              
              // Calculate and send progress
              updateProgress = (written * 100) / contentLength;
              String progressMsg = "PROGRESS:" + String(updateProgress) + "\n";
              server.sendContent(progressMsg);
              
              Serial.print("Progress: ");
              Serial.print(updateProgress);
              Serial.println("%");
            }
            delay(1);
          }
          
          if (Update.end()) {
            server.sendContent("OTA update complete!\n");
            Serial.println("OTA update complete!");
            
            if (Update.isFinished()) {
              server.sendContent("Update successfully completed. Rebooting...\n");
              Serial.println("Update successfully completed. Rebooting...");
              
              // Update current version
              currentVersion = latestVersion;
              updateStatus = "Update completed successfully";
              
              delay(2000);
              ESP.restart();
            } else {
              server.sendContent("Update not finished?\n");
              Serial.println("Update not finished?");
            }
          } else {
            String errorMsg = "Error Occurred: " + String(Update.getError()) + "\n";
            server.sendContent(errorMsg);
            Serial.println("Error Occurred: " + String(Update.getError()));
          }
        } else {
          server.sendContent("Not enough space to begin OTA\n");
          Serial.println("Not enough space to begin OTA");
        }
      } else {
        server.sendContent("Content-Length was 0\n");
        Serial.println("Content-Length was 0");
      }
    } else {
      String errorMsg = "Failed to download firmware. HTTP Code: " + String(httpCode) + "\n";
      server.sendContent(errorMsg);
      Serial.println("Failed to download firmware. HTTP Code: " + String(httpCode));
    }
    
    http.end();
  } else {
    server.sendContent("WiFi not connected\n");
    Serial.println("WiFi not connected");
  }
  
  updateAvailable = false;
  updateInProgress = false;
}
