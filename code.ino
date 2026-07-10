#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------------- HARDWARE MAP ----------------
#define DHTPIN 15
#define DHTTYPE DHT11
#define FLAME_PIN 34
#define BUZZER_PIN 4
#define TRIG_PIN 5
#define ECHO_PIN 18
#define RST_PIN 22 
#define SS_PIN 21  

const int motorPins[] = {12, 13, 14, 27};

// ---------------- NETWORK CONFIG ----------------
const char* ssid = "Divyansh Garg";
const char* password = "divyansh";
const char* camera_url = "http://192.168.1.10:8080/video";

// ---------------- SYSTEM INSTANCES ----------------
WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ---------------- GLOBAL BALANCES ----------------
bool isLocked = true;
bool loggedIn = false;
unsigned long lastActivity = 0;
unsigned long lastSensorRead = 0;

// Dynamic GPS Telemetry Simulation
float lat = 28.6139;
float lon = 77.2090;
String systemState = "LOCKED";

// ---------------- SECURITY INFRASTRUCTURE ----------------
String sessionToken = "UNAUTHORIZED";

String generateToken() {
  return String(random(100000, 999999));
}

bool isAuth() {
  return server.hasArg("token") && server.arg("token") == sessionToken && !isLocked;
}

// ---------------- UPGRADED CYBERPUNK HUD INTERFACE ----------------
String page() {
  return R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SENTINEL-X: Tactical Command</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <link rel="stylesheet" href="https://unpkg.com/leaflet/dist/leaflet.css"/>
    <script src="https://unpkg.com/leaflet/dist/leaflet.js"></script>
    <style>
        body { background: radial-gradient(circle at center, #0f172a, #020617); font-family: monospace; }
        .glass { background: rgba(30, 41, 59, 0.4); backdrop-filter: blur(12px); border: 1px solid rgba(56, 189, 248, 0.2); box-shadow: 0 0 20px rgba(56, 189, 248, 0.1); }
        .btn-ctrl { background: rgba(56, 189, 248, 0.1); border: 1px solid rgba(56, 189, 248, 0.4); transition: all 0.2s; }
        .btn-ctrl:active { transform: scale(0.95); background: rgba(56, 189, 248, 0.4); }
        .scrollbar-none::-webkit-scrollbar { display: none; }
    </style>
</head>
<body class="text-slate-100 min-h-screen p-4 select-none">

    <header class="max-w-6xl mx-auto mb-6 flex justify-between items-center border-b border-sky-500/30 pb-4">
        <div>
            <h1 class="text-3xl font-extrabold tracking-wider text-sky-400 drop-shadow-[0_0_8px_rgba(56,189,248,0.5)]">SENTINEL-X HUD</h1>
            <p class="text-xs text-slate-400 mt-1">Telemetry Status Loop: Secure Connection Established</p>
        </div>
        <div id="lock-state" class="px-4 py-2 rounded font-bold transition-all border text-rose-400 border-rose-500/30 bg-rose-500/10">
            SYSTEM LOCKED
        </div>
    </header>

    <main class="max-w-6xl mx-auto grid grid-cols-1 lg:grid-cols-3 gap-6">
        <div class="glass rounded-xl p-4 flex flex-col justify-between lg:col-span-2">
            <h2 class="text-sm font-bold uppercase tracking-widest text-sky-400 mb-2">// OPTICAL RECONNAISSANCE STREAM</h2>
            <div class="w-full bg-black/40 rounded-lg overflow-hidden border border-slate-800 flex items-center justify-center h-80">
                <img src=")rawliteral" + String(camera_url) + R"rawliteral(" class="w-full h-full object-cover" onerror="this.style.display='none'; this.nextElementSibling.style.display='flex';">
                <div class="hidden flex-col items-center space-y-2 text-rose-400 font-bold">
                    <span class="animate-pulse">⚠️ VIDEO STREAM OFFLINE</span>
                    <span class="text-xs text-slate-500 font-normal">Check camera gateway at )rawliteral" + String(camera_url) + R"rawliteral("</span>
                </div>
            </div>
        </div>

        <div class="glass rounded-xl p-6 flex flex-col items-center justify-center h-96 lg:h-auto">
            <h2 class="text-sm font-bold uppercase tracking-widest text-sky-400 mb-6 border-b border-sky-500/20 pb-2 w-full">// MANEUVER OVERRIDES</h2>
            <div class="grid grid-cols-3 gap-3 w-44 my-auto">
                <div></div>
                <button onmousedown="cmd('up')" onmouseup="cmd('stop')" class="btn-ctrl p-4 rounded-xl font-bold text-xl text-sky-400">▲</button>
                <div></div>
                <button onmousedown="cmd('left')" onmouseup="cmd('stop')" class="btn-ctrl p-4 rounded-xl font-bold text-xl text-sky-400">◀</button>
                <button onclick="cmd('stop')" class="bg-rose-500/20 hover:bg-rose-500/40 border border-rose-500/40 p-4 rounded-xl text-rose-400 font-bold">■</button>
                <button onmousedown="cmd('right')" onmouseup="cmd('stop')" class="btn-ctrl p-4 rounded-xl font-bold text-xl text-sky-400">▶</button>
                <div></div>
                <button onmousedown="cmd('down')" onmouseup="cmd('stop')" class="btn-ctrl p-4 rounded-xl font-bold text-xl text-sky-400">▼</button>
                <div></div>
            </div>
        </div>

        <div class="glass rounded-xl p-4 lg:col-span-2">
            <h2 class="text-sm font-bold uppercase tracking-widest text-sky-400 mb-2">// GEOLOCATION deployment TRACKING</h2>
            <div id="map" class="w-full h-64 rounded-lg border border-slate-800 shadow-inner"></div>
        </div>

        <div class="glass rounded-xl p-6 flex flex-col justify-between">
            <h2 class="text-sm font-bold uppercase tracking-widest text-sky-400 mb-4 border-b border-sky-500/20 pb-2">// EDGE LOGISTICS</h2>
            <div class="grid grid-cols-2 gap-3 flex-grow my-auto">
                <div class="p-3 bg-slate-900/50 rounded-lg border border-slate-800">
                    <span class="text-[10px] text-slate-400 uppercase block">Temperature</span>
                    <span id="t" class="text-xl font-bold text-sky-300">--</span><span class="text-xs text-sky-400">°C</span>
                </div>
                <div class="p-3 bg-slate-900/50 rounded-lg border border-slate-800">
                    <span class="text-[10px] text-slate-400 uppercase block">Humidity</span>
                    <span id="h" class="text-xl font-bold text-sky-300">--</span><span class="text-xs text-sky-400">%</span>
                </div>
                <div class="p-3 bg-slate-900/50 rounded-lg border border-slate-800">
                    <span class="text-[10px] text-slate-400 uppercase block">Thermal Array</span>
                    <span id="f" class="text-sm font-bold text-emerald-400 tracking-wider">SCANNING</span>
                </div>
                <div class="p-3 bg-slate-900/50 rounded-lg border border-slate-800">
                    <span class="text-[10px] text-slate-400 uppercase block">Signal Matrix</span>
                    <span id="r" class="text-xl font-bold text-sky-300">--</span><span class="text-xs text-sky-400"> dBm</span>
                </div>
            </div>
            <div class="mt-4 p-3 bg-black/40 rounded-lg border border-slate-800 text-center">
                <span class="text-[10px] text-slate-500 block uppercase mb-1">System State Array</span>
                <span id="s" class="text-sm font-bold tracking-widest text-sky-400">INITIALIZING</span>
            </div>
        </div>
    </main>

    <script>
        let token = localStorage.getItem("session_token") || "";

        function cmd(c) {
            fetch('/cmd?c=' + c + '&token=' + token).catch(e => console.error(e));
        }

        let map = L.map('map', { attributionControl: false }).setView([28.6139, 77.2090], 17);
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
        let marker = L.marker([28.6139, 77.2090]).addTo(map);

        setInterval(() => {
            fetch('/tele?token=' + token)
            .then(r => {
                if (r.status === 403) { window.location.reload(); }
                return r.json();
            })
            .then(d => {
                document.getElementById('t').innerText = d.t.toFixed(1);
                document.getElementById('h').innerText = d.h.toFixed(0);
                document.getElementById('r').innerText = d.r;
                
                const sBox = document.getElementById('s');
                sBox.innerText = d.s;
                
                const fBox = document.getElementById('f');
                if(d.f < 400) {
                    fBox.innerText = "⚠️ THERMAL FLAME";
                    fBox.className = "text-sm font-bold text-rose-400 animate-pulse";
                } else {
                    fBox.innerText = "NORMAL";
                    fBox.className = "text-sm font-bold text-emerald-400";
                }

                const lBox = document.getElementById('lock-state');
                if (d.s === "LOCKED") {
                    lBox.innerText = "SYSTEM LOCKED";
                    lBox.className = "px-4 py-2 rounded font-bold text-rose-400 border border-rose-500/30 bg-rose-500/10";
                } else {
                    lBox.innerText = "AUTH ACCESS PASS";
                    lBox.className = "px-4 py-2 rounded font-bold text-emerald-400 border border-emerald-500/30 bg-emerald-500/10";
                }

                marker.setLatLng([d.lat, d.lon]);
                map.panTo([d.lat, d.lon]);
            }).catch(err => {
                document.getElementById('s').innerText = "DISCONNECTED";
            });
        }, 800);
    </script>
</body>
</html>
  )rawliteral";
}

// ---------------- HANDSHAKE EXCHANGE TARGET ----------------
String authBridgePage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>SENTINEL ACCESS BRIDGE</title>
    <script>
        // Extract parameters embedded by system core, cache locally, and clear authentication parameters from history
        const urlParams = new URLSearchParams(window.location.search);
        const token = urlParams.get('token');
        if (token) {
            localStorage.setItem("session_token", token);
            window.location.href = "/";
        }
    </script>
</head>
<body style="background:#020617;color:#38bdf8;font-family:monospace;text-align:center;padding-top:20%;">
    <h3>GENERATING SECURE HARDWARE PASS BRIDGE...</h3>
</body>
</html>
  )rawliteral";
}

