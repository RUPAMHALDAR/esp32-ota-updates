#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ArduinoJson.h>

// WiFi Credentials
const char* ssid = "Vivo 9";
const char* password = "99999999";

// GitHub Configuration
const char* githubUsername = "RUPAMHALDAR";
const char* repoName = "esp32-ota-updates";
const char* branch = "main";

// Generated URLs
String firmwareURL = "https://raw.githubusercontent.com/" + String(githubUsername) + "/" + String(repoName) + "/" + String(branch) + "/firmware.bin";
String versionURL = "https://raw.githubusercontent.com/" + String(githubUsername) + "/" + String(repoName) + "/" + String(branch) + "/version.json";

// Web Server
WebServer server(80);

// Firmware Info
String currentVersion = "1.1.2";
bool updateAvailable = false;
String latestVersion = "";
String updateStatus = "System Ready";
bool updateInProgress = false;

// LED Configuration
const int ledPin = 2;  // Built-in LED pin
bool ledBlinking = false;
unsigned long previousBlinkMillis = 0;
const long blinkInterval = 500;

void setup() {
  Serial.begin(115200);
  
  // Initialize LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // Connect to WiFi
  connectToWiFi();
  
  // Start LED blinking
  ledBlinking = true;
  
  // Setup web server
  setupWebServer();
  
  // Check for updates on startup
  checkForUpdates();
  
  Serial.println("System started successfully!");
  Serial.print("Web interface: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
  blinkLED();
  delay(10);
}

void blinkLED() {
  if (ledBlinking) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousBlinkMillis >= blinkInterval) {
      previousBlinkMillis = currentMillis;
      digitalWrite(ledPin, !digitalRead(ledPin));
    }
  }
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
  // Main page
  server.on("/", HTTP_GET, []() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>ESP32 OTA Update System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body { 
            font-family: Arial, sans-serif; 
            max-width: 800px; 
            margin: 0 auto; 
            padding: 20px;
            background: #f5f5f5;
        }
        .container {
            background: white;
            padding: 20px;
            border-radius: 10px;
            box-shadow: 0 2px 10px rgba(0,0,0,0.1);
        }
        .card {
            background: #f8f9fa;
            padding: 20px;
            border-radius: 8px;
            margin: 15px 0;
            border-left: 4px solid #007bff;
        }
        .button {
            background: #007bff;
            border: none;
            color: white;
            padding: 12px 24px;
            margin: 5px;
            border-radius: 5px;
            cursor: pointer;
            font-size: 14px;
        }
        .button:hover {
            background: #0056b3;
        }
        .button-success {
            background: #28a745;
        }
        .button-success:hover {
            background: #1e7e34;
        }
        .button-danger {
            background: #dc3545;
        }
        .button-danger:hover {
            background: #c82333;
        }
        .button-warning {
            background: #ffc107;
            color: black;
        }
        .button-warning:hover {
            background: #e0a800;
        }
        .button:disabled {
            background: #6c757d;
            cursor: not-allowed;
        }
        .status {
            padding: 5px 10px;
            border-radius: 4px;
            font-weight: bold;
            display: inline-block;
        }
        .status-on {
            background: #d4edda;
            color: #155724;
        }
        .status-off {
            background: #f8d7da;
            color: #721c24;
        }
        .status-update {
            background: #fff3cd;
            color: #856404;
        }
        .info-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin: 10px 0;
        }
        .info-item {
            background: white;
            padding: 10px;
            border-radius: 5px;
            border: 1px solid #dee2e6;
        }
        .progress {
            width: 100%;
            background: #e9ecef;
            border-radius: 5px;
            margin: 10px 0;
            display: none;
        }
        .progress-bar {
            width: 0%;
            height: 20px;
            background: #28a745;
            border-radius: 5px;
            transition: width 0.3s;
            text-align: center;
            color: white;
            line-height: 20px;
        }
        .log {
            background: #1a1a1a;
            color: #00ff00;
            padding: 15px;
            border-radius: 5px;
            height: 200px;
            overflow-y: auto;
            font-family: monospace;
            font-size: 12px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESP32 OTA Update System</h1>
        
        <!-- Device Info Card -->
        <div class="card">
            <h2>Device Information</h2>
            <div class="info-grid">
                <div class="info-item">
                    <strong>IP Address:</strong><br>
                    <span id="ipAddress">)</span>
                </div>
                <div class="info-item">
                    <strong>WiFi Strength:</strong><br>
                    <span id="wifiStrength">0 dBm</span>
                </div>
                <div class="info-item">
                    <strong>Current Version:</strong><br>
                    <span id="currentVersion">1.0.0</span>
                </div>
                <div class="info-item">
                    <strong>Latest Version:</strong><br>
                    <span id="latestVersion">Checking...</span>
                </div>
            </div>
            <div class="info-item">
                <strong>Update Status:</strong><br>
                <span id="updateStatus" class="status status-off">System Ready</span>
            </div>
        </div>

        <!-- OTA Update Card -->
        <div class="card">
            <h2>OTA Update Control</h2>
            <button class="button" onclick="checkUpdate()" id="checkBtn">Check for Updates</button>
            <button class="button button-success" onclick="installUpdate()" id="installBtn" disabled>Install Update</button>
            <button class="button button-warning" onclick="restartDevice()">Restart Device</button>
            
            <div class="progress" id="progressSection">
                <div class="progress-bar" id="progressBar">0%</div>
            </div>
            <div id="progressText" style="text-align: center; margin: 10px 0;"></div>
        </div>

        <!-- System Control Card -->
        <div class="card">
            <h2>System Control</h2>
            <div class="info-grid">
                <div class="info-item">
                    <strong>LED Status:</strong><br>
                    <span id="ledStatus" class="status status-off">OFF</span>
                </div>
                <div class="info-item">
                    <strong>Uptime:</strong><br>
                    <span id="uptime">0</span> seconds
                </div>
                <div class="info-item">
                    <strong>Free Memory:</strong><br>
                    <span id="freeMemory">0</span> KB
                </div>
                <div class="info-item">
                    <strong>Total Memory:</strong><br>
                    <span id="totalMemory">0</span> KB
                </div>
            </div>
            <button class="button button-success" onclick="controlLED('start')">Start LED Blink</button>
            <button class="button button-danger" onclick="controlLED('stop')">Stop LED Blink</button>
            <button class="button" onclick="updateSystemInfo()">Refresh System Info</button>
        </div>

        <!-- System Log -->
        <div class="card">
            <h2>System Log</h2>
            <div class="log" id="log">
                System started...
            </div>
        </div>
    </div>

    <script>
        function addLog(message) {
            const log = document.getElementById('log');
            const time = new Date().toLocaleTimeString();
            log.innerHTML = '[' + time + '] ' + message + '<br>' + log.innerHTML;
        }

        // LED Control
        async function controlLED(command) {
            addLog('LED Command: ' + command);
            try {
                const response = await fetch('/led', {
                    method: 'POST',
                    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                    body: 'command=' + command
                });
                const result = await response.json();
                addLog('LED: ' + result.status);
                
                const ledStatus = document.getElementById('ledStatus');
                if (command === 'start') {
                    ledStatus.textContent = 'BLINKING';
                    ledStatus.className = 'status status-on';
                } else {
                    ledStatus.textContent = 'OFF';
                    ledStatus.className = 'status status-off';
                }
            } catch (error) {
                addLog('LED Error: ' + error.message);
            }
        }

        // System Info
        async function updateSystemInfo() {
            try {
                const response = await fetch('/system');
                const data = await response.json();
                
                document.getElementById('uptime').textContent = data.uptime;
                document.getElementById('freeMemory').textContent = data.freeMemory;
                document.getElementById('totalMemory').textContent = data.totalMemory;
                document.getElementById('wifiStrength').textContent = data.wifiStrength + ' dBm';
                
                addLog('System info updated');
            } catch (error) {
                addLog('System info error: ' + error.message);
            }
        }

        // OTA Update Functions
        async function checkUpdate() {
            const checkBtn = document.getElementById('checkBtn');
            checkBtn.disabled = true;
            checkBtn.textContent = 'Checking...';
            addLog('Checking for updates...');
            
            try {
                const response = await fetch('/check-update');
                const data = await response.json();
                
                document.getElementById('latestVersion').textContent = data.latestVersion;
                document.getElementById('currentVersion').textContent = data.currentVersion;
                
                const installBtn = document.getElementById('installBtn');
                const statusElem = document.getElementById('updateStatus');
                
                if (data.updateAvailable) {
                    installBtn.disabled = false;
                    statusElem.textContent = 'Update Available: ' + data.latestVersion;
                    statusElem.className = 'status status-on';
                    addLog('Update available: ' + data.latestVersion);
                } else {
                    installBtn.disabled = true;
                    statusElem.textContent = 'System Up to Date';
                    statusElem.className = 'status status-off';
                    addLog('System is up to date');
                }
            } catch (error) {
                addLog('Update check error: ' + error.message);
            } finally {
                checkBtn.disabled = false;
                checkBtn.textContent = 'Check for Updates';
            }
        }

        async function installUpdate() {
            if (!confirm('Install firmware update? Device will restart automatically.')) return;
            
            const installBtn = document.getElementById('installBtn');
            const checkBtn = document.getElementById('checkBtn');
            const progressSection = document.getElementById('progressSection');
            const progressBar = document.getElementById('progressBar');
            const progressText = document.getElementById('progressText');
            
            installBtn.disabled = true;
            checkBtn.disabled = true;
            progressSection.style.display = 'block';
            
            addLog('Starting firmware update...');
            
            try {
                const response = await fetch('/install-update');
                const reader = response.body.getReader();
                const decoder = new TextDecoder();
                
                while (true) {
                    const {value, done} = await reader.read();
                    if (done) break;
                    
                    const chunk = decoder.decode(value);
                    const lines = chunk.split('\n');
                    
                    for (const line of lines) {
                        if (line.startsWith('PROGRESS:')) {
                            const progress = line.split(':')[1];
                            progressBar.style.width = progress + '%';
                            progressBar.textContent = progress + '%';
                            progressText.textContent = 'Downloading: ' + progress + '%';
                        } else if (line.trim()) {
                            addLog(line);
                        }
                    }
                }
            } catch (error) {
                addLog('Update failed: ' + error.message);
            }
        }

        async function restartDevice() {
            if (confirm('Restart device?')) {
                addLog('Restarting device...');
                await fetch('/restart');
            }
        }

        // Initialize
        window.onload = function() {
            document.getElementById('ipAddress').textContent = window.location.hostname;
            updateSystemInfo();
            checkUpdate();
            controlLED('start');
            addLog('Web interface loaded');
            
            // Auto-update system info every 5 seconds
            setInterval(updateSystemInfo, 5000);
            
            // Auto-check for updates every 2 minutes
            setInterval(checkUpdate, 120000);
        };
    </script>
