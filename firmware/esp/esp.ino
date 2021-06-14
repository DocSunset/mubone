// wifi manager includes
#include "ESP8266WiFi.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> 

IPAddress device_address;
IPAddress output_address;
void setup()
{
    WiFiManager wifiman;
    wifiman.autoConnect("mubone");
    device_address = WiFi.localIP();
    output_address = device_address;
    output_address[3] = 255; // broadcast;
}

void loop()
{
}

