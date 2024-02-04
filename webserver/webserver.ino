#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>

#define APP_WIFI_SSID  "CoEIoT"
#define APP_WIFI_PASS  "iot.coe.psu.ac.th"
#define SERVO_1 2
#define SERVO_2 4
#define SERVO_3 6
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

Servo myServo;
WebServer webServer(80);
unsigned int counter = 0;
unsigned long previousMillis = 0;
bool servoRunning = false;

void setupPCA9685() {
    pwm.begin();
    pwm.setPWMFreq(60);  // 60Hz
}

const char *index_html PROGMEM = R"(
<!DOCTYPE html>
<head>
    <meta charset=\UTF-8\>
    <title>ESP32 Web Server</title>
</head>
<body>
    <h1>Hello from ESP32!</h1>

    <!-- New button to control the Servo -->
   <form action="/controlServo" method="POST">
        <input type="submit" name="servo" value="Buy 1">
        <input type="submit" name="servo" value="Buy 2">
        <input type="submit" name="servo" value="Buy 3">
    </form>
</body>
</html>
)";

const char *sensor_html PROGMEM = R"(
<!DOCTYPE html>
<head>
    <meta charset=\UTF-8\>
    <title>ESP32 Sensor</title>
</head>
<body>
    <h1>ESP32 Sensor</h1>
    <p>The current counter is <span id="counter">N/A</span></p>
    <script>
        setInterval(async () => {
            const response = await fetch("/getSensorData");
            const data = await response.json();

            document.getElementById("counter").innerHTML = data.counter;
        }, 1000);
    </script>
</body>
</html>
)";//test web

void setupWiFi() {
    Serial.println();
    Serial.println("Connecting to " + String(APP_WIFI_SSID) + " ...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(APP_WIFI_SSID, APP_WIFI_PASS);

    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Failed to connect to the Wi-Fi network, restarting...");
        delay(2000);
        ESP.restart();
    }

    Serial.println("Wi-Fi connected, browse to http://" + WiFi.localIP().toString());
}


// void handleControlServo() {
//     myServo.attach(25); // SERVO_PIN คือขาที่ Servo ต่อเข้า
//     myServo.write(360); // หมุน Servo ที่องศาที่ต้องการ (ตั้งค่าตามที่ต้องการ)

//     delay(1000); // รอ 1 วินาที
//     myServo.detach(); // ยกเลิกการเชื่อมต่อ Servo
// }

void controlServo(int servoChannel) {
  servoRunning = true;
  unsigned long startTime = millis();
  unsigned long delayStart = millis();

  // Delay for 2 seconds before activating the servo
  while (millis() - delayStart < 2000) {
      webServer.handleClient();
  }

  // Map the servo channels to PCA9685 outputs (0, 2, 4 for channels 2, 4, 6)
  int pulse = map(servoChannel, 0, 6, 150, 600);
  pwm.setPWM(servoChannel, 0, pulse);

  delay(10000); // 10 seconds

  // Stop servo by setting PWM to 0
  pwm.setPWM(servoChannel, 0, 0);
  servoRunning = false;
}
void setupWebServer() {
    webServer.on("/", HTTP_GET, []() {
        webServer.send(200, "text/html", index_html);
    });

    // -------------------- Sensor Display --------------------

    webServer.on("/sensor", HTTP_GET, []() {
        webServer.send(200, "text/html", sensor_html);
    });

    webServer.on("/getSensorData", HTTP_GET, []() {
        StaticJsonDocument<200> doc;
        doc["counter"] = counter;

        String json;
        serializeJson(doc, json);

        webServer.send(200, "application/json", json);
    });

    // -------------------- HTTP Form POST --------------------

    // webServer.on("/setName", HTTP_POST, []() {
    //     String name = webServer.arg("name");
    //     Serial.println("Hello, " + name);
    //     webServer.send(200, "text/plain", "Name set to: " + name);
    

    // });
        // -------------------- New HTTP Form POST for Controlling Servo --------------------
    // webServer.on("/controlServo", HTTP_POST, []() {
    //     // เรียกใช้ฟังก์ชัน handleControlServo เพื่อควบคุม Servo
    //     handleControlServo();

    //     // ส่ง HTTP response กลับ
    //     webServer.send(200, "text/plain", "Servo controlled");
    // });

    webServer.on("/controlServo", HTTP_POST, []() {
        String servoValue = webServer.arg("servo");

        if (servoValue == "Buy 1"&& !servoRunning) {
            // เรียกใช้ handleControlServo เพื่อควบคุม Servo
            controlServo(2);
        } else if (servoValue == "Buy 2"&& !servoRunning) {
            // Control servo on channel 4
            controlServo(4);
        } else if (servoValue == "Buy 3"&& !servoRunning) {
            // Control servo on channel 6
            controlServo(6);
        }
        webServer.send(200, "text/plain", "Servo controlled");
    });
    webServer.begin();
}

void setup() {
	Serial.begin(115200);
    myServo.attach(25); // SERVO_PIN คือขาที่ Servo ต่อเข้า (ใน setup เพื่อให้มันเชื่อมต่อและเรียบร้อยตั้งแต่เริ่มต้น)
    Wire.begin(); // Initialize I2C communication
    setupPCA9685(); // Add this line to initialize PCA9685

    setupWiFi();
    setupWebServer();
}

void loop() {
    if (millis() - previousMillis > 1000) {
        previousMillis = millis();
        counter++;
    }


    webServer.handleClient();
}