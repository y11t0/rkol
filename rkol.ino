#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// WiFi bilgileri
const char *ssid = "ssid";
const char *password = "password";

// Logo bilgileri
const char *robotArmLogo = R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="#cdd6f4"><path d="M12 2L2 7l10 5 10-5-10-5zM2 17l10 5 10-5V12L2 17z"/></svg>)rawliteral";
const int logoWidth = 60;
const int logoHeight = 60;

// Statik IP ayarlari
IPAddress staticIP(192, 168, 1, 31); // Denenecek statik IP
IPAddress gateway(192, 168, 1, 1);   // Gateway IP
IPAddress subnet(255, 255, 255, 0);  // Subnet mask
IPAddress dns(8, 8, 8, 8);           // DNS sunucusu

// Servo motor pinleri / ESP8266 için ayarlanmıştır
#define SERVO1_PIN 5 // GPIO5 (D1)
#define SERVO2_PIN 4 // GPIO4 (D2)
#define SERVO3_PIN 0 // GPIO0 (D3)
#define SERVO4_PIN 2 // GPIO2 (D4)

// Servo ayarları
#define SERVO_DELAY 15 // Her adım arası bekleme süresi (ms)
#define SERVO_STEP 2   // Her adımda hareket edilecek açı
#define MIN_ANGLE 20   // Minimum açı
#define MAX_ANGLE 160  // Maksimum açı

// Servo nesneleri
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// Mevcut servo pozisyonları
int currentPos1 = 90;
int currentPos2 = 90;
int currentPos3 = 90;
int currentPos4 = 20; // Gripper başlangıçta kapalı

// Hedef servo pozisyonları
int targetPos1 = 90;
int targetPos2 = 90;
int targetPos3 = 90;
int targetPos4 = 20; // Gripper başlangıçta kapalı

// Servo hareket zamanlayıcıları
unsigned long lastMoveTime1 = 0;
unsigned long lastMoveTime2 = 0;
unsigned long lastMoveTime3 = 0;
unsigned long lastMoveTime4 = 0;

// Web sunucusu
ESP8266WebServer server(80);

// HTML sayfası
const char *html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Robot Kol Kontrol</title>
    <style>
        :root {
            --bg: #1e1e2e;
            --text: #cdd6f4;
            --subtext1: #bac2de;
            --subtext0: #a6adc8;
            --surface2: #585b70;
            --surface1: #45475a;
            --surface0: #313244;
            --base: #181825;
            --mantle: #181825;
            --crust: #11111b;
            --blue: #89b4fa;
            --lavender: #b4befe;
            --sapphire: #74c7ec;
            --sky: #89dceb;
            --teal: #94e2d5;
            --green: #a6e3a1;
            --yellow: #f9e2af;
            --peach: #fab387;
            --maroon: #eba0ac;
            --red: #f38ba8;
            --mauve: #cba6f7;
            --pink: #f5c2e7;
            --flamingo: #f2cdcd;
            --rosewater: #f5e0dc;
        }

        body {
            font-family: 'Inter', Arial, sans-serif;
            background-color: var(--bg);
            color: var(--text);
            margin: 0;
            padding: 20px;
            min-height: 100vh;
        }

        .header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 20px;
            background-color: var(--surface0);
            border-radius: 15px;
            margin-bottom: 30px;
        }

        .logo {
            width: 100px;
            height: 100px;
            background-color: var(--surface1);
            border-radius: 10px;
            display: flex;
            align-items: center;
            justify-content: center;
            color: var(--subtext0);
            font-size: 14px;
        }

        .title {
            color: var(--lavender);
            text-align: center;
            margin: 0;
        }

        .control-panel {
            background-color: var(--surface0);
            border-radius: 15px;
            padding: 20px;
            margin-bottom: 20px;
        }

        .slider-container {
            margin: 20px 0;
            padding: 15px;
            background-color: var(--surface1);
            border-radius: 10px;
        }

        .slider-container h3 {
            color: var(--blue);
            margin-top: 0;
        }

        .slider {
            width: 100%;
            height: 10px;
            -webkit-appearance: none;
            background: var(--surface2);
            border-radius: 5px;
            outline: none;
        }

        .slider::-webkit-slider-thumb {
            -webkit-appearance: none;
            appearance: none;
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--mauve);
            cursor: pointer;
        }

        .slider::-moz-range-thumb {
            width: 20px;
            height: 20px;
            border-radius: 50%;
            background: var(--mauve);
            cursor: pointer;
        }

        .value {
            display: inline-block;
            width: 50px;
            text-align: center;
            color: var(--yellow);
            font-weight: bold;
        }

        .gripper-buttons {
            margin: 20px 0;
            text-align: center;
        }

        .reset-button {
            margin: 20px 0;
            text-align: center;
        }

        button {
            padding: 12px 24px;
            margin: 0 10px;
            font-size: 16px;
            cursor: pointer;
            border: none;
            border-radius: 8px;
            background-color: var(--mauve);
            color: var(--base);
            font-weight: bold;
            transition: all 0.3s ease;
        }

        button:hover {
            background-color: var(--pink);
            transform: translateY(-2px);
        }

        .reset-btn {
            background-color: var(--red);
        }

        .reset-btn:hover {
            background-color: var(--maroon);
        }

        @media (max-width: 768px) {
            .header {
                flex-direction: column;
                gap: 20px;
            }
            
            .logo {
                width: 80px;
                height: 80px;
            }
        }
    </style>
