from influxdb_client.client.write_api import SYNCHRONOUS
from dash import Dash, html, dcc, Input, Output
import plotly.express as px
import pandas as pd
import influxdb_client
import time, serial, json

app = Dash(__name__)

def query(sensor,measure):
  s = "\""+sensor+"\""
  m = "\""+measure+"\""
  query= f'from(bucket: "mesh")\
    |> range(start:-30d)\
    |> filter(fn: (r) => r["_measurement"] == {s})\
    |> filter(fn: (r) => r["Host"] == "echo")\
    |> filter(fn: (r) => r["_field"] == {m})\
    |>group(columns:["Host"])\
    |> pivot(rowKey: ["_time"], columnKey: ["_field"], valueColumn: "_value")'

  df = query_api.query_data_frame(org=org, query=query)
  return df

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

all_sensors = {
  'dht11' :['humidity','temperature'],
  'pir' : ['detections'],
}

app.layout = html.Div([
    html.Div(children=[
      dcc.Dropdown(
        #INGRESAR LAS POSIBLES OPCIONES
        list(all_sensors.keys()),
        'dht11',
        id="sensors"
        ),
        dcc.Dropdown(
          id="measures"
        ),
      dcc.Graph(id="data")
    ])
])
#Este callback actualiza los datos del segundo dropdown
@app.callback(
  Output('measures','options'),
  Input('sensors','value')
)
def update_dropdown(selected_sensor):
  return [{'label': i, 'value': i} for i in all_sensors[selected_sensor]]

#Este callback pone una ocpión por default en el segundo dropdown, NO LO HACE, PERO DEBERIA XD
@app.callback(
    Output('measures', 'value'),
    Input('measures', 'options'))
def set_measure_value(available_options):
    #print(available_options[0]['value'])
    return (available_options[0]['value'])

#Este callback actualiza la gráfica 
@app.callback(
    Output(component_id='data', component_property='figure'),
    Input(component_id='sensors', component_property='value'),
    Input('measures','value')
)
def update_graph(sensor,measure):
    df1=query(sensor,measure)
    fig = px.scatter(df1,x=df1["_time"],y=df1[measure],
    labels=dict(_time="Time"))
    fig.update_layout(transition_duration=200)
    return fig




if __name__ == '__main__':
    app.run_server(debug=True)