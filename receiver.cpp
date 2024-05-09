// =========================================================== //
// ======================= Receiver Code ===================== //
// =========================================================== //

// This code is designed for the master node, i.e the node meant to transfer the data to the database, receiving all data.
// Similarly to the transceiver code, for each node connected to the master node an adress & if-statements is needed.
// If these are not added it will be more difficult to differentiate where the data was coming from.

//============ Libraries ============// 
#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24Network.h>

//===================================// 

//==== Radio Network Definitions ====// 
RF24 radio(7, 8);                // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 00;   // Address of this node in Octal format ( 04,031, etc)
const uint16_t node01 = 01;      // Adress to next node, more can be added in Octal format (02, 03, etc) 

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
  while ( network.available() ) { // Is there any incoming data?
    RF24NetworkHeader header;
    char incomingData[32] = "";
    network.read(header, &incomingData, sizeof(incomingData)); // Read the incoming data   
    digitalWrite(3,HIGH);

    if(header.from_node=node01){ //If data comes from Node 01, if-statement can be duplicated for multiple nodes (02, 03 etc)
      Serial.println("Received: ");
      Serial.println(incomingData); 
    
      digitalWrite(3,LOW); //LED will blink if data is received from node 01
    }
  }
}