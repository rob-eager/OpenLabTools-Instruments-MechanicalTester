/* 
 One Wire sensor scanner
 Written by: Eng. SANGA, Valerian Linus
             sangavalerian@live.com

 Based on example at: 
 http://www.hacktronics.com/Tutorials/arduino-1-wire-address-finder.html
 */

#include <OneWire.h>

#define SENSOR_PIN 2  // Any pin 2 to 12 (not 13) and A0 to A5

OneWire  ourBus(SENSOR_PIN);  // Create a 1-wire object

void setup(){
  Serial.begin(115200);
  delay(1000);
  Serial.println("Sensor scanner initiated successfully.");
  delay(200);
  discoverOneWireDevices();  // Calling the scanning function
}

void loop() {
  // Nothing happening here
}

//The function for scanning sensor addresses
void discoverOneWireDevices(void) {
  byte v;
  //byte present = 0;
  int myCount=0;
  byte data[12];
  byte addr[8];

  Serial.println("Scanning for 1-Wire devices...\n\r");
  while(ourBus.search(addr)) {
    Serial.print("\n\r\n\rFound \'1-Wire\' device with address:\n\r");
    for( v = 0; v < 8; v++) {
      Serial.print("0x");
      if (addr[v] < 16) {
        Serial.print('0');
      }
      Serial.print(addr[v], HEX);
      if (v < 7) {
        Serial.print(", ");
      }
    }
    myCount++;  //Counts the number of devises
    if ( OneWire::crc8( addr, 7) != addr[7]) {
      Serial.print("CRC is not valid!\n\r");
      return;
    }
    
  }
  Serial.println();
  Serial.println();
  Serial.print("Devices found: "); //Tells the number of devices found
  Serial.println(myCount);
  Serial.println("Scanning Compete!");
  Serial.println();
  ourBus.reset_search();
  return;
}
//The End of scanning function
