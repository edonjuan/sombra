/* MESH_Master
*/

// Libraries
#include "painlessMesh.h"

// Define's
#define   MESH_PREFIX     "MESH_intel"
#define   MESH_PASSWORD   "uteqintel"
#define   MESH_PORT       5555

#define MASTER   3257176644
#define BRAVO    3822979383
#define CHARLY   2440612283 
#define DELTA    3257178212  
#define ECHO     3257177579 

#define ZEROSMAX 10

#define node_rate  15

String devices[5] = {"alpha", "bravo", "charly", "delta", "echo"};
unsigned int nodes;

// Objects
Scheduler userScheduler;
painlessMesh  mesh;

void node_task();
Task taskNodes(TASK_SECOND *node_rate, TASK_FOREVER, &node_task);

void setup() {  

  // HW
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !LOW);

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  userScheduler.addTask(taskNodes);
  taskNodes.enable();
}

void loop() {
  mesh.update();
}

void receivedCallback(uint32_t from, String&msg ) {  

    short i;

    digitalWrite(LED_BUILTIN, !HIGH);
    Serial.print(msg);
    Serial.println();
    digitalWrite(LED_BUILTIN, !LOW);

    DynamicJsonDocument doc(100);
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();

    String id = obj["id"];
    for(i=0; i<5; i++)
    {
      if(id == devices[i])
      {
        nodes = nodes | (1<<i);
      } 
    }
}

void node_task(){
    short i, total;   
    static short zeros;

    StaticJsonDocument<200> doc;    
    doc["type"] = "nodes";
    doc["id"] = "master";

    JsonArray dispositivos = doc.createNestedArray("devices");

    for(i=0; i<5; i++)
    {
      if(nodes & (1<<i))
      {
        dispositivos.add(devices[i]);
        total++;
      } 
    }

    if(total == 0)
    {
      zeros++;
      if(zeros >= ZEROSMAX) ESP.reset();
    }
    else
    {
      zeros = 0;
    }
    
    doc["nodes"] = String (total);    
    nodes = 0;

    serializeJson(doc, Serial);
    Serial.println();
}
