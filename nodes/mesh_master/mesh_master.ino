/* MESH_Master
*/

// Libraries
#include "painlessMesh.h"
#include <Adafruit_NeoPixel.h> 

// Define's
#define   MESH_PREFIX     "UTEQ_CLIMOVI"
#define   MESH_PASSWORD   "uteqintel"
#define   MESH_PORT       5555

#define   NEOPIXELPIN     D4
#define   NEOPIXELLEDS    1

#define ZEROSMAX 8
#define node_rate  15

String devices[26] = {"alpha", "bravo", "charly", "delta", "echo",
                     "foxtrot", "golf", "hotel", "india", "juliet",
                     "kilo", "lima", "mike", "november", "oscar",
                     "papa", "quebec", "romeo", "sierra", "tango",
                     "uniform", "victor", "whiskey", "xray", "yankee", "zulu"};
unsigned int nodes;

// Objects
Scheduler userScheduler;
painlessMesh  mesh;
Adafruit_NeoPixel neo = Adafruit_NeoPixel(NEOPIXELLEDS, NEOPIXELPIN, NEO_GRB + NEO_KHZ800); 


void node_task();
Task taskNodes(TASK_SECOND *node_rate, TASK_FOREVER, &node_task);

void setup() {  

  // HW
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // NeoPixel
  neo.begin();             
  neo.setBrightness(255);
  neo.setPixelColor(0,255,0,0); // (POS; R;G;B)
  neo.show();

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

    digitalWrite(LED_BUILTIN, HIGH);
    Serial.print(msg);
    Serial.println();
    digitalWrite(LED_BUILTIN, LOW);

    DynamicJsonDocument doc(100);
    deserializeJson(doc, msg);
    JsonObject obj = doc.as<JsonObject>();

    String id = obj["id"];
    for(i=0; i<26; i++)
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

    for(i=0; i<26; i++)
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
      neo.setPixelColor(0,255,0,0); // (POS; R;G;B)
      neo.show();  
      if(zeros >= ZEROSMAX) ESP.reset();
    }
    else
    {
      zeros = 0;
      neo.setPixelColor(0,0,0,255); // (POS; R;G;B)
      neo.show();
    }
    
    doc["nodes"] = String (total);    
    nodes = 0;

    serializeJson(doc, Serial);
    Serial.println();
}
