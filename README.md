## Remote controller for PC
Able to remote boot and reset a computer by hooking into the motherboard front IO header pins.
Also features an 8 channel PWM array of open collector LED drivers for PC lighting.
Built off the RTOS SDK by Espressif for the ESP8266.  

### Completed
* Basic WiFi connectivity to local WLAN 
* Basic RESTful server
    * Setting and getting of PWM channels

### TODO 
#### Micontroller
* Add button/jumper for user initialisation on pre-boot
  * Connect to access point to configure settings
  * Connect to serial port to configure settings
>>* After restoring this button/jumper should execute user settings
    * Access point should be disabled after config
* Add onboard software for advanced control of 8 channel PWM LED controller
  * Add list of pre-set patterns which are addressable remotely
  * Add direct streaming of PWM data to microcontroller 
* Add support for remote boot and reset
  * Add corresponding URIs to server
* Add user authentication for server
* Add support for OTA (over the air) updates
* Add power management software to reduce power consumption when idle
  * Intelligently disable PWM if it is doing nothing
  * Look into deep sleep in SDK

#### Desktop/Mobile app
* Use the restful api 
  * Provide corresponding user authentication details
  * Autoconnect to the server
  * Should be able to connect over local WLAN or remotely through a static ip (port forwarding?)