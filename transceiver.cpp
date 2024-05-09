// =========================================================== //
// ===================== Transceiver Code ==================== //
// =========================================================== //

// This code is designed for the middle nodes in the network, currently 01.
// These are meant to receive data and transmit it to the next relay node/master node.
// Each node that is transmitting data to this node, needs an adress in definitions & an if-statement in the loop.
// If no if-statement is added it will not be possible to easily differentiate where the data was coming from using headers.
// For each node added in the network, adresses for the nodes in the code will need to be slightly adjusted for each node.


//============ Libraries ============// 
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24Network.h>
//===================================// 

//==== Radio Network Definitions ====// 
RF24 radio(7, 8);                // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 01;   // Address of this node in Octal format 
const uint16_t master00 = 00;    // Address of the master node in Octal format
const uint16_t node11 = 011;     // Adress to next node, more can be added in Octal format (021, 031, etc) 



// =================================================== //
// ====================== SETUP ====================== //
// =================================================== // 
void setup() { // Initiating all libraries 
  SPI.begin();
  radio.begin();
  network.begin(90, this_node);  //(channel, node address)
  Serial.begin(9600);            // Needed to visualize in serial monitor

  pinMode(3,OUTPUT); // Initializing LED
  for (int i = 0; i < 7; i++)
  {
    digitalWrite(3, HIGH);
    delay(100);
    digitalWrite(3, LOW);
    delay(100);
  }
  
}

// =================================================== //
// ====================== LOOP ======================= //
// =================================================== // 
void loop() {
  network.update();
  
  //============ Receiving ============//
  while ( network.available() ) {  // Is there any incoming data?
    RF24NetworkHeader header;
    char incomingData[32]= "";
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data

    Serial.println("Received: ");
    Serial.println(incomingData);
    digitalWrite(3,HIGH);

  
  //============= Sending =============//
    if (header.from_node == node11) {    // If data comes from Node 011, if-statement can be duplicated for multiple nodes (021, 031 etc)
        RF24NetworkHeader header(master00); // (Address where the data is going)
        network.write(header, &incomingData, sizeof(incomingData)); //sending received data to master node 


        Serial.println("Forwarding data from node11: ");
        Serial.println(incomingData);

        digitalWrite(3,LOW); //LED will blink if data is received & transmitted 
        delay(1000);
    }

  }
  
  }