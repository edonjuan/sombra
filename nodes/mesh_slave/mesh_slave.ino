/* MESH_Slave
 *  Sensor: Temperature / Humidity (DTH11/22)
 *  PIR( Digital), LDR (Analog)
 *  Watchdog for Master
*/

// Libraries
#include <DHT.h>
#include <painlessMesh.h>

// HW
#define DHTTYPE   DHT11
#define DHTPIN    13 
#define BTN       0
#define PIR       5 
#define LDR       A0
#define NAME    "echo"

// SW
#define MASTER    3257176644
#define msg_rate  10
#define wtd_rate  10
#define wtd_rest  3

// NETWORK
#define   MESH_PREFIX     "MESH_intel"
#define   MESH_PASSWORD   "uteqintel"
#define   MESH_PORT       5555

// Objetct declaration
DHT dht (DHTPIN, DHTTYPE);
Scheduler userScheduler;
painlessMesh  mesh;

// Global variables
float mov_detections;
unsigned int mac;

// Functions structure
void sendMessage(); 
void wtd();
void IRAM_ATTR pir_action();

// Task declaration
Task taskSendMessage(TASK_SECOND *msg_rate, TASK_FOREVER, &sendMessage);
Task taskWatchDog(TASK_SECOND *wtd_rate, TASK_FOREVER, &wtd);     

void setup() {
  
  // HW   
  Serial.begin(9600); 
  delay(100);
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !LOW);
  pinMode(PIR, INPUT);
  pinMode (BTN,INPUT);
  attachInterrupt(digitalPinToInterrupt(PIR), pir_action, RISING);

  dht.begin ();

  // Network
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  delay(5000);
  mac = mesh.getNodeId();

  // SW
  userScheduler.addTask(taskSendMessage);
  userScheduler.addTask(taskWatchDog);
  taskWatchDog.enable();
  taskSendMessage.disable();
}
          
void loop() {
  mesh.update();
}

void sendMessage() {

  float t=dht.readTemperature();
  float h=dht.readHumidity();  
  if (isnan(h) || isnan(t))
   Serial.println(F("Failed to read DHT!"));
  
  float l = analogRead(LDR);
    
  StaticJsonDocument<500> doc;  
  doc["type"] = "data";
  doc["id"] = NAME;
  doc["mac"] = String(mac);

  JsonArray group = doc.createNestedArray("group");

  StaticJsonDocument<100> sensor;
  sensor["sensor"] = "esp8266";
  sensor["measure"] = "status";
  sensor["value"] = "1";  
  group.add(sensor);  
  
  sensor["sensor"] = "dht11";
  sensor["measure"] = "temperature";
  sensor["value"] = String(t);  
  group.add(sensor);

  sensor["sensor"] = "dht11";
  sensor["measure"] = "humidity";
  sensor["value"] = String(h); 
  group.add(sensor);

  sensor["sensor"] ="ldr";
  sensor["measure"] = "light";
  sensor["value"] = String(l); 
  group.add(sensor);  

  sensor["sensor"] ="pir";
  sensor["measure"] = "detections";
  sensor["value"] = String(mov_detections); 
  group.add(sensor);
  
  serializeJsonPretty(doc, Serial);
  Serial.println(); 

  String output;
  serializeJson(doc, output);
  mesh.sendSingle(MASTER, output);
}

void wtd(){
  static short watchCount, first;
  char strBuf[40];

  if(mesh.isConnected(MASTER))
  {
    digitalWrite(LED_BUILTIN, !HIGH);
    watchCount = 0;
    if(!first){
      taskSendMessage.enable();
      first = 1;
    }    
  }
  else
  {
    digitalWrite(LED_BUILTIN, !LOW);
    taskSendMessage.disable();
    first = 0;
    watchCount++;
    if(watchCount>=wtd_rest) ESP.reset();
    sprintf(strBuf, "Master not found! Watchdog Count: %d of %d", watchCount, wtd_rest);
    Serial.println(strBuf);    
  }
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
}

void IRAM_ATTR pir_action() {
  mov_detections++;    
}