</body>
</html>
    )rawliteral";
    
    server.send(200, "text/html", html);
  });

  // LED control endpoint
  server.on("/led", HTTP_POST, []() {
    if (server.hasArg("command")) {
      String command = server.arg("command");
      Serial.println("LED command: " + command);
      
      if (command == "start") {
        ledBlinking = true;
        server.send(200, "application/json", "{\"status\":\"LED blinking started\"}");
      } else if (command == "stop") {
        ledBlinking = false;
        digitalWrite(ledPin, LOW);
        server.send(200, "application/json", "{\"status\":\"LED blinking stopped\"}");
      } else {
        server.send(400, "application/json", "{\"error\":\"Invalid command\"}");
      }
    } else {
      server.send(400, "application/json", "{\"error\":\"No command\"}");
    }
  });

  // System info endpoint
  server.on("/system", HTTP_GET, []() {
    String response = "{";
    response += "\"uptime\":" + String(millis() / 1000) + ",";
    response += "\"freeMemory\":" + String(ESP.getFreeHeap() / 1024) + ",";
    response += "\"totalMemory\":" + String(ESP.getHeapSize() / 1024) + ",";
    response += "\"wifiStrength\":" + String(WiFi.RSSI());
    response += "}";
    
    server.send(200, "application/json", response);
  });

  // Check for updates endpoint
  server.on("/check-update", HTTP_GET, []() {
    checkForUpdates();
    
    String response = "{";
    response += "\"currentVersion\":\"" + currentVersion + "\",";
    response += "\"latestVersion\":\"" + latestVersion + "\",";
    response += "\"updateAvailable\":" + String(updateAvailable ? "true" : "false");
    response += "}";
    
    server.send(200, "application/json", response);
  });

  // Install update endpoint
  server.on("/install-update", HTTP_GET, []() {
    server.sendHeader("Content-Type", "text/plain");
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "text/plain", "");
    installUpdate();
  });

  // Restart endpoint
  server.on("/restart", HTTP_GET, []() {
    server.send(200, "text/plain", "Restarting...");
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
      Serial.println("Version info: " + payload);
      
      StaticJsonDocument<512> doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        latestVersion = doc["version"].as<String>();
        
        if (latestVersion != currentVersion) {
          updateAvailable = true;
          updateStatus = "Update Available: " + latestVersion;
          Serial.println("Update available: " + latestVersion);
        } else {
          updateAvailable = false;
          updateStatus = "System Up to Date";
          Serial.println("System is up to date");
        }
      } else {
        updateStatus = "Error parsing version info";
        Serial.println("JSON parse error");
      }
    } else {
      updateStatus = "Failed to check updates";
      Serial.println("HTTP error: " + String(httpCode));
    }
    
    http.end();
  } else {
    updateStatus = "WiFi not connected";
    Serial.println("WiFi not connected");
  }
}

