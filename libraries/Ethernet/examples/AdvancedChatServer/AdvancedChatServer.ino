/*
 Advanced Chat Server

 A more advanced server that distributes any incoming messages
 to all connected clients but the client the message comes from.
 To use, telnet to your device's IP address and type.

  Usage:
  1. Upload this sketch to your board.
  2. Make sure your board is connected to the network and note its IP address.
  3. From a computer on the same network, open a terminal and connect via Telnet:

  - On macOS or Linux (using netcat if telnet is not available):
      telnet <board_ip> 23
      # or, if telnet is missing:
      nc <board_ip> 23

  - On Windows (Command Prompt):
      telnet <board_ip> 23
      # If 'telnet' is not recognized, enable it in "Windows Features".

  4. Type a message and press Enter.
  Your message will be broadcast to all connected clients except you.

  Example:
  telnet 192.168.1.177 23

  Press CTRL + ] then 'quit' to exit Telnet.

 */

#include "ZephyrServer.h"
#include "ZephyrClient.h"
#include "ZephyrEthernet.h"

// The IP address will be dependent on your local network.
// gateway and subnet are optional:
IPAddress ip(192, 168, 1, 177);
IPAddress myDns(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);


// telnet defaults to port 23
ZephyrServer server(23);

ZephyrClient clients[8];

void setup() {
  // start serial port:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // in Zephyr system check if Ethernet is ready before proceeding to initialize
  Serial.print("Waiting for link on");
  while (Ethernet.linkStatus() != LinkON) {
      Serial.print(".");
      delay(100);
  }
  Serial.println();

  // initialize the Ethernet device
  Ethernet.begin(ip, myDns, gateway, subnet);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // start listening for clients
  server.begin();

  Serial.print("Chat server address:");
  Serial.println(Ethernet.localIP());
}

void loop() {
  // check for any new client connecting, and say hello (before any incoming data)
  ZephyrClient newClient = server.accept();
  if (newClient) {
    for (byte i=0; i < 8; i++) {
      if (!clients[i]) {
        Serial.print("We have a new client #");
        Serial.println(i);
        newClient.print("Hello, client number: ");
        newClient.println(i);
        // Once we "accept", the client is no longer tracked by EthernetServer
        // so we must store it into our list of clients
        clients[i] = newClient;
        break;
      }
    }
  }

  // check for incoming data from all clients
  for (byte i=0; i < 8; i++) {
    if (clients[i] && clients[i].available() > 0) {
      // read bytes from a client
      byte buffer[80];
      int count = clients[i].read(buffer, 80);
      // write the bytes to all other connected clients
      for (byte j=0; j < 8; j++) {
        if (j != i && clients[j].connected()) {
          clients[j].write(buffer, count);
        }
      }
    }
  }

  // stop any clients which disconnect
  for (byte i=0; i < 8; i++) {
    if (clients[i] && !clients[i].connected()) {
      Serial.print("disconnect client #");
      Serial.println(i);
      clients[i].stop();
    }
  }
}
