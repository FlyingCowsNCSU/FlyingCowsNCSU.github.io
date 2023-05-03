Here is the link to the hardware set up for the camera trap. 

The physical camera setup is near the bottom of the page:
https://randomnerdtutorials.com/esp32-cam-pir-motion-detector-photo-capture/

Within the camera trap is the protoboard that is made up using the circuit that is referred to in the above link. The circuit is made up of ESP32-CAM, connected to a MOSFET, a 1K resistor, a 10K resistor, a 5V voltage source, and a PIR sensor. The PIR sensor is set off once motion is detected in front of it which triggers the ESP32-CAM to take a photo. 

The camera trap enclosure was setup inside of a waterproof box that housed all necessary electronics. These necessary electronics included a 12 V battery, the Wanderer Solar Charge Controller, and the protoboard. The box has a connection port on the side that is used to connect the solar panel, used to recharge the battery, to the Wanderer. The Wanderer handles the transfer of power from the solar panel which charges the battery, which in turn powers the protoboard. An incident occured with the Wanderer accidentally supplying the full 12 V that the battery provides to the protoboard thus destroying the ESP32-CAM and its SD card, to prevent this from happening again, a USB cable was spliced to the cable supplying power to the protoboard and the USB was connected to the 5V power source provided by the Wanderer.
