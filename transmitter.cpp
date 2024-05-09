// =========================================================== //
// ===================== Transmitter Code ==================== //
// =========================================================== //

// This code is for the last node in the network, which gathers data from different sensors.
// It is built to transmit data from 5 different sensors, temperature, wind (dir & speed), intensity, voltage & current.
// Currently assigned node position 011, this node will transmit code to node 01, which in turn sends to the master node 00.
// The sensors can be divided over multiple nodes, for example assigned position 011, 021, 031,... which all send data to node 01. 
// For each node added in the network, adresses for the nodes in the code will need to be slightly adjusted for each node.

//========= Choose Sensors ==========//
bool Text         = true;
bool Temperature  = true;
bool Anemometer   = true;
bool Pyranometer  = true;
bool Voltage      = true;
bool Current      = true;
//===================================//



//============ Libraries ============// 
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <DallasTemperature.h> // For DS18B20 temperature sensor

//===================================// 

//==== Radio Network Definitions ====// 
RF24 radio(7, 8);                // nRF24L01 (CE,CSN)
RF24Network network(radio);      // Include the radio in the network
const uint16_t this_node = 011;  // Address of this node in Octal format
const uint16_t node01 = 01;      // Address of the next node in Octal format

char datapack[32] = ""; // data to be sent, max payload of NRF24 is 32 bytes

//====== RDS18B20 Definitions =======// 
#define ONE_WIRE_BUS 4 // pin defintion temperature sensor
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);


//===== Anemometer Definitions ======//
#define waitWhileHigh  while  (PIND & 0b00000100)
#define waitWhileLow   while ((PIND & 0b00000100)==0)
volatile uint32_t buffer, telegram[2];
volatile uint8_t  bit, ready;
char strbuf[40];

ISR(TIMER1_CAPT_vect)
{
  if (!ready)
  {
    buffer >>= 1;
    if (PIND & 0b00000100)
    {
      buffer |= 0x80000000;
    }
    bit ++;
    if (bit==32)
    {
      bit   = 0;
      ready = 1;
    }
    _delay_us(200);
  }
}

//== Anemometer wait function ==//
int8_t waitWhileHighTO (void)
{
  uint16_t timeout=0;
  while ((PIND & 0b00000100) && (timeout<10000))
  {
    timeout++;
    _delay_us(1);
  }
  if (timeout<10000)
  {
    return 0;
  }
  else
  {
    return -1;
  }
}
//===================================//



//===== Other Pin Definitions =======//
const int pyra_pin = A6;

const int voltage_pin = A2;

const int current_pin = A4;
//===================================//


// =================================================== //
// ====================== SETUP ====================== //
// =================================================== // 
void setup() { // Initiating all libraries 
  
  SPI.begin();                  
  radio.begin();
  network.begin(90, this_node);   // (channel, node address)

  sensors.begin();                
  Serial.begin(9600);             // Needed to visualize in serial monitor

  radio.setPALevel(RF24_PA_MIN);  // Either PA_MIN or PA_MAX depending on range between nodes


  pinMode(3,OUTPUT); // Initializing LED 
  for (int i = 0; i < 7; i++)
  {
    digitalWrite(3, HIGH);
    delay(100);
    digitalWrite(3, LOW);
    delay(100);
  }
  
  //======= Anemometer setup ========//

  // run TIMER 1 on f_cpu / 8, CTC every 100us
  // mode 12, prescaler 8
  // fcpu = 16 MHz -> ICR1 = 1999
  
  ICR1 = 1999;
  TCCR1A = (0 << WGM11) | (0 << WGM10);
  TCCR1B = (1 << WGM13) | (1 << WGM12)
         | (0 << CS12)  | (1 << CS11) | (0 << CS10);
  ready = 1;
  TIMSK1  |= (1 << ICIE1);

  sei();
  DDRD  &= ~0b00000100;
  PORTD |=  0b00000100;   // set to 1
  _delay_ms(2500);        // wait 10 seconds

  Serial.println("Setup done");

  //=================================//

}

// =================================================== //
// ====================== LOOP ======================= //
// =================================================== // 
void loop() {
  network.update();
  strcpy(datapack, ""); //cleanse message

  //=========== Text msg ============//
  if(Text){
    const char text[] = "Aa";
    strcat(datapack, text);
  }

  //========== Temperature ==========//
  if(Temperature){
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
    char tempdata[20];
    sprintf(tempdata,"T%d.%02d,",int(temp),int(temp*100)%100);
    strcat(datapack, tempdata);
  }
  
  //========== Anemometer ===========//
  if(Anemometer){
    uint8_t i;
    uint16_t dt2, val;
    DDRD  &= ~0b00000100;   // float pin
    PORTD |=  0b00000100;   // set to 1
    _delay_ms(2000);
    PORTD &= ~0b00000100;   // set to 0
    DDRD  |=  0b00000100;
    _delay_ms(500);
    DDRD  &= ~0b00000100;   // float pin
    PORTD |=  0b00000100;   // set to 1
    waitWhileLow;           // wait to reach "1"
    for (i=0; i<2; i++)
    {
      ICR1   = 65535;       // max runtime on Timer 1
      if (waitWhileHighTO()==0)
      {
      waitWhileLow;         // wait for start bit, 2x "1"
        TCNT1 = 0;          // reset TIMER1
        waitWhileHigh;      // wait for start
        dt2 = TCNT1;        // length of 2 bits in timer ticks
        ICR1 = dt2/2;
        waitWhileLow;       // wait for telegram start
        TCNT1 = dt2/4;      // 1/2 bit head start
        bit  = 0;
        ready = 0;          // start sampling
        while (!ready);     // get telegram: 2x uint16_t
        telegram[i] = buffer;
      }
    }

      for (i=0; i<1; i++)   //There are two telegrams, second with instantaneous measurements, first with average values
    {
      val = (telegram[i] >> 2) & 0b1111;
      sprintf(strbuf, "d%02u,", val);
      strcat(datapack,strbuf);
      val = (telegram[i] >> 6) & 0b111111111; //number of 1's equals number of bits analyzed
      sprintf(strbuf, "s%02u,", val);
      strcat(datapack,strbuf);
    }
  }

  
  //========== Pyranometer ==========//
  if(Pyranometer){
    int pyra_val_int = analogRead(pyra_pin);
    float pyra_val = pyra_val_int * (5.0 / 1024.0);
    char pyra_data[20];
    sprintf(pyra_data,"P%d.%02d,",int(pyra_val),int(pyra_val*100)%100);
    strcat(datapack, pyra_data);
  }

  //============ Voltage ============//
  if(Voltage){
    int span_val_int = analogRead(voltage_pin);
    float span_val = span_val_int * (5.0 / 1024.0)*11; //Voltage has been divided using voltage divider 11 times
    char span_data[20];
    sprintf(span_data,"V%d.%02d,",int(span_val),int(span_val*100)%100);
    strcat(datapack, span_data);
  }

  //============ Current ============//
  if(Current){ //Wrong values, new constants needs to be introduced
    int strom_val_int = analogRead(current_pin);
    float strom_val = strom_val_int * (5.0 / 1024.0);
    char strom_data[20];
    sprintf(strom_data,"I%d.%02d,",int(strom_val),int(strom_val*100)%100);
    strcat(datapack, strom_data);
  }


  //============ Sending ============//
  RF24NetworkHeader header1(node01);     // (Address where the data is going)
  network.write(header1, &datapack, sizeof(datapack)); // Send the data

  Serial.println("Sending: ");
  Serial.println(datapack);

  digitalWrite(3,LOW);
  delay(1000);
}