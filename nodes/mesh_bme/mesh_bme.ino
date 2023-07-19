/* MESH_Slave_Foxtrot
 *  Sensor: Temperature/Humidity (DTH11)
 *  Sensor: Temp/Hum/CO2/Altitude/Pressure (BME680)
 *  PIR(Digital), LDR (Analog)
 *  NeoPixel included
 *  Watchdog for Master
*/

// Libraries
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <Adafruit_NeoPixel.h> 
#include <DHT.h>
#include <Wire.h>
#include <painlessMesh.h>

// HW
#define DHTTYPE               DHT11
#define SEALEVELPRESSURE_HPA (1013.25)
#define DHTPIN                14 
#define NEOPIXELPIN           12
#define NEOPIXELLEDS          3
#define PIR                   13 
#define LDR                   A0
#define BUZZER                2
#define NAME                  "foxtrot"

// SW   rate-->seconds
#define MASTER    3257176644
#define msg_rate  10
#define wtd_rate  10
#define wtd_rest  3

// NETWORK
#define   MESH_PREFIX     "MESH_intel"
#define   MESH_PASSWORD   "uteqintel"
#define   MESH_PORT       5555

// Objetct declaration
DHT dht(DHTPIN, DHTTYPE, 27); 
Adafruit_BME680 bme; 
Adafruit_NeoPixel neo = Adafruit_NeoPixel(NEOPIXELLEDS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800); 
Scheduler userScheduler;
painlessMesh  mesh;

// Global variables
float mov_detections;
unsigned int mac;

// Functions structure
void sendMessage(); 
void wtd();
void IRAM_ATTR pir_action();
void neo_level(float level);

// Task declaration
Task taskSendMessage(TASK_SECOND *msg_rate, TASK_FOREVER, &sendMessage);
Task taskWatchDog(TASK_SECOND *wtd_rate, TASK_FOREVER, &wtd);

void setup() {  
  // HW   
  Serial.begin(9600); 
  delay(100);

  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, LOW);
  
  pinMode(PIR, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR), pir_action, CHANGE);

  dht.begin ();
  bme.begin ();

  // Network
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  delay(5000);
  mac = mesh.getNodeId();

  // SW
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskWatchDog);
  taskWatchDog.enable();
  taskSendMessage.disable();
  
  // NeoPixel
  neo.begin();             
  neo.setBrightness(255);
  neo.setPixelColor(0,255,0,0); // (POS; R;G;B)
  neo.setPixelColor(1,0,255,0); // (POS; R;G;B)
  neo.setPixelColor(2,0,255,0); // (POS; R;G;B)
  neo.show();
}

void loop() {
  mesh.update();
}

void sendMessage() {
  
  float dht_t=dht.readTemperature();
  float dht_h=dht.readHumidity();  
  float l = analogRead(LDR);
    
  StaticJsonDocument<2000> doc;  
  doc["type"] = "data";
  doc["id"] = NAME;
  doc["mac"] = String(mac);

  JsonArray group = doc.createNestedArray("group");

  StaticJsonDocument<200> sensor;
  sensor["sensor"] = "esp8266";
  sensor["measure"] = "status";
  sensor["value"] = "1";  
  group.add(sensor);  

  sensor["sensor"] ="pir";
  sensor["measure"] = "detections";
  sensor["value"] = String(mov_detections); 
  group.add(sensor);

  sensor["sensor"] ="ldr";
  sensor["measure"] = "light";
  sensor["value"] = String(l); 
  group.add(sensor);    

  if (isnan(dht_t) || isnan(dht_h))
  {
    Serial.println(F("Failed to read DHT!"));
  }
  else
  {
    sensor["sensor"] = "dht11";
    sensor["measure"] = "temperature";
    sensor["value"] = String(dht_t);  
    group.add(sensor);
  
    sensor["sensor"] = "dht11";
    sensor["measure"] = "humidity";
    sensor["value"] = String(dht_h); 
    group.add(sensor);
  }
   
  if (!bme.performReading())
  {
    Serial.println(F("Failed to read BME!")); 
  }
  else
  {
    float bme_t = bme.temperature;
    float bme_h = bme.humidity;
    float bme_p = bme.pressure / 100.0;
    float bme_a = bme.readAltitude(SEALEVELPRESSURE_HPA);
    float bme_g = bme.gas_resistance/1000.0;

    neo_level(bme_g);
    
    sensor["sensor"] = "bme680";
    sensor["measure"] = "temperature";
    sensor["value"] = String(bme_t); 
    group.add(sensor);
  
    sensor["sensor"] = "bme680";
    sensor["measure"] = "humidity";
    sensor["value"] = String(bme_h); 
    group.add(sensor);
  
    sensor["sensor"] = "bme680";
    sensor["measure"] = "pressure";
    sensor["value"] = String(bme_p); 
    group.add(sensor);
  
    sensor["sensor"] = "bme680";
    sensor["measure"] = "altitude";
    sensor["value"] = String(bme_a); 
    group.add(sensor);
  
    sensor["sensor"] = "bme680";
    sensor["measure"] = "co2";
    sensor["value"] = String(bme_g); 
    group.add(sensor);  
  } 
    
  //serializeJsonPretty(doc, Serial);
  //Serial.println(); 
  
  String output;
  serializeJson(doc, output);
  mesh.sendSingle(MASTER, output);
}

void wtd(){
  static short watchCount, first;
  char strBuf[40];

  if(mesh.isConnected(MASTER))
  {
    neo.setPixelColor(0,0,0,255); // (POS; R;G;B)
    watchCount = 0;
    if(!first){
      taskSendMessage.enable();
      first = 1;
    }    
  }
  else
  {
    neo.setPixelColor(0,255,0,0); // (POS; R;G;B)
    taskSendMessage.disable();
    first = 0;
    watchCount++;
    if(watchCount>=wtd_rest) ESP.reset();
    sprintf(strBuf, "Master not found! Watchdog Count: %d of %d", watchCount, wtd_rest);
    Serial.println(strBuf);    
  }
  neo.show();
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
}

void IRAM_ATTR pir_action() {
  if(digitalRead(PIR))
  {
    mov_detections++;   
    neo.setPixelColor(1,255,0,0); 
  }
  else
  {
    neo.setPixelColor(1,0,255,0);
  }
  neo.show();  
}

void neo_level(float level)
{
  if(level>3000)
  {
    neo.setPixelColor(2,255,0,0);
    digitalWrite(BUZZER, HIGH);
  }
  else if(level>2200)
  {
    neo.setPixelColor(2,255,70,0); 
    digitalWrite(BUZZER, LOW);
  }
  else if(level>1500)
  {
    neo.setPixelColor(2,255,255,0); 
    digitalWrite(BUZZER, LOW);
  }
  else
  {
    neo.setPixelColor(2,0,255,0); 
    digitalWrite(BUZZER, LOW);
  }
  neo.show();  
}
