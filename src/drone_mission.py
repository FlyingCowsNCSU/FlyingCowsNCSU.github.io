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
    (35.727491, -78.697359, 4), # Camera location 1 (edge of the field)
]

def start_socket_server():
    # Create a TCP socket object
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Bind the socket to a local address and port
    server_socket.bind(("", SOCKET_PORT))
    # Start listening for incoming connections, allowing up to one client in the queue
    server_socket.listen(1)

    # Set a timeout for the server socket
    server_socket.settimeout(60)  # Set a timeout of 60 seconds

    try:
        # Wait for a client to connect to the server
        client_socket, _ = server_socket.accept()
    except socket.timeout:
        # If no client connects within the timeout period, print a message and close the server socket
        print("No client connection within 60 seconds.")
        server_socket.close()
        return

    # Initialize variables for tracking received image data and the number of images received
    image_counter = 0
    received_data = b""

    # Continuously receive data from the client until a timeout or no data is received
    while True:
        try:
            # Receive data from the client in chunks of size BUFFER_SIZE
            data = client_socket.recv(BUFFER_SIZE)
        except socket.timeout:
            # If no data is received within the timeout period, print a message and exit the loop
            print("No data received within 60 seconds.")
            break
        if not data:
            # If no data is received, print a message and exit the loop
            print("No Data Received")
            break
        # Append the received data to the total received data
        received_data += data

    # Split the received data into segments delimited by the bytes b"BEGIN"
    image_data_segments = received_data.split(b"BEGIN")
    # For each segment, extract the image data between b"BEGIN" and b"DONE", and save it as a file
    for segment in image_data_segments[1:]:
        # Extract the image data between b"BEGIN" and b"DONE"
        image_data = segment.split(b"DONE")[0]
        # Generate a unique file name using the current timestamp and an incrementing counter
        image_name = f"received_image_{int(time.time())}_{image_counter}.jpg"
        # Set the file path to the "images_received" directory
        image_path = os.path.join("images_received", image_name)
        # Write the image data to a file in binary mode
        with open(image_path, "wb") as image_file:
            image_file.write(image_data)
        # Print a message indicating that an image was received and saved
        print(f"Received image {image_name} from client.")
        # Increment the image counter
        image_counter += 1

    # Close the client socket and server socket objects
    client_socket.close()
    server_socket.close()

def create_images_received_folder():
    # Create images_received folder if it doesn't exist
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
    # Go to coordinates
    location = LocationGlobalRelative(coordinates[0], coordinates[1], coordinates[2])
    vehicle.simple_goto(location)
    while distanceToWaypoint(vehicle, location) > WAYPOINT_LIMIT:
        time.sleep(1)
    
    print("Starting socket server at current location...")
    # Start socket server
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