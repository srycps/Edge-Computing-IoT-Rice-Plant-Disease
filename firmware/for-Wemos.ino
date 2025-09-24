#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <BH1750.h>
#include <SoftwareSerial.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// WiFi credentials-----------------------
const char* ssid = "xxx"; // adjust
const char* password = "123"; // here too

// httpsPort n host
const char* host = "script.google.com";
const int httpsPort = 443;

WiFiClientSecure client;  // Create a WiFi client object

String GAS_ID = "xxx"; // adjust

#define I2C_ADDR 0x3F
LiquidCrystal_I2C lcd(I2C_ADDR, 16, 2);
BH1750 lightMeter;  // Object light sensor
SoftwareSerial espSerial(4, 5); // RX, TX (ex D5 = GPIO14, D6 = GPIO12)

// Def pin for LED
#define LED_GREEN  15   // GPIO2  -> Healthy Leaf
#define LED_RED    14  // GPIO14 -> Brown Spot
#define LED_YELLOW 12  // GPIO12 -> Tungro
#define LED_WHITE  13  // GPIO13 -> Blight

// Light range needed
#define MIN_LIGHT 80   // Min lux
#define MAX_LIGHT 90000  // Max

void connectToWiFi(){
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
    delay(500);
    Serial.println("Connecting to WiFi...");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi");
  } else {
    Serial.println("WiFi connection failed. Continuing without internet.");
  }
}

void sendData(int result, float confidence) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Skipping data send.");
    return;
  }

  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  String resString = String(result);
  String confString = String(confidence, 2);

  String url = "/macros/s/" + GAS_ID + "/exec?Result=" + resString + "&Confidence=" + confString;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: ESP8266Client\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
}

void setup() {
    Serial.begin(115200); // UART for general comms
    Serial.flush();
    espSerial.begin(115200); // comms w ESP32
    Wire.begin(2, 0); // using GPIO2 (D4) as SDA n GPIO0 (D3) as SCL

    connectToWiFi();
    client.setInsecure();
    
    // Init LCD
    lcd.init();
    lcd.begin(16, 2);
    lcd.backlight();

    // Init BH1750
    if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("BH1750 Error, check connection!");
        while (1);
    }
    
    // Inist LED as output
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_WHITE, OUTPUT);

    // Turn all LED off 
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_WHITE, LOW);
    
    // display initial message
    lcd.setCursor(0, 0);
    lcd.print("Waiting for data...");
}

void loop() {
  if (espSerial.available()) {
    // === READ LUX WHEN PERFORMING DETECTION ===
    float lux = lightMeter.readLightLevel();
    Serial.print("Light Intensity: ");
    Serial.print(lux);
    Serial.println(" lux");

    // Reset LCD disp
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Lux: ");
    lcd.print(lux);  
    lcd.print(" lx");

    // Check if there is sufficient light
    if (lux < MIN_LIGHT || lux > MAX_LIGHT) {
        Serial.println("Not enough light, waiting...");
        lcd.setCursor(0, 1);
        lcd.print("Not enough light...");
        delay(3000);  // Wait before continuing detection
        return;
    }

    // Turn off all LEDs
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_WHITE, LOW);

    // === READ DATA FROM ESP32-CAM ===
    String data = espSerial.readStringUntil('\n');
    data.trim();
    Serial.print("Raw data: ");
    Serial.println(data);

    int separatorIndex = data.indexOf(':');
    if (separatorIndex > 0) {
        int result = data.substring(0, separatorIndex).toInt();
        float confidence = data.substring(separatorIndex + 1).toFloat();

        Serial.print("Parsed result: ");
        Serial.println(result);
        Serial.print("Parsed confidence: ");
        Serial.println(confidence);

        // Display detection results on the LCD
        //lcd.clear();
        lcd.setCursor(0, 0);

        if (result == 0) {
            lcd.setCursor(0, 1);
            lcd.print("Healthy Plants");
            digitalWrite(LED_GREEN, HIGH);
        } else if (result == 1) {
            lcd.setCursor(0, 1);
            lcd.print("Brown Spot!");
            digitalWrite(LED_RED, HIGH);
        } else if (result == 2) {
            lcd.setCursor(0, 1);
            lcd.print("Tungro!");
            digitalWrite(LED_YELLOW, HIGH);
        } else if (result == 3) {
            lcd.setCursor(0, 1);
            lcd.print("Blight!");
            digitalWrite(LED_WHITE, HIGH);
        } else {
            lcd.setCursor(0, 1);
            lcd.print("Invalid Data");
        }

        sendData(result, confidence);
    } else {
        Serial.println("Invalid data format.");
    }

    delay(3000); // Wait x seconds before reading new data
  }
}


