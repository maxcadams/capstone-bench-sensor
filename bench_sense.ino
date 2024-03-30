#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define analogPin A0

Adafruit_ADS1015 ads;

int adcVal = 0;
const char *ssid = "CMU-DEVICE";
const int threshold = 570;         // Threshold value for occupancy detection
const unsigned long sampleInterval = 300; // Interval between each sample in milliseconds
const int numSamples = 10;         // Number of samples to calculate running average
int sensorReadings[numSamples];    // Array to store sensor readings for running average
int currentIndex = 0;              // Index for current sensor reading
int runningSum = 0;                // Running sum of sensor readings
int occupiedInstances = 0;
int unoccupiedInstances = 0;       // Keep track of unoccupied instances (should allow for someone to take water breaks)
int runningAverage = 0;            // Running average of sensor readings
bool occupied = false;             // Keeps track if node is occupied or not
bool initialized = false;          // Keeps track if we have our initial values yet or not

const char* url = "http://ip-hidden-for-now/update_bench_status";
const char* occupiedBody = "{\"bench_id\":\"bench_1\",\"occupied\":true}";
const char* unoccupiedBody = "{\"bench_id\":\"bench_1\",\"occupied\":false}";


void setup() {
  // put your setup code here, to run once:
  //Serial.begin(115200); // setting up baud rate
  //Serial.println();
  // pinMode(LED_BUILTIN, OUTPUT);  // Initialize the LED_BUILTIN pin as an output
  //Serial.println("Connecting");
 
  WiFi.begin(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    //Serial.print(".");
  }
  //Serial.println();
  //Serial.print("Connected to "); Serial.println(ssid);
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(unoccupiedBody);
  ads.setGain(GAIN_ONE);
  ads.begin();
}

void loop() {
  // put your main code here, to run repeatedly:
  int sensorValue = analogRead(analogPin);

  if (!initialized) {
    sensorReadings[currentIndex] = sensorValue;
    runningSum += sensorValue;
    currentIndex = (currentIndex + 1) % numSamples;
    
    if (currentIndex == 0) {
      initialized = true;  // Array is filled with initial readings
    }
  } else {
    runningSum = runningSum - sensorReadings[currentIndex] + sensorValue;
    sensorReadings[currentIndex] = sensorValue;
    currentIndex = (currentIndex + 1) % numSamples;

    // Calculate running average
    runningAverage = runningSum / numSamples;

    // Print running average
    //Serial.print("Running Average: ");
    //Serial.print(runningAverage); Serial.print(" | occupied: "); Serial.println(occupied);
  
    // Check for occupancy
    if (runningAverage > threshold && !occupied && occupiedInstances >= 8) {
      //Serial.println("Occupied");
      unoccupiedInstances = 0;
      occupied = true;
      WiFiClient client;
      HTTPClient http;
      http.begin(client, url);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(occupiedBody);
      //Serial.print("Got response: "); Serial.println(httpResponseCode);
      occupiedInstances = 0;
    } else if (runningAverage > threshold && !occupied) {
      occupiedInstances+=1;
    } else if (runningAverage > threshold && occupied) {
      unoccupiedInstances = 0;
    }
    else if (runningAverage <= threshold && occupied && unoccupiedInstances == 10) {
      //Serial.println("Unoccupied");
      unoccupiedInstances = 0;
      occupied = false;
      WiFiClient client;
      HTTPClient http;
      http.begin(client, url);
      http.addHeader("Content-Type", "application/json");
      int httpResponseCode = http.POST(unoccupiedBody);
      //Serial.print("Got response: "); Serial.println(httpResponseCode);
    } else if (runningAverage <= threshold && occupied) {
      //Serial.println("incr unoccupiedInstances");
      unoccupiedInstances+=1;
      delay(1100);
    }
  }

  delay(sampleInterval); // Delay before next sample
}

