import influxdb_client
from influxdb_client.client.write_api import SYNCHRONOUS
import time, serial, json

def send_data():
    '''
        This functions send the data of the sensors to the server
    '''
    for i in msg["group"]:
        '''
        try:
            p = influxdb_client.Point(i["sensor"]).tag("Host",msg["id"]).field(i["measure"], float(i["value"]))
            print("writing data --> ",p)
            write_api.write(bucket=bucket, org=org, record=p)
        except ValueError: 
            p = influxdb_client.Point(i["sensor"]).tag("Host",msg["id"]).field(i["measure"], float(i["value"]))
            print("writing data --> ",p)
            write_api.write(bucket=bucket, org=org, record=p)
        '''
        p = influxdb_client.Point(i["sensor"]).tag("host",msg["id"]).field(i["measure"], float(i["value"]))
        print("writing data --> ",p)
        write_api.write(bucket=bucket, org=org, record=p)
    return None

def send_node():
    '''
        This functions send the number of slaves in the mesh to the server
    '''
    for i in msg["devices"]:
        p = influxdb_client.Point("connections").tag("host", str(i)).field("status",1)
        p = influxdb_client.Point("connections").tag("host", msg["id"]).field("nodes",int(msg["nodes"]))
        print("writing node --> ",p)
        write_api.write(bucket=bucket, org=org, record=p)
    return None

#Serial comunication
puertoSerial = serial.Serial('/dev/tty.usbserial-0001', 9600)
time.sleep(2)
#Influx info
bucket = "mesh"
org = "karma"
token="QGk5MP-jHwbQtx_eiyFVAEgEDUbae8282XKDNRkOs-vJ7r6ozz9CIvabD9TxdZrt_iZa-e1L1lMHJxCCq7FZhQ=="
url="http://localhost:8086"
client = influxdb_client.InfluxDBClient(
    url=url,
    token=token,
    org=org
)

write_api = client.write_api(write_options=SYNCHRONOUS)
#Receive and send data.
while True:
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
    except KeyboardInterrupt:
        break
    except:
        print("Unknow data")
puertoSerial.close()
