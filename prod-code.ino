
#include "VOneMqttClient.h"
#include <ESP32Servo.h>
#include "DHT.h"

// define device id  
const char* InfraredSensor = "36c5b460-3913-46c2-b452-a8e4aa04fe9f";  // to detect human presence
const char* MQ2Sensor = "30758fd2-d3ff-4451-af6d-3edf878cd4d8";   // to detect harmful gas 
const char* ServoMotor = "4f6539dd-49d5-427f-a985-a99cbaeb7e5a";  //  to open and close the lid
const char* UltraSonic = "bbbf4765-a61f-4b18-9e50-677564b6ff51"; // to measure the thrash level distance - if can config
const char* servo = "4f6539dd-49d5-427f-a985-a99cbaeb7e5a"; // servo 
const char* red_led1 = "428bf1fb-58b6-4981-a602-b2777f4b40ef"; // led 1
const char* greed_led2 = "5ad78057-180a-4435-995e-038c8522feb1"; // led 2
const char* allData = "685e9070-560d-49eb-b24e-5f29e4ba0588"; // data from ir,ultrasonic,gas
const char* DHT11Sensor = "fbce49d7-8d12-42c2-bb6a-04f685a50603"; // to measure the humidity and temperature level

// used pin on the board
const int servoPin = 22;
const int mq2Pin = 32;
const int infraredPin = 7;
const int trigPin = 1; // ultrasonic use
const int echoPin = 3; // ultrasonic use
const int ledPin1 = 26; // for led - red - works
const int ledPin2 = 27; // for led - green - works
const int dht11Pin = 19; 
long duration; // ultrasonic use
float distance; // ultrasonic use
float gasValue;
int infraredVal;
float humidity; // dht11 
float temp; // dht11

// create a servo object
Servo Servo1;

#define DHTTYPE DHT11
DHT dht(dht11Pin, DHTTYPE);

//Create an instance of VOneMqttClient
VOneMqttClient voneClient;

//last message time
unsigned long lastMsgTime = 0;

void setup_wifi() {
  delay(10); // connect to the wifi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void triggerActuator_callback(const char* actuatorDeviceId, const char* actuatorCommand)
{
  //actuatorCommand format {"servo":90}
  Serial.print("Main received callback : ");
  Serial.print(actuatorDeviceId);
  Serial.print(" : ");
  Serial.println(actuatorCommand);

  String errorMsg = "";

  JSONVar commandObjct = JSON.parse(actuatorCommand);
  JSONVar keys = commandObjct.keys();

  if (String(actuatorDeviceId) == ServoMotor)
  {
    //{"servo":90}
    String key = "";
    JSONVar commandValue = "";
    for (int i = 0; i < keys.length(); i++) {
      key = (const char* )keys[i];
      commandValue = commandObjct[keys[i]];

    }
    Serial.print("Key : ");
    Serial.println(key.c_str());
    Serial.print("value : ");
    Serial.println(commandValue);

    int angle = (int)commandValue;
    Servo1.write(angle);
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, errorMsg.c_str(), true);//publish actuator status
  }
  else
  {
    Serial.print(" No actuator found : ");
    Serial.println(actuatorDeviceId);
    errorMsg = "No actuator found";
    voneClient.publishActuatorStatusEvent(actuatorDeviceId, actuatorCommand, errorMsg.c_str(), false);//publish actuator status
  }
}

void setup() {
  // put your setup code here, to run once:
  setup_wifi();
  voneClient.setup();
  voneClient.registerActuatorCallback(triggerActuator_callback);
  //sensor
  pinMode(infraredPin, INPUT_PULLUP);
  pinMode(trigPin,OUTPUT);
  pinMode(echoPin,INPUT);
  dht.begin();
  //actuator
  Servo1.attach(servoPin);
  pinMode(ledPin1,OUTPUT);
  pinMode(ledPin2,OUTPUT);
  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!voneClient.connected()) {
    voneClient.reconnect();
    voneClient.publishDeviceStatusEvent(InfraredSensor, true);
    voneClient.publishDeviceStatusEvent(MQ2Sensor, true);
    voneClient.publishDeviceStatusEvent(UltraSonic, true);
    voneClient.publishDeviceStatusEvent(DHT11Sensor, true);
  }
  voneClient.loop();

   // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2;
  // Prints the distance on the Serial Monitor
  Serial.print(">>> Distance: ");
  Serial.println(distance);
  Serial.print("> Infrared value : ");
  Serial.println(infraredVal);
  Serial.print(">> Gas value :: ");
  Serial.println(gasValue);

  if (infraredVal == 0){
    // Make servo go to 0 degrees 
    Servo1.write(155); 
    digitalWrite(ledPin1, HIGH); // red led
    digitalWrite(ledPin2, LOW);  // green led
    delay(1000); 
  }
  if (infraredVal == 1) {
    // Make servo go to 0 degrees 
    Servo1.write(65); 
    digitalWrite(ledPin1, LOW); // red led
    digitalWrite(ledPin2, HIGH); // green led
    delay(1000); 
  }

  unsigned long cur = millis();
  if (cur - lastMsgTime > INTERVAL) {
    lastMsgTime = cur;
    //Publish telemetry data for infrared
    infraredVal = !digitalRead(infraredPin);
 //   voneClient.publishTelemetryData(InfraredSensor, "Obstacle", infraredVal);
    //Publish telemetry data for mq2 gas sensor
    gasValue = analogRead(mq2Pin);
  //  voneClient.publishTelemetryData(MQ2Sensor, "Gas detector", gasValue);
     //Publish telemetry data for ultrasonic sensor
   // voneClient.publishTelemetryData(UltraSonic, "Distance", distance); 

    humidity = dht.readHumidity();
    temp = dht.readTemperature();
    JSONVar payloadObject;
    payloadObject["gas value"] = gasValue;
    payloadObject["ir value"] = infraredVal;
    payloadObject["distance value"] = distance;
    payloadObject["humidity"] = humidity;
    payloadObject["temperature"] = temp;
    voneClient.publishTelemetryData(allData, payloadObject);
  }

}