// ---------------- POSITION SIMULATION MODULE ----------------
void updatePos(char cmd){
  float step = 0.00002; // Enhanced stepping footprint precision
  if(cmd=='F') lat += step;
  if(cmd=='B') lat -= step;
  if(cmd=='L') lon -= step;
  if(cmd=='R') lon += step;
}

void move(int a, int b, int c, int d, char cmd){
  if(isLocked) return;

  digitalWrite(motorPins[0], a);
  digitalWrite(motorPins[1], b);
  digitalWrite(motorPins[2], c);
  digitalWrite(motorPins[3], d);

  updatePos(cmd);
  lastActivity = millis();
}

// ---------------- PLATFORM RUNTIME INITIALIZATION ----------------
void setup(){
  Serial.begin(115200);
  WiFi.softAP(ssid, password);

  dht.begin();
  lcd.init();
  lcd.backlight();

  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  for(int i=0; i<4; i++) pinMode(motorPins[i], OUTPUT);

  // Main Access Landing Page Entrypoint
  server.on("/", [](){
    if(!loggedIn || isLocked){
      server.send(403, "text/plain", "ACCESS DENIED: CONTACT COMMAND LAYER MODULE VIA RFID TOKEN");
      return;
    }
    server.send(200, "text/html", page());
  });

  // Secure Handshake Endpoint Callback
  server.on("/handshake", [](){
    server.send(200, "text/html", authBridgePage());
  });

  // Navigation Drive Endpoints
  server.on("/cmd", [](){
    if(!isAuth()) { server.send(403, "text/plain", "UNAUTHORIZED"); return; }

    String c = server.arg("c");
    if(systemState != "OBSTACLE" && systemState != "FIRE") {
      if(c=="up")        { move(HIGH, LOW, HIGH, LOW, 'F');  systemState = "PROPULSION: FWD"; }
      else if(c=="down") { move(LOW, HIGH, LOW, HIGH, 'B');  systemState = "PROPULSION: REV"; }
      else if(c=="left") { move(LOW, HIGH, HIGH, LOW, 'L');  systemState = "STEERING: LEFT"; }
      else if(c=="right"){ move(HIGH, LOW, LOW, HIGH, 'R');  systemState = "STEERING: RIGHT"; }
      else               { move(0, 0, 0, 0, 'S');             systemState = "NOMINAL HOLD"; }
    } else {
      move(0, 0, 0, 0, 'S'); // Safety containment clamp execution
    }
    server.send(200, "text/plain", "OK");
  });

  // Async Telemetry Endpoints
  server.on("/tele", [](){
    if(!isAuth()) { server.send(403, "text/plain", "UNAUTHORIZED"); return; }

    float tVal = dht.readTemperature();
    float hVal = dht.readHumidity();
    if (isnan(tVal)) tVal = 0.0;
    if (isnan(hVal)) hVal = 0.0;

    String json="{";
    json+="\"t\":"+String(tVal,1)+",";
    json+="\"h\":"+String(hVal,1)+",";
    json+="\"lat\":"+String(lat,6)+",";
    json+="\"lon\":"+String(lon,6)+",";
    json+="\"f\":"+String(analogRead(FLAME_PIN))+",";
    json+="\"s\":\""+systemState+"\",";
    json+="\"r\":"+String(WiFi.RSSI());
    json       +="}";
    server.send(200, "application/json", json);
  });

  server.begin();
  lcd.print("SYSTEM: LOCKED");
}