</head>
<body>
    <div class="header">
        <div class="logo">
            <img src="data:image/svg+xml;utf8,)rawliteral" +
                   String(robotArmLogo) + R"rawliteral(" alt="Robot Arm" style="width: )rawliteral" + String(logoWidth) + R"rawliteral(px; height: )rawliteral" + String(logoHeight) + R"rawliteral(px;">
        </div>
        <h1 class="title">Robot Kol Kontrol</h1>
        <div class="logo">
            <img src="data:image/svg+xml;utf8,)rawliteral" +
                   String(robotArmLogo) + R"rawliteral(" alt="Robot Arm" style="width: )rawliteral" + String(logoWidth) + R"rawliteral(px; height: )rawliteral" + String(logoHeight) + R"rawliteral(px;">
        </div>
    </div>

    <div class="control-panel">
        <div class="slider-container">
            <h3>Govde (20-160)</h3>
            <input type="range" min="20" max="160" value="90" class="slider" id="servo1">
            <span class="value" id="servo1Value">90</span>
        </div>
        <div class="slider-container">
            <h3>Kol (20-160)</h3>
            <input type="range" min="20" max="160" value="90" class="slider" id="servo2">
            <span class="value" id="servo2Value">90</span>
        </div>
        <div class="slider-container">
            <h3>Dirsek (20-160)</h3>
            <input type="range" min="20" max="160" value="90" class="slider" id="servo3">
            <span class="value" id="servo3Value">90</span>
        </div>
        <div class="gripper-buttons">
            <h3 style="color: var(--green)">Tutucu Kontrol</h3>
            <button onclick="setGripper(20)">Ac</button>
            <button onclick="setGripper(160)">Kapat</button>
        </div>
        <div class="reset-button">
            <button class="reset-btn" onclick="resetAll()">Reset</button>
        </div>
    </div>

    <script>
        let debounceTimer;
        const DEBOUNCE_DELAY = 20; // ms

        function updateServo(servoId, value) {
            clearTimeout(debounceTimer);
            debounceTimer = setTimeout(() => {
                fetch(`/set?servo=${servoId}&angle=${value}`);
            }, DEBOUNCE_DELAY);
        }

        const sliders = document.querySelectorAll('.slider');
        sliders.forEach(slider => {
            const valueDisplay = document.getElementById(slider.id + 'Value');
            slider.addEventListener('input', () => {
                valueDisplay.textContent = slider.value;
                updateServo(slider.id, slider.value);
            });
        });

        function setGripper(angle) {
            updateServo('servo4', angle);
        }

        function resetAll() {
            // Tüm servoları başlangıç pozisyonlarına getir
            updateServo('servo1', 90);
            updateServo('servo2', 90);
            updateServo('servo3', 90);
            updateServo('servo4', 20);
            
            // Slider değerlerini güncelle
            document.getElementById('servo1').value = 90;
            document.getElementById('servo2').value = 90;
            document.getElementById('servo3').value = 90;
            document.getElementById('servo1Value').textContent = '90';
            document.getElementById('servo2Value').textContent = '90';
            document.getElementById('servo3Value').textContent = '90';
        }
    </script>
