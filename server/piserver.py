import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS
import time, serial, json

def puertos_seriales():
    ports = ['/dev/ttyUSB%s' % (i + 1) for i in range(-1,20,1)]
    encontrados = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            encontrados.append(port)
        except (OSError, serial.SerialException):
            pass
    return encontrados

def send_data():
    '''
        This functions send the data of the sensors to the server
    '''
    for i in msg["group"]:
        p = influxdb_client.Point(i["sensor"]).tag("host",msg["id"]).field(i["measure"], float(i["value"]))
        #print("writing data --> ",p)
        write_api.write(bucket=bucket, org=org, record=p)
    return None

def send_node():
    '''
        This functions send the number of slaves in the mesh to the server
    '''
    p = influxdb_client.Point("connections").tag("host", msg["id"]).field("nodes",int(msg["nodes"]))
    #print("writing node --> ",p)
    write_api.write(bucket=bucket, org=org, record=p)
    return None

#Influx info
bucket = "mesh4"
org = "intel"
token="-brfTpS6lLlNSLLBF-LM2rbqNkJXFaCyIyRGam69LqgIiNhwu_yXE1rp68ORg_TKPWvPMmk4dxrGOLFQbkK90A=="
url="http://localhost:8086"
client = influxdb_client.InfluxDBClient(
    url=url,
    token=token,
    org=org
)

write_api = client.write_api(write_options=SYNCHRONOUS)
#Receive and send data.
flag = False
while True:
    if flag == False:
        #Serial comunication
        while True:
            try:
                puertos = puertos_seriales()
                puerto = puertos[0]
                puertoSerial = serial.Serial(puerto, 9600)
                time.sleep(2)
                break
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(e)
        flag=True
    try:
        msg = json.loads(puertoSerial.readline())
        #debug
        #print("Este es mensaje que mando el arduino-->",msg)
        '''
        msg = {
            "type" : "data",
            "id" : name_host,
            "group":
            [
                {
                    "sensor":name_sensor,
                    "measure":variables_fisica,
                    "value":valor_medida
                },
                {
                    "sensor":name_sensor,
                    "medida":variables_fisica,
                    "value":valor_medida
                },
                {
                    "sensor":name_sensor,
                    "medida":variables_fisica,
                    "value":valor_medida
                }
            ]
        }
        msg = {
            "type":"nodes",
            "id":"name_host",
            "devices":[
                device_1,
                devic_2,
                device_n
            ]
            "nodes":numberOfnodes
        }
        '''
        if msg["type"]=="data":
            send_data()
        elif msg["type"]=="nodes":
            send_node()
        puertoSerial.flush();
    except KeyboardInterrupt:
        break
    except Exception as e:
        print(e,' type->' ,type(e))
        flag = False
puertoSerial.close()
