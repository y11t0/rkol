#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>
#include <Arduino.h>

// WiFi bilgileri
const char *ssid = "WayFay";
const char *password = "abimbenim";

// Servo pinleri
#define SERVO1_PIN 5
#define SERVO2_PIN 4
#define SERVO3_PIN 0
#define SERVO4_PIN 2

#define SERVO_DELAY 15
#define SERVO_STEP 2
#define MIN_ANGLE 20
#define MAX_ANGLE 160

Servo servo1, servo2, servo3, servo4;
int currentPos1 = 90, currentPos2 = 90, currentPos3 = 90, currentPos4 = 20;
int targetPos1 = 90, targetPos2 = 90, targetPos3 = 90, targetPos4 = 20;
unsigned long lastMoveTime1 = 0, lastMoveTime2 = 0, lastMoveTime3 = 0, lastMoveTime4 = 0;

ESP8266WebServer server(80);
bool demoModeActive = false;

const char *html = R"rawliteral(
<!DOCTYPE html><html><head><title>Robot Kol Kontrol</title>
<style>
body { font-family: Arial; background: #1e1e2e; color: #cdd6f4; padding: 20px; }
.control-panel { background: #313244; padding: 20px; border-radius: 15px; }
.slider { width: 100%; height: 10px; background: #585b70; border-radius: 5px; outline: none; }
.slider::-webkit-slider-thumb { width: 20px; height: 20px; border-radius: 50%; background: #b4befe; cursor: pointer; }
button { padding: 12px 24px; margin: 10px; background: #f5c2e7; border: none; border-radius: 8px; color: #181825; }
button:hover { background: #f5e0dc; }
</style></head><body><h1>Robot Kol Kontrol</h1>
<div class="control-panel">
<h3>Govde (20-160)</h3><input type="range" min="20" max="160" value="90" class="slider" id="servo1">
<h3>Kol (20-160)</h3><input type="range" min="20" max="160" value="90" class="slider" id="servo2">
<h3>Dirsek (20-160)</h3><input type="range" min="20" max="160" value="90" class="slider" id="servo3">
<h3>Tutucu</h3>
<button onclick="setGripper(20)">Ac</button>
<button onclick="setGripper(160)">Kapat</button>
<div>
    <button onclick="resetAll()">Reset</button>
    <button onclick="startDemo()">Demo Başlat</button>
    <button onclick="stopDemo()">Demo Durdur</button>
</div></div>
<script>
let debounceTimer;
function updateServo(servoId, value) {
    clearTimeout(debounceTimer);
    debounceTimer = setTimeout(() => {
        fetch(`/set?servo=${servoId}&angle=${value}`);
    }, 20);
}
document.querySelectorAll('.slider').forEach(slider => {
    slider.addEventListener('input', () => {
        updateServo(slider.id, slider.value);
    });
});
function setGripper(angle) { updateServo('servo4', angle); }
function resetAll() {
    updateServo('servo1', 90);
    updateServo('servo2', 90);
    updateServo('servo3', 90);
    updateServo('servo4', 20);
}
function startDemo() { fetch('/startDemo'); }
function stopDemo() { fetch('/stopDemo'); }
</script></body></html>
)rawliteral";

void smoothMove(Servo &servo, int &currentPos, int targetPos, unsigned long &lastMoveTime) {
    targetPos = constrain(targetPos, MIN_ANGLE, MAX_ANGLE);
    if (millis() - lastMoveTime < SERVO_DELAY) return;
    if (currentPos != targetPos) {
        int step = (targetPos - currentPos) / 2;
        if (abs(step) < SERVO_STEP) step = (targetPos > currentPos) ? SERVO_STEP : -SERVO_STEP;
        currentPos += step;
        if (abs(targetPos - currentPos) < SERVO_STEP) currentPos = targetPos;
    }
    currentPos = constrain(currentPos, MIN_ANGLE, MAX_ANGLE);
    servo.write(currentPos);
    lastMoveTime = millis();
}

void resetAllServos() {
    currentPos1 = currentPos2 = currentPos3 = 90;
    currentPos4 = 20;
    targetPos1 = targetPos2 = targetPos3 = 90;
    targetPos4 = 20;
    servo1.write(currentPos1);
    servo2.write(currentPos2);
    servo3.write(currentPos3);
    servo4.write(currentPos4);
}

void startDemo() { demoModeActive = true; }
void stopDemo() { demoModeActive = false; resetAllServos(); }

void demoLoop() {
    static unsigned long lastDemoTime = 0;
    if (demoModeActive && millis() - lastDemoTime > 2000) {
        targetPos1 = random(MIN_ANGLE, MAX_ANGLE);
        targetPos2 = random(MIN_ANGLE, MAX_ANGLE);
        targetPos3 = random(MIN_ANGLE, MAX_ANGLE);
        targetPos4 = random(MIN_ANGLE, MAX_ANGLE);
        lastDemoTime = millis();
    }
}

void setup() {
    Serial.begin(9600);
    servo1.attach(SERVO1_PIN, 500, 2500);
    servo2.attach(SERVO2_PIN, 500, 2500);
    servo3.attach(SERVO3_PIN, 500, 2500);
    servo4.attach(SERVO4_PIN, 500, 2500);
    resetAllServos();

    WiFi.begin(ssid, password);
    Serial.print("WiFi bağlantısı: ");
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 20) {
        delay(500); Serial.print(".");
        retry++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nBağlantı başarılı! IP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nWiFi bağlantısı başarısız!");
    }

    server.on("/", []() { server.send(200, "text/html", html); });

    server.on("/set", []() {
        String servo = server.arg("servo");
        int angle = server.arg("angle").toInt();
        demoModeActive = false; // Web'den komut gelirse demo durmalı
        if (servo == "servo1") targetPos1 = angle;
        else if (servo == "servo2") targetPos2 = angle;
        else if (servo == "servo3") targetPos3 = angle;
        else if (servo == "servo4") targetPos4 = angle;
        server.send(200, "text/plain", "OK");
    });

    server.on("/startDemo", []() {
        startDemo();
        server.send(200, "text/plain", "Demo Başlatıldı");
    });

    server.on("/stopDemo", []() {
        stopDemo();
        server.send(200, "text/plain", "Demo Durduruldu");
    });

    server.begin();
}

void loop() {
    server.handleClient();
    smoothMove(servo1, currentPos1, targetPos1, lastMoveTime1);
    smoothMove(servo2, currentPos2, targetPos2, lastMoveTime2);
    smoothMove(servo3, currentPos3, targetPos3, lastMoveTime3);
    smoothMove(servo4, currentPos4, targetPos4, lastMoveTime4);
    demoLoop();
}
