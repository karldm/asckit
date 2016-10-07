# ASCKit
Air Solar Controller - Kit for arduino yun
## Overview
This thermostat provides an optimal control of an air solar ventilation and a single heated zone to reduce the primary energy needs by 30 to 60 %. Connected to your local network, an embedded Webserver provides an user-friendly interface accessible by most browsers from a computer or a mobile device.  The heating needs can be scheduled for 7 days by the web interface.  For user applications, two temperature sensors and one relay can be remotely monitored and controlled.
## Specifications
* External Micro-USB plug or any regulated power supply 5V/2A
* Main heater controlled by a single relay (SPDT – single latch 5V coil - 250VAC—10A)
* Air solar ventilation controlled by a relay (SPDT – single latch 5V coil - 250VAC—10A) (the thermostat may be used without an air solar ventilation, in that case it is a classical single zone thermostat)
* One user-controlled relay (SPDT – single latch 5V coil)
* Two numerical temperature sensors for user applications (-50/+125 °C)
* Heated zone measurement accuracy (DHT22 sensor) [temperature +/- 0.2 °C, humidity +/- 1%]
* Network connection by Wifi or Ethernet cable
* Embedded Webserver (Linino on Arduino Yun)
* REST requests format for parameters access (compatible with most modern Home Automation systems)