void installUpdate() {
  updateInProgress = true;
  updateStatus = "Update in progress...";
  
  Serial.println("Starting OTA update...");
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(firmwareURL);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      int contentLength = http.getSize();
      Serial.println("Firmware size: " + String(contentLength) + " bytes");
      
      if (contentLength > 0) {
        bool canBegin = Update.begin(contentLength);
        
        if (canBegin) {
          WiFiClient* stream = http.getStreamPtr();
          size_t written = 0;
          uint8_t buff[1024] = { 0 };
          
          while (http.connected() && written < contentLength) {
            size_t size = stream->available();
            
            if (size) {
              int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
              Update.write(buff, c);
              written += c;
              
              int progress = (written * 100) / contentLength;
              String progressMsg = "PROGRESS:" + String(progress);
              server.sendContent(progressMsg + "\n");
              
              Serial.println("Progress: " + String(progress) + "%");
            }
            delay(1);
          }
          
          if (Update.end()) {
            server.sendContent("Update complete! Rebooting...\n");
            Serial.println("Update complete");
            
            if (Update.isFinished()) {
              currentVersion = latestVersion;
              delay(2000);
              ESP.restart();
            }
          } else {
            server.sendContent("Update failed: " + String(Update.getError()) + "\n");
            Serial.println("Update failed");
          }
        } else {
          server.sendContent("Cannot begin update\n");
          Serial.println("Cannot begin update");
        }
      } else {
        server.sendContent("Invalid file size\n");
        Serial.println("Invalid file size");
      }
    } else {
      server.sendContent("Download failed: " + String(httpCode) + "\n");
      Serial.println("Download failed: " + String(httpCode));
    }
    
    http.end();
  } else {
    server.sendContent("WiFi not connected\n");
    Serial.println("WiFi not connected");
  }
  
  updateInProgress = false;
}