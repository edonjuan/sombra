from influxdb_client.client.write_api import SYNCHRONOUS
from dash import Dash, html, dcc
import plotly.express as px
import pandas as pd
import influxdb_client
import time, serial, json

#DATABASE INFORMATION
bucket = "mesh"
org = "karma"
token="QGk5MP-jHwbQtx_eiyFVAEgEDUbae8282XKDNRkOs-vJ7r6ozz9CIvabD9TxdZrt_iZa-e1L1lMHJxCCq7FZhQ=="
url="http://localhost:8086"

client = influxdb_client.InfluxDBClient(
    url=url,
    token=token,
    org=org
)

query_api = client.query_api()

def query():
  m = "\""+'temperature'+"\""
  s = "\""+'dht11'+"\""
  query= f'from(bucket: "mesh")\
    |> range(start:-30d)\
    |> filter(fn: (r) => r["_measurement"] == {s})\
    |> filter(fn: (r) => r["Host"] == "echo")\
    |> filter(fn: (r) => r["_field"] == {m})\
    |>group(columns:["Host"])\
    |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")\
    |> yield(name:"DHT")'
  df = query_api.query_data_frame(org=org, query=query)
  return df
df1 = query()
if df1.empty:
  print("lel")

print(df1)



