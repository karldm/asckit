# Copyright (c) 2017 Renergia.fr
# by karldm, Feb 2017

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

#
# Sending data to Adafruit IO with the REST
# Adapted from simpleMy.py and bridgeclient_example.py
#
# Installation & usage
# ********************
# Should be installed in /osjs/dist/renergia/python
# insert your IO Adafruit key in line #59
# Use crontab to launch the script every 10 min
# log on yun or use the interface
#
# > crontab -e
#
# add line:
# */10 * * * * python /osjs/dist/renergia/python/adafeedsMy.py
#
# > /etc/init.d/cron start
# > /etc/init.d/cron enable
#
# 
# v1-20170204
#   First release
#   Some issues if the bridge is not responding
#   Try /data/get 
#   > nhil socket message
#   > restart the Yun
#
import time
# Import Adafruit IO REST client.
from Adafruit_IO import Client

# bridge setup
import sys
sys.path.insert(0, '/usr/lib/python2.7/bridge/')
from bridgeclient import BridgeClient as bridgeclient
client = bridgeclient()

# Set to your Adafruit IO key and uncomment the line
#ADAFRUIT_IO_KEY = 'xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx'

# Create an instance of the REST client.
aio = Client(ADAFRUIT_IO_KEY)

#
# begin()
print "===> BEGIN <==="
client.begin()
	
# Get all data from bridge
all = client.getall()
print all
	
# 
print 'Value assigned to tamb is ' + all['tamb']
print 'Value assigned to hamb is ' + all['hamb']
	
#
print 'Value assigned to swusr1 is ' + all['swusr1']
print 'Value assigned to swusr2 is ' + all['swusr2']
	
#
aio.send('tamb', float(all['tamb']))
aio.send('hamb', float(all['hamb']))
	
#
if int(all['swusr1']) == 1:
	aio.send('swusr1', 'ON')
else:
	aio.send('swusr1', 'OFF')
		
#
if int(all['swusr2']) == 1:
	aio.send('swusr2', 'ON')
else:
	aio.send('swusr2', 'OFF')
	
# close()
print "===> CLOSE <==="
client.close()