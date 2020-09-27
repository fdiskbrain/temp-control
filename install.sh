#!/bin/sh

sudo echo 'hdmi_force_hotplug=1' >> /boot/config.txt

cd /home/pi/temp_control
gcc -o temp_control temp_control.c ssd1306_i2c.c -lwiringPi

cd /home/pi/.config/
mkdir /home/pi/.config/autostart
cd /home/pi/.config/autostart
cp /home/pi/temp_control/start.desktop /home/pi/.config/autostart/
echo 'install ok!'
