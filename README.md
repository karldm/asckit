# ASCKit
Air Solar Controller - Kit for arduino yun

# Overview
This thermostat provides an optimal control of an air solar ventilation and a single heated zone to reduce the primary energy needs by 30 to 60 %. Connected to your local network, an embedded Webserver provides an user-friendly interface accessible by most browsers from a computer or a mobile device.  The heating needs can be scheduled for 7 days by the web interface.  For user applications, two temperature sensors and one relay can be remotely monitored and controlled.

# Specifications
* External Micro-USB plug or any regulated power supply 5V/2A
* Main heater controlled by a single relay (SPDT – single latch 5V coil - 250VAC—10A)
* Air solar ventilation controlled by a relay (SPDT – single latch 5V coil - 250VAC—10A) (the thermostat may be used without an air solar ventilation, in that case it is a classical single zone thermostat)
* One user-controlled relay (SPDT – single latch 5V coil)
* Two numerical temperature sensors for user applications (-50/+125 °C)
* Heated zone measurement accuracy (DHT22 sensor) [temperature +/- 0.2 °C, humidity +/- 1%]
* Network connection by Wifi or Ethernet cable
* Embedded Webserver (Linino on Arduino Yun)
* REST requests format for parameters access (compatible with most modern Home Automation systems)

# Documentation
You will get the relevant documentation in the /docs directory
* software intallation
* hardware installation
* user manual
* reference manual for more advance usage and modifications

# Licences
This work has been done following the free-makers spirit and largely benefit from other works found in various Web sites. The licence choice gives you some rights for using and re-using the material given, both the sofware, the documentation and the hardware drawings.

## Software
The software is licence is the MIT, one of the most permissive (see http://choosealicense.com/licenses/). You are allowed to copy it, modifiy it, for personnal or even commercial usage. The only condition is to indicate the following copyright notice:

MIT License

Copyright (c) [2016] [renergia.fr]

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


## Hardware
Licencing the hardware is a more recent subject. See the discussion http://www.open-electronics.org/how-to-choose-your-open-source-hardware-license/. We have choosen the same licence as the Arduino products, i.e. CC-BY-SA. The SA letters means that you have to keep the same licence for derivative works. You will find a readable explanation of the licence at https://creativecommons.org/licenses/by-sa/4.0/, the legal code is avaibale at https://creativecommons.org/licenses/by-sa/4.0/legalcode.
