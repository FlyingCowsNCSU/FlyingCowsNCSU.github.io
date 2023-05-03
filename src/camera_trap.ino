/* Camera Trap Program ------------------------------------------------
 *    Using the ESP32-CAM, starts the camera in low-power mode.
 *    Wakes up triggered by a GPIO pin connected to a PIR sensor, then
 *        takes a picture and saves it to the SD card.
 *    Every 10 seconds, wakes up and tries to establish a WiFi 
 *        connection with the drone. If connection succeeds, sends the 
 *        images to the drone and deletes them from the SD card. Else,
 *        goes back to sleep. 
 * 
 * Heavily references Rui Santos' tutorials from RandomNerdTutorials.com
 *
 */

#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include "driver/rtc_io.h"     // Access RTC data
#include <EEPROM.h>            // read and write from flash memory
#include "WiFi.h"              // WiFi client/server connections
#include <WiFiClientSecure.h>  // Includes features to send many bytes in a buffer
#include <ssl_client.h>        // WiFiClientSecure dependency
#include <string>

// RTC Data/EEPROM configurations -------------------------------------
#define EEPROM_SIZE 1
RTC_DATA_ATTR uint8_t bootCount = 0;        /* RTC data will hold its value in deep-sleep mode */
RTC_DATA_ATTR int intermediateCount = 0;
int pictureNumber = 0;

// Time sleep definitions ---------------------------------------------

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  10        /* Time ESP32 will go to sleep (in seconds) */

// Pin definitions ----------------------------------------------------
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// WiFi connection info -----------------------------------------------
const char* ssid = "flyingcows-desktop";
const char* password = "password";           
const uint16_t port = 8090;                
const char* host = "10.42.0.1";        


// TakePicture() function ---------------------------------------------
void takePicture() {

  // TakePicture() configures the camera, takes a picture, and writes it to the SD card.
  // This function is called when the camera trap wakes up from the GPIO 13 interrupt, which
  //     occurs when the PIR sensor has detected motion.

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
 
  // configure camera pins 
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // setup for pin 4 camera flash
  pinMode(4, INPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_dis(GPIO_NUM_4);
 
  // picture configurations -- format, resolution, etc.
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
 
  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }


  // Start the SD card  
  Serial.println("Starting SD Card");
 
  delay(500);
  if(!SD_MMC.begin()){
    Serial.println("SD Card Mount Failed");
    //return;
  }
 
  uint8_t cardType = SD_MMC.cardType();
  if(cardType == CARD_NONE){
    Serial.println("No SD Card attached");
    return;
  }
  

  // Initialize frame buffer
  camera_fb_t * fb = NULL;
 
  // Take Picture with Camera
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  // retrieve picture number from EEPROM
  EEPROM.begin(EEPROM_SIZE);
  pictureNumber = EEPROM.read(0) + 1;
 
  // Path where new picture will be saved in SD Card
  String path = "/pictures/picture" + String(pictureNumber) +".jpg";
 
  // Access file system and write image to SD card
  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());
 
  File file = fs.open(path.c_str(), FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file in writing mode");
  }
  else {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: %s\n", path.c_str());
    EEPROM.write(0, pictureNumber);
    EEPROM.commit();
  }


  file.close(); // close the image file
  esp_camera_fb_return(fb); // return the frame buffer memory
  SD_MMC.end(); // close the SD card
  
  delay(1000);
  
  // Turns off the ESP32-CAM white on-board LED (flash) connected to GPIO 4
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  rtc_gpio_hold_en(GPIO_NUM_4);

} 