// ---------------- HIGH FREQUENCY RUNTIME LOOP ----------------
void loop(){
  server.handleClient();

  // Non-blocking high frequency environmental background sensor read intervals (Every 150ms)
  if (millis() - lastSensorRead > 150) {
    lastSensorRead = millis();

    // 1. Fire Protection Diagnostics Execution Block
    int flameRaw = analogRead(FLAME_PIN);
    if(flameRaw < 400){
      move(0, 0, 0, 0, 'S');
      systemState = "CRIT: FIRE DETECT";
      digitalWrite(BUZZER_PIN, HIGH);
      lcd.setCursor(0, 1);
      lcd.print("HAZARD: FIRE!  ");
    } else {
      digitalWrite(BUZZER_PIN, LOW);
      
      // 2. Ultrasonic Safety Lock Evasion Sweep Execution Block
      digitalWrite(TRIG_PIN, LOW);   delayMicroseconds(2);
      digitalWrite(TRIG_PIN, HIGH);  delayMicroseconds(10);
      digitalWrite(TRIG_PIN, LOW);
      
      long pulseDuration = pulseIn(ECHO_PIN, HIGH, 30000); // Fast 30ms timeout loop window limits
      long calcDistance = (pulseDuration > 0) ? (pulseDuration * 0.034 / 2) : 999;

      if(!isLocked && calcDistance < 15 && calcDistance > 0){
        move(0, 0, 0, 0, 'S');
        systemState = "CRIT: OBSTACLE";
        lcd.setCursor(0, 1);
        lcd.print("LOCK: DETECTED  ");
      } else if (!isLocked && systemState.startsWith("CRIT")) {
        systemState = "NOMINAL HOLD";
        lcd.setCursor(0, 1);
        lcd.print("PATH CLEAR      ");
      }
    }
  }

  // 3. RFID Authentication Processing Block
  if(isLocked && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()){
    isLocked = false;
    loggedIn = true;
    sessionToken = generateToken();

    lcd.clear();
    lcd.print("ACCESS CONFIRMED");
    lcd.setCursor(0, 1);
    lcd.print("IP: 192.168.4.1 ");
    
    // Explicit serial transmission mapping key for logging monitoring setups
    Serial.println("====== NEW RFID TOKEN GRANTEDCODE ======");
    Serial.print("TOKEN LINK BRIDGE DIRECTORY: http://192.168.4.1/handshake?token=");
    Serial.println(sessionToken);
    Serial.println("=========================================");

    systemState = "NOMINAL HOLD";
    lastActivity = millis();
    mfrc522.PICC_HaltA(); // Halt execution scan state pipelines safely
  }

  // 4. Autonomous System Security Auto-Lock Timer Out Block
  if(!isLocked && (millis() - lastActivity > 120000)){
    isLocked = true;
    loggedIn = false;
    sessionToken = "EXPIRED_SESSION";
    move(0, 0, 0, 0, 'S');
    systemState = "LOCKED";
    lcd.clear();
    lcd.print("TIMEOUT ENFORCED");
    lcd.setCursor(0, 1);
    lcd.print("SYSTEM: LOCKED  ");
  }
}