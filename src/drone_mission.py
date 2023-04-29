#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Drone Mission Script

This script controls a drone to visit a list of specified waypoints and receive
images from a client while the drone is hovering at each waypoint. The drone
will then save these images to a local folder.

Usage:
    python drone_mission.py --connect <connection_string>

Dependencies:
    - dronekit
    - pymavlink
    - numpy
    - opencv-python (cv2)
    - argparse

Please refer to the README for more information on usage, configuration, and workflow.
"""

import math
import time
import sys
import os
import cv2
import socket
import numpy as np
import argparse
import threading
from dronekit import connect, VehicleMode, LocationGlobalRelative
from pymavlink import mavutil

# Desired altitude (in meters) to takeoff to
TARGET_ALTITUDE = 20
# Portion of TARGET_ALTITUDE at which we will break from takeoff loop
ALTITUDE_REACH_THRESHOLD = 0.95
# Maximum distance (in meters) from waypoint at which drone has "reached" waypoint
WAYPOINT_LIMIT = 1
# Socket server port
SOCKET_PORT = 8090
# Buffer size for receiving images
BUFFER_SIZE = 4096
# Server shutdown message
SERVER_SHUTDOWN_MSG = "end of transmission"
# Server timeout
SERVER_TIMEOUT = 60 # 60 seconds

# List of coordinates to visit (latitude, longitude, altitude)
coordinates_list = [
    # Add coordinates here in the format (latitude, longitude, TARGET_ALTITUDE)
    # e.g., (37.422231, -122.085563, TARGET_ALTITUDE),
    (35.727491, -78.697359, 4), # Dollar Store
]

def start_socket_server():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(("", SOCKET_PORT))
    server_socket.listen(1)

    server_socket.settimeout(60)  # Set a timeout of 60 seconds

    try:
        client_socket, _ = server_socket.accept()
    except socket.timeout:
        print("No client connection within 60 seconds.")
        server_socket.close()
        return

    image_counter = 0
    received_data = b""
    while True:
        try:
            data = client_socket.recv(BUFFER_SIZE)
        except socket.timeout:
            print("No data received within 60 seconds.")
            break
        if not data:
            print("No Data Received")
            break
        received_data += data

    image_data_segments = received_data.split(b"BEGIN")
    for segment in image_data_segments[1:]:
        image_data = segment.split(b"DONE")[0]
        image_name = f"received_image_{int(time.time())}_{image_counter}.jpg"
        image_path = os.path.join("images_received", image_name)
        with open(image_path, "wb") as image_file:
            image_file.write(image_data)
        print(f"Received image {image_name} from client.")
        image_counter += 1

    client_socket.close()
    server_socket.close()

def create_images_received_folder():
    if not os.path.exists("images_received"):
        os.makedirs("images_received")

create_images_received_folder()

def get_distance_metres(aLocation1, aLocation2):
    """
    Returns the ground distance in metres between two LocationGlobal objects.
    """
    dlat = aLocation2.lat - aLocation1.lat
    dlong = aLocation2.lon - aLocation1.lon
    return math.sqrt((dlat*dlat) + (dlong*dlong)) * 1.113195e5

def distanceToWaypoint(vehicle, coordinates):
    """
    Returns distance between vehicle and specified coordinates
    """
    distance = get_distance_metres(vehicle.location.global_frame, coordinates)
    return distance

# Set up option parsing to get connection string and mission plan file
parser = argparse.ArgumentParser(description='Commands vehicle using vehicle.simple_goto.')
parser.add_argument('--connect', help="Vehicle connection target string.")
args = parser.parse_args()

# aquire connection_string
connection_string = args.connect

# Exit if no connection string specified
if not connection_string:
    sys.exit('Please specify connection string')

# Connect to the Vehicle
print('Connecting to vehicle on: %s' % connection_string)
vehicle = connect(connection_string, wait_ready=False)

print('Successfully connected to vehicle')

# Wait for pilot before proceeding
print('Waiting for safety pilot to arm...')

# Wait until safety pilot arms drone
while not vehicle.armed:
    time.sleep(1)

print('Armed...')
vehicle.mode = VehicleMode("GUIDED")

# Takeoff to target altitude
print("Taking off!")
vehicle.simple_takeoff(TARGET_ALTITUDE)

while True:
    # Break just below target altitude.
    if vehicle.location.global_relative_frame.alt >= TARGET_ALTITUDE * 0.95:
        print("Reached target altitude")
        break
    time.sleep(1)

print('Visiting coordinates...')
for coordinates in coordinates_list:
    location = LocationGlobalRelative(coordinates[0], coordinates[1], coordinates[2])
    vehicle.simple_goto(location)
    while distanceToWaypoint(vehicle, location) > WAYPOINT_LIMIT:
        time.sleep(1)
    
    print("Starting socket server at current location...")
    start_socket_server()
    print("Proceeding to the next location...")

print('Done visiting coordinates!')

# Return to launch
print("Returning to launch")
vehicle.mode = VehicleMode("RTL")

# Wait for vehicle to land
print("Waiting for vehicle to land...")
while vehicle.armed:
    time.sleep(1)

print("Done!")

# Close vehicle object before exiting script
vehicle.close()