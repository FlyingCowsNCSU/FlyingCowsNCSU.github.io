## Drone Mission Documentation

This script is a Python program that controls a drone to visit a list of specified waypoints and receive images from a client while the drone is hovering at each waypoint. The drone will then save these images to a local folder.

**Note**: Before executing this script, ensure that the Raspberry Pi running on Ubuntu 22.04 LTS is set to SoftAP mode by enabling the hotspot in the settings. This will disable the all active Wi-Fi connections.

### Dependencies
The following dependencies are required for this program:
- `dronekit`
- `pymavlink`
- `argparse`

### Usage
To use this script, run the following command:

```bash
python drone_mission.py --connect <connection_string>
```

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




## Camera Trap Documentation

`camera_trap.ino` is a program written in Arduino C++, containing all of the code for the camera trap. This code is downloaded onto the ESP32-CAM microcontroller, which can then be placed in the field. For the majority of the time, the ESP32-CAM is in its lowest power deep-sleep mode. There are two interrupts configured that wake up the ESP32: an external GPIO interrupt, and a timer interrupt. These allow the ESP32 to wake up when motion is detected, or after a set time interval to check for the drone's presence. 

### Dependencies
This code can be downloaded onto the ESP32-CAM from the Arduino IDE. Instructions to install the IDE and setup the required board packages can be found [here](https://flyingcowsncsu.github.io/). 

The following Arduino libraries are used. Installation instructions for these are also found at the link above.
- `Arduino.h`: standard Arduino library
- `EEPROM.h`: access EEPROM memory
- `esp_camera.h`: board-specific info
- `FS.h`: file system on SD card
- RTC data and brownout libraries: `rtc_cntl_reg.h`, `rtc_io.h`, `soc.h`
- `SD_MMC.h`: SD card
- `string.h`
- `Wi-Fi.h`: basic Wi-Fi functionality
- `WiFiClientSecure.h`: send many bytes at once. Also relies on `ssl_client.h`.

### Functions
The following functions are defined:

- `takePicture()`: takes a picture with the attached camera. Called when the ESP32 wakes up from an external GPIO interrupt. GPIO pin 13 is connected to the PIR sensor.
- `pollDrone()`: wakes up every 10 seconds and tries to connect to the drone's Wi-Fi. If connection is successful, sends all images on the SD card.
- `setup()`: runs on wakeup. Based on which interrupt triggered the wakeup, calls either `takePicture()` or `pollDrone()`.
- `loop()`: dummy while-loop function present in all Arduino scripts; runs nothing
