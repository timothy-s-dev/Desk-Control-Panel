# Desk Control Panel

This is a PlatformIO project intended for deployment on an ESP32 in a custom
control panel/macro-pad. The hardware features include five buttons, a dial 
(with a 6th button) and a small oled screen. The dial allows navigation of a
nested menu and selection of options within it.

The device connects to WiFi using the WiFiManager library (and also accepts
MQTT configuration through the same portal). Once connected it sends messages
on MQTT topics in response to button presses, and selections made with the dial.

The intent is for it to connect to an MQTT broker as part of a Home Assistant
system, to control other devices on the network via HA automations.