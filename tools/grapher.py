#!/usr/bin/env python3

import requests
import urllib
from pandas.io.json import json_normalize
import pandas as pd
import plotly.express as px

import dash
from dash.dependencies import Input, Output
from dash import dcc
from dash import html
import datetime

s = requests.Session()

def getTemperature(session):
	base_url = "http://coffee.local/api/v1"
	temperature_data = session.get(base_url + "/temp/raw")
	return temperature_data

def getPressure(session):
	base_url = "http://coffee.local/api/v1"
	pressure_data = session.get(base_url + "/pressure/raw")
	return pressure_data

app = dash.Dash()

app.layout = html.Div([
    dcc.Interval(id='my-interval', interval=100),
	dcc.Graph(id="plot-data-from-csv-x-graph"),
])

temperature = getTemperature(s)
pressure = getPressure(s)

result = pd.json_normalize(temperature.json())
result["point"] = 0

@app.callback(
	Output("plot-data-from-csv-x-graph", "figure"), 
	[Input('my-interval', 'n_intervals')])
def display_graph(n_intervals):
	temperature = getTemperature(s)
	pressure = getPressure(s)

	newTemperature = pd.json_normalize(temperature.json())
	newTemperature["point"] = n_intervals

	global result
	result = pd.concat([result, newTemperature])

	x, y = 'point', ' Temperature'

	fig = px.line(result, x=x, y=['current'], markers=True)    
	return fig

if __name__ == '__main__':
	app.run_server(debug=True, port=8080)
