#include <Wire.h> // Library for I2C communication

#define BLYNK_PRINT Serial // Enables Blynk debug prints

/* Blynk Device Info */
#define BLYNK_TEMPLATE_ID "TMPL62dTnsvrD"
#define BLYNK_TEMPLATE_NAME "Bike Guard by Electrical Ghost"
#define BLYNK_AUTH_TOKEN "3axZFQRZJsMfFGpWaLFsrBQrhFQIMlXm"

#include <ESP8266WiFi.h>           // Library for ESP8266 WiFi functionality
#include <BlynkSimpleEsp8266.h>    // Blynk library for ESP8266

// WiFi credentials
char ssid[] = "Musa Personal";
char pass[] = "hello2441139";

const int MPU = 0x68;               // I2C address of the MPU-6050 sensor
const int SYSTEM_SWITCH = 16;       // GPIO pin for System (D0)
const int KILL_SWITCH = 14;         // GPIO pin for Kill (D5)
const int SIREN_SWITCH = 12;        // GPIO pin for Siren (D6)
const int ALARM = 13;               // GPIO pin for the Alarm (D7)
const int SYSTEM_INDICATOR = 15;    // GPIO pin for the System Indicator (D8)

int16_t AcX, AcY, AcZ;              // Variables to store sensor data

float baselineX = 0, baselineY = 0, baselineZ = 0;  // Baseline values for accelerometer data

bool baselineCollected = false;     // Flag to indicate if baseline is collected
bool systemIsOn = false;            // Flag to indicate if System is On
bool killIsOn = false;              // Flag to indicate if Kill is On
bool sirenIsOn = false;             // Flag to indicate if Kill is On


unsigned long startMillis;          // Variable to store the start time

const int BASELINE_DURATION = 5000; // Duration to collect baseline data (5 seconds)

const int BaseDiffX = 500;          // Threshold for X-axis deviation
const int BaseDiffY = 500;          // Threshold for Y-axis deviation
const int BaseDiffZ = 500;          // Threshold for Z-axis deviation

int sampleCount = 0;                // Number of samples collected for baseline
int ledOnDuration = 2000;           // Initial LED on duration (2 seconds)

void setup() {
  Serial.begin(9600);  // Initialize serial communication
  Wire.begin();        // Initialize I2C communication

  // Initialize MPU-6050 sensor
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0);     // Wake the sensor up
  Wire.endTransmission(true);

  // Pin setup
  pinMode(SYSTEM_SWITCH, OUTPUT);  // D0 System Switch
  pinMode(KILL_SWITCH, OUTPUT);    // D5 Kill Switch
  pinMode(SIREN_SWITCH, OUTPUT);   // D6 Siren Switch
  pinMode(SYSTEM_INDICATOR, OUTPUT);  // D8 System Indicator
  pinMode(LED_BUILTIN, OUTPUT);    // Built-in LED
  pinMode(ALARM, OUTPUT);          // Alarm LED

  // Start Blynk
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Start collecting baseline data
  startMillis = millis();  // Record the start time
}

// Blynk connection established
BLYNK_CONNECTED() {
  digitalWrite(SYSTEM_INDICATOR, HIGH);  // Turn on System Indicator (D8)
}

// Blynk connection lost
BLYNK_DISCONNECTED() {
  digitalWrite(SYSTEM_INDICATOR, LOW);  // Turn off System Indicator (D8)
}

// Blynk virtual pin handlers
BLYNK_WRITE(V0) {
  int value = param.asInt();                        // Read value from the Blynk app
  digitalWrite(SYSTEM_SWITCH, value ? HIGH : LOW);  // Control D0 System Switch
  systemIsOn = value ? true : false;                // Update systemIsOn flag
  baselineCollected = false;                        // Reset baseline collection flag
  ledOnDuration= 2000;                              // Reset duration time
  sampleCount = 0;                                  // Reset sample count
  baselineX = 0;                                    // Reset baseline X value
  baselineY = 0;                                    // Reset baseline Y value
  baselineZ = 0;                                    // Reset baseline Z value
  startMillis = millis();                           // Reset start time
}

