## Drone Mission Documentation

This script is a Python program that controls a drone to visit a list of specified waypoints and receive images from a client while the drone is hovering at each waypoint. The drone will then save these images to a local folder.

### Dependencies
The following dependencies are required for this program:
- `dronekit`
- `pymavlink`
- `numpy`
- `opencv-python` (cv2)
- `argparse`

### Usage
To use this script, run the following command:
`python drone_mission.py --connect <connection_string>`


Replace `<connection_string>` with the appropriate connection string for your drone.

### Constants

The following constants are defined in the script:

- `TARGET_ALTITUDE`: The desired altitude (in meters) to take off to.
- `ALTITUDE_REACH_THRESHOLD`: The portion of `TARGET_ALTITUDE` at which the drone will break from the takeoff loop.
- `WAYPOINT_LIMIT`: The maximum distance (in meters) from a waypoint at which the drone is considered to have "reached" the waypoint.
- `SOCKET_PORT`: The port for the socket server.
- `BUFFER_SIZE`: The buffer size for receiving images.
- `SERVER_SHUTDOWN_MSG`: The server shutdown message.
- `SERVER_TIMEOUT`: The server timeout (in seconds).

### Waypoints
To add waypoints, update the `coordinates_list` list with the desired latitude, longitude, and altitude of each waypoint.

### Functions

The following functions are defined in the script:

- `start_socket_server()`: Starts a socket server to receive images from a client while the drone is hovering at a waypoint.
- `create_images_received_folder()`: Creates a folder named "images_received" to store the received images.
- `get_distance_metres(aLocation1, aLocation2)`: Returns the ground distance in meters between two `LocationGlobal` objects.
- `distanceToWaypoint(vehicle, coordinates)`: Returns the distance between the vehicle and the specified coordinates.

### Workflow

1. The script connects to the drone using the connection string provided as an argument.
2. The drone waits for the safety pilot to arm it.
3. The drone takes off to the `TARGET_ALTITUDE`.
4. The drone visits each waypoint specified in `coordinates_list` and hovers at the waypoint.
5. While hovering, the drone starts a socket server to receive images from a client.
6. The drone saves the received images to the "images_received" folder.
7. After visiting all waypoints, the drone returns to the launch site and lands.
8. The script closes the connection to the drone.