// PollDrone() function -----------------------------------------------
void pollDrone() {

  // pollDrone() attempts to establish a connection to the server on the drone.
  // If successful, it sends the images in the pictures directory on the SD card
  //    over Wi-Fi.
  // This function is called when the camera trap wakes up from the 10-second timer interrupt.

  uint8_t gotWifi = 0;
  uint8_t gotSocket = 0;
  uint8_t sentImages = 0;
  Serial.println("In pollDrone() fn");

// Connect to WiFi --------------------------------------------
  /* Connecting to the drone happens in two steps. First, the camera trap attempts to connect to
  *  a Wi-Fi network. If this connection fails, the camera trap goes back to deep sleep.
  */

  WiFi.begin(ssid, password);                // Connect to WiFi network
  for(int i = 0; i < 20; i++){               // Retry connection for 10 seconds. This interval can be decreased to reduce overall power consumption.
    if(WiFi.status() != WL_CONNECTED){              // For now, we leave it at 10 seconds for testing purposes.
      Serial.print("Trying to connect, i = ");
      Serial.println(i);
      delay(500);
    }
    else break;
  }
  if (WiFi.status()==WL_CONNECTED) {
  
    Serial.print("WiFi connected with IP: ");
    Serial.println(WiFi.localIP());             // Print the IP address of the device connected to the WiFi network

  // Mount SD Card ----------------------------------------------
    /* The SD card read mode appears to fail if pin 4 is ever set to OUTPUT. 
    *  As a temporary solution, we're leaving pin 4 off in this function and using the SD card's 1 pin mode.
    *  The read speed is barely affected; mounting/unmounting the SD card takes a bit longer.
    */

    Serial.println("Starting SD Card");
  
    delay(500);
    if(!SD_MMC.begin("/sdcard", true, false)){    // to go back to 4 pin mode, change the 2nd argument to "true"
      Serial.println("SD Card Mount Failed");
    }

    uint8_t cardType = SD_MMC.cardType();
    if(cardType == CARD_NONE){
      Serial.println("No SD Card attached");
      return;
    }


    // 

    // Connect to client ------------------------------------------
    /* The second phase of connecting to Wi-Fi is establishing a connection from the client (camera trap) to the drone (server). */
    WiFiClient client;                         // Create a new instance of the WiFiClient class

    // Try to connect for 10 seconds. If the connection fails, the camera trap returns to deep sleep.
    int isConn = 0;
    for (int i=0; i<10; i++) {
      /* if (client.connect(WiFi.localIP(), port)) {    // This code is strictly not necessary and was included for debugging purposes
        Serial.println("Connected on localip");         // as we were having issues with connecting to the correct IP address. 
        isConn = 1;                                     // Since this debugging code was running on the camera trap in the final demo, we've left it here.
        break;                                          
      }
      delay(1000); */
      if (client.connect(host, port)) {
        Serial.println("Connected on given ip");
        isConn = 1;
        break;
      }
      delay(1000);
      Serial.println("Connection failed. Retrying...");      
    }
    
    if (!isConn) return;


  // Open File --------------------------------------------------

    fs::FS &fs = SD_MMC; // look at the SD card's files

    File root = fs.open("/pictures"); // "root" here is a pointer to the /pictures folder on the SD card
    if(!root){
      Serial.println("Failed to open directory");
      return;
    }
  
    // return if "root" contains something unexpected
    if(!root.isDirectory()){
      Serial.println("Not a directory");
      return;
    }

    // open the first picture file
    File file = root.openNextFile();  // defaults to read mode

    // iterate through the picture files and send each one
    while(file){
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());

      // read 4096 bytes into a buffer, then send the whole buffer at once using the client.write() function
      // This is a significant performance increase over calling client.write() once per byte.
      #define BUFFER_SIZE (4096)

      Serial.println("File Size:");
      Serial.print(file.size());

      uint8_t buffer[BUFFER_SIZE];

      // Each image is preceded by the string "BEGIN" and followed by the string "DONE". The drone expects these strings
      // to mark the beginning and end of each image.
      char begin_string[6] = "BEGIN"; 
      client.write(begin_string, 5);  // The drone *DOES NOT* expect a C-style string with a null terminator; *only* write the actual 5 characters.


      // Send the image data
      int packages_sent = 0;
      while (file.available()) {
        int n = file.available();
        if (n >= BUFFER_SIZE) {
          file.read(buffer, BUFFER_SIZE);     // Use readBytes() for char, read() for uint8_t; client.write() works with uint8_t
          client.write(buffer, BUFFER_SIZE);
          packages_sent++;
          Serial.print("Packages sent: ");
          Serial.println(packages_sent);
          Serial.print("    size-pos = ");
          Serial.println(n);      
        }
        // The last package will be <=BUFFER_SIZE bytes
        else {
          file.read(buffer, n); // read remainder
          client.write(buffer, n);
          Serial.print("Sending last package, size = ");
          Serial.println(n);
        }
      }

      char done_string[5] = "DONE"; // Write done string
      client.write(done_string, 4);

      // Close file, get next one -------------------------------------
      file.close();                           // Close the image file
      delay(250);
      file = root.openNextFile();

    }

    delay(100);
    // no more images to send
    char eot_string[20] = "end of transmission";  // The drone expects this string after all images are sent.
    client.write(eot_string, 20);
    

    // Close SD card, disconnect from Wi-Fi server
    SD_MMC.end();
    client.stop();                            

    Serial.println("Disconnecting...");
    
  }

  // If the camera trap never connected to Wi-Fi, return
  else Serial.println("Connection failed");


}


// Setup() ------------------------------------------------------------
void setup() {

  /* setup() runs every time the ESP32-CAM wakes up from deep sleep.
  *  This function contains a switch statement that selects the action to take based on 
  *  which interrupt caused the ESP32-CAM to wake up.
  */

  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // get the wakeup reason, or which interrupt triggered the wakeup
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  // decide which action to take
  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : {  // wakeup was caused by an external GPIO pin -- for us,
                                    // pin 13 is connected to the PIR sensor
      takePicture();  // take a picture
    } break;

    case ESP_SLEEP_WAKEUP_TIMER : { // wakeup was caused by 10 second timer interrupt
      pollDrone();  // try to connect to drone

      // Every time the camera trap wakes up from the timer interrupt, write the letter 'A' to a logfile.
      // Simple debugging method to see how many times the camera trap woke up.
      SD_MMC.begin("/sdcard", true, false);
      fs::FS &fs = SD_MMC;
      File log = fs.open("/log.txt", FILE_APPEND);
      if (!log){
        Serial.println("Failed to open file");
      }
      else {
        uint8_t a = 0x41;
        log.write(&a, 1);        
      }

    } break;

    // default case for unexpected wakeup reason
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }

  // enable interrupts
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0); // set up PIR gpio interrupt
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // timed interrupt

  // go back to sleep
  Serial.println("Going to sleep now");
  delay(1000);
  esp_deep_sleep_start();
  Serial.println("This will never be printed");

}

void loop() {
  // Nothing runs in the while loop, since the ESP32 is in deep sleep. All of the camera trap's
  // functionality is implemented in setup().
}