BLYNK_WRITE(V1) {
  int value = param.asInt();
  digitalWrite(KILL_SWITCH, value ? HIGH : LOW);  // Control D5 Kill Switch
  killIsOn = value ? true : false;                // Update Kill flag
}

BLYNK_WRITE(V2) {
  int value = param.asInt();
  digitalWrite(SIREN_SWITCH, value ? HIGH : LOW);  // Control D6 Siren Switch
  sirenIsOn = value ? true : false;                // Update Kill flag

}

BLYNK_WRITE(V3) {
  int value = param.asInt();
  digitalWrite(SYSTEM_INDICATOR, value ? HIGH : LOW);  // Control D8 System Indicator
}

void loop() {
  // Run Blynk
  Blynk.run();

  // Request data from MPU-6050
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);                  // Start reading from register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);   // Request 14 bytes of data

  // Read accelerometer data
  AcX = Wire.read() << 8 | Wire.read();  // Accelerometer X
  AcY = Wire.read() << 8 | Wire.read();  // Accelerometer Y
  AcZ = Wire.read() << 8 | Wire.read();  // Accelerometer Z

  if (systemIsOn) {
    // If baseline is not yet collected
    if (!baselineCollected) {
      // Collect baseline data for 5 seconds
      baselineX += AcX;
      baselineY += AcY;
      baselineZ += AcZ;
      sampleCount++;

      // Check if the baseline collection time has elapsed
      if (millis() - startMillis >= BASELINE_DURATION) {
        // Calculate average baseline values
        baselineX /= sampleCount;
        baselineY /= sampleCount;
        baselineZ /= sampleCount;

        baselineCollected = true;  // Mark baseline as collected
        Serial.print("baselineCollected: "); Serial.println(baselineCollected);

        // Briefly turn on the alarm to indicate baseline collection is complete
        digitalWrite(ALARM, HIGH);
        delay(500);
        digitalWrite(ALARM, LOW);

        // Print the baseline values for debugging
        Serial.print("baselineX: ");
        Serial.print(baselineX);
        Serial.print(" || baselineY: ");
        Serial.print(baselineY);
        Serial.print(" || baselineZ: ");
        Serial.println(baselineZ);
      }
    } else {
      // Calculate the difference between current data and baseline
      float diffX = abs(AcX - baselineX);
      float diffY = abs(AcY - baselineY);
      float diffZ = abs(AcZ - baselineZ);

      // If the difference exceeds the threshold
      if (diffX > BaseDiffX || diffY > BaseDiffY || diffZ > BaseDiffZ) {

        
        // Deviation detected, activate the alarm LED
        digitalWrite(ALARM, HIGH);
        delay(ledOnDuration);  // Keep the LED on for the set duration
        digitalWrite(ALARM, LOW);

        // Increase the LED on duration for subsequent deviations
        if (ledOnDuration == 2000) {
          Serial.println("Triggered for 2 seconds");
          ledOnDuration = 4000;
        } 
        
        else if (ledOnDuration == 4000) {
          Serial.println("Triggered for 4 seconds");
          ledOnDuration = 6000;
        } 
        
        else if (ledOnDuration == 6000){
          Serial.println("Triggered for 15 seconds");
          ledOnDuration = 15000;
        }

        else{
          Serial.println("Triggered for 30 seconds");
          ledOnDuration = 30000;
        }
      }
    }
  }

  Serial.print("systemIsOn: "); Serial.print(systemIsOn);
  Serial.print("|| killIsOn: "); Serial.print(killIsOn);
  Serial.print("|| sirenIsOn: "); Serial.print(sirenIsOn);
  Serial.print("|| baselineCollected: "); Serial.println(baselineCollected);


  delay(500);  // Adjust delay as needed for responsiveness
}