</body>
</html>
)rawliteral";

// Servo pozisyonunu hızlı şekilde güncelle ve sınırları kontrol et
void smoothMove(Servo &servo, int &currentPos, int targetPos, unsigned long &lastMoveTime)
{
    // Hedef pozisyonu sınırlar içinde tut
    targetPos = constrain(targetPos, MIN_ANGLE, MAX_ANGLE);

    // Hareket için minimum bekleme süresi kontrolü
    if (millis() - lastMoveTime < SERVO_DELAY)
    {
        return;
    }

    if (currentPos != targetPos)
    {
        // Yumuşak hareket için hedefe doğru kademeli yaklaş
        int step = (targetPos - currentPos) / 2;
        if (abs(step) < SERVO_STEP)
        {
            step = (targetPos > currentPos) ? SERVO_STEP : -SERVO_STEP;
        }

        currentPos += step;

        // Hedef pozisyona çok yakınsak, direkt hedefe git
        if (abs(targetPos - currentPos) < SERVO_STEP)
        {
            currentPos = targetPos;
        }
    }

    // Mevcut pozisyonu sınırlar içinde tut
    currentPos = constrain(currentPos, MIN_ANGLE, MAX_ANGLE);

    servo.write(currentPos);
    lastMoveTime = millis();
}

void setup()
{
    Serial.begin(9600);

    // Servo motorlari baslat ve aci araliklarini ayarla
    servo1.attach(SERVO1_PIN, 500, 2500); // Genisletilmis PWM araligi
    servo2.attach(SERVO2_PIN, 500, 2500);
    servo3.attach(SERVO3_PIN, 500, 2500);
    servo4.attach(SERVO4_PIN, 500, 2500);

    // Baslangic pozisyonlari
    resetAllServos();

    // WiFi baglantisi
    WiFi.mode(WIFI_STA);

    // Once statik IP'yi dene
    Serial.println("Statik IP ayarlaniyor...");
    WiFi.config(staticIP, gateway, subnet, dns);

    WiFi.begin(ssid, password);

    // Baglanti icin 10 saniye bekle
    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20)
    {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    // Eger statik IP ile baglanilamazsa, DHCP'ye gec
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("\nStatik IP ile baglanti basarisiz, DHCP deneniyor...");
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(ssid, password);

        timeout = 0;
        while (WiFi.status() != WL_CONNECTED && timeout < 20)
        {
            delay(500);
            Serial.print(".");
            timeout++;
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nBaglanti basarili!");
        Serial.print("IP Adresi: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("\nBaglanti basarisiz!");
    }

    // Web sunucu routes
    server.on("/", []()
              { server.send(200, "text/html", html); });

    server.on("/set", []()
              {
        String servo = server.arg("servo");
        int angle = server.arg("angle").toInt();
        
        // Hedef pozisyonları güncelle
        if (servo == "servo1") {
            targetPos1 = angle;
        } else if (servo == "servo2") {
            targetPos2 = angle;
        } else if (servo == "servo3") {
            targetPos3 = angle;
        } else if (servo == "servo4") {
            targetPos4 = angle;
        }
        
        server.send(200, "text/plain", "OK"); });

    server.begin();
}

void loop()
{
    server.handleClient();

    // Servo pozisyonlarını asenkron olarak güncelle
    smoothMove(servo1, currentPos1, targetPos1, lastMoveTime1);
    smoothMove(servo2, currentPos2, targetPos2, lastMoveTime2);
    smoothMove(servo3, currentPos3, targetPos3, lastMoveTime3);
    smoothMove(servo4, currentPos4, targetPos4, lastMoveTime4);
}

// Tüm servoları başlangıç pozisyonlarına getir
void resetAllServos()
{
    currentPos1 = 90;
    currentPos2 = 90;
    currentPos3 = 90;
    currentPos4 = 20;

    targetPos1 = 90;
    targetPos2 = 90;
    targetPos3 = 90;
    targetPos4 = 20;

    servo1.write(currentPos1);
    servo2.write(currentPos2);
    servo3.write(currentPos3);
    servo4.write(currentPos4);
}
