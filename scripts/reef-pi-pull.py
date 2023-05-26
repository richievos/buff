#!/usr/bin/env python3

import json
import http.client
import sys


host = sys.argv[1]
tank_name = sys.argv[2]
output_file = sys.argv[3]

conn = http.client.HTTPConnection(host)
conn.request("GET", "/readings.json")
response = conn.getresponse()

if response.status != 200:
    print("Failed!")
    sys.exit(1)


response_body = json.loads(response.read())

most_recent_reading = next((r for r in response_body["readings"] if r["title"] == tank_name))

if most_recent_reading:
    with open(output_file, "w") as f:
        f.write(str(round(most_recent_reading["alkReadingDKH"], 1)))

