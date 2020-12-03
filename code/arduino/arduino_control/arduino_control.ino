/*
 * We need 'SoftwareSerial.h' header even if we're not using a SoftwareSerial objects         
 * BUT incase you are using the Arduino DUE you will have to comment this header and then follow further instruction in SerialCommand.h for usage of HardwareSerial Only. 
 */
//#include <SoftwareSerial.h>  

#include <SerialCommand.h> 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>
#include <DHT.h>

SerialCommand Olab_Cmd;                                            // The Olab SerialCommand object

unsigned long Enormous = 0xFFFFFFFF;
unsigned long Current_Time;                                        // Overrun at 25 days

#define Heater_Pin 4                                               // Relays active low
unsigned long Heat_Cycle_Interval = 1000;                          // Cycle time (seconds)
unsigned long Percent_Heat = 0;                                    // Percentage duty cycle
unsigned long Heat_On_Interval, Heat_Off_Interval, Heat_Next_Time;

#define Mist_Pin 3                                                 // Relays active low
unsigned long Mist_On_Interval = 0;
unsigned long Mist_Next_Time = Enormous;

Servo flap;
const int flap_open   = 150;
const int flap_closed = 55;

#define Top_Fan_Pin    6
#define Bottom_Fan_Pin 5
#define LED_Pin        11

int LED_Target_Val = 0;
int LED_Current_Val = 0;
int LED_Delta = 1;
int LED_Div = 10;

#define DHT_Pin        9

DHT dht(DHT_Pin, DHT22);

#define Temp_Pin 2                                                 // DS1820B Pin
OneWire Temp_Wire(Temp_Pin);
DallasTemperature sensors(&Temp_Wire);

//DeviceAddress Ext_Temp_Addr       = { 0x28, 0xA6, 0xE4, 0xEE, 0x04, 0x00, 0x00, 0xF7 };
//DeviceAddress Bottom_Chamber_Temp_Addr       = { 0x28, 0x25, 0xE3, 0xEE, 0x04, 0x00, 0x00, 0x15 };
//DeviceAddress Top_Chamber_Temp_Addr = { 0x28, 0xCA, 0x71, 0xEE, 0x04, 0x00, 0x00, 0x2C };
//DeviceAddress Heater_Temp_Addr  = { 0x28, 0x35, 0x99, 0xEF, 0x04, 0x00, 0x00, 0x11 };
//unsigned long Temp_Cycle_Interval = Enormous;                      // temperature cycle in seconds
//unsigned long Temp_Interval, Temp_Next_Time;

DeviceAddress Ext_Temp_Addr       = { 0x28, 0xD0, 0x38, 0x23, 0x09, 0x00, 0x00, 0x16 };
DeviceAddress Bottom_Chamber_Temp_Addr       = { 0x28, 0xC8, 0xBC, 0x22, 0x09, 0x00, 0x00, 0xB7 };
DeviceAddress Top_Chamber_Temp_Addr = { 0x28, 0xA3, 0xD4, 0x22, 0x09, 0x00, 0x00, 0xA3 };
DeviceAddress Heater_Temp_Addr  = { 0x28, 0x5F, 0x6D, 0x23, 0x09, 0x00, 0x00, 0x9F };
unsigned long Temp_Cycle_Interval = Enormous;                      // temperature cycle in seconds
unsigned long Temp_Interval, Temp_Next_Time;


void setup() {
  Serial.begin(115200);
  dht.begin();

  // Setup callbacks for SerialCommand commands
  Olab_Cmd.addCommand("FANS", fan_update);          // Sets each fan on or off
  Olab_Cmd.addCommand("FLAP", flap_update);         // Flap position
  Olab_Cmd.addCommand("OPEN", close_flap);          // Flap position
  Olab_Cmd.addCommand("CLOSE",open_flap);           // Flap position
  Olab_Cmd.addCommand("LED",  LED_update);          // Flap position
  Olab_Cmd.addCommand("HEAT", heat_update_params);  // Heater, Cycle_Period, %On
  Olab_Cmd.addCommand("MIST", mister_on);           // Mister, Cycle_Period, %On
  Olab_Cmd.addCommand("DATA", data_output);         // Data
  Olab_Cmd.addDefaultHandler(unrecognized_params);  // Handler for command that isn't matched  (says "What?")
  Serial.println("Mechanical Tester: System Ready!");

  // initialize pins:
  digitalWrite(Heater_Pin, HIGH);
  pinMode(Heater_Pin, OUTPUT);
  
  digitalWrite(Mist_Pin, HIGH);
  pinMode(Mist_Pin, OUTPUT);
    
  digitalWrite(Top_Fan_Pin, LOW);
  digitalWrite(Bottom_Fan_Pin, LOW);
  pinMode(Top_Fan_Pin, OUTPUT);
  pinMode(Bottom_Fan_Pin, OUTPUT);
  
  analogWrite(LED_Pin, 0);
  pinMode(LED_Pin, OUTPUT);
  
  flap.attach(10);
  flap.write(150);

  // initialise temperature sensors
  sensors.begin();

  // set up variables
  Heat_On_Interval  = 1000 * Heat_Cycle_Interval * Percent_Heat/100;
  Heat_Off_Interval = 1000 * Heat_Cycle_Interval * (100-Percent_Heat)/100;

  Temp_Interval = 1000 * Temp_Cycle_Interval;
  sensors.setResolution(Ext_Temp_Addr, 10);
  sensors.setResolution(Bottom_Chamber_Temp_Addr, 10);
  sensors.setResolution(Top_Chamber_Temp_Addr, 10);
  sensors.setResolution(Heater_Temp_Addr,  10);
}

void loop() {
  Current_Time = millis();

  Olab_Cmd.readSerial();     // See if there are changes to timers

  heat_subroutine();
  LED_subroutine();
  mist_subroutine();

  // strain_subroutine();
}

// SUBROUTINES

  void fan_update() {
    char *arg;

    Serial.println("Adjusting Fans");
    arg = Olab_Cmd.next();
    if (arg != NULL) {
      digitalWrite(Top_Fan_Pin, atoi(arg));
    }
    arg = Olab_Cmd.next();
    if (arg != NULL) {
      digitalWrite(Bottom_Fan_Pin, atoi(arg));
    }
  }
  
  void flap_update() {
    char *arg;
 
    Serial.println("Adjusting Flap");
    arg = Olab_Cmd.next();
    if (arg != NULL) {
      flap.write(atoi(arg));
    }
  }
  
  void open_flap() {
    flap.write(flap_open);
  }
  
  void close_flap() {
    flap.write(flap_closed);
  }
  
  void LED_update() {
    char *arg;

    arg = Olab_Cmd.next();
    if (arg != NULL) {
      LED_Target_Val = atoi(arg);
    }
    arg = Olab_Cmd.next();
    if (arg != NULL) {
      LED_Div = atoi(arg);
    }
  }
  
  void heat_update_params() {
    char *arg;

    Serial.println("Adjusting Heat Params");
    arg = Olab_Cmd.next();
    if (arg != NULL) {
      Heat_Cycle_Interval = atoi(arg);    // Converts a char string to an integer
    }
    else {
      Serial.println("  No arguments");
    }

    arg = Olab_Cmd.next();
    if (arg != NULL)
    {
      Percent_Heat = atol(arg);
    }
    else {
      Serial.println("  No second argument");
    }
     Heat_On_Interval = 1000 * Heat_Cycle_Interval * Percent_Heat/100;
     Heat_Off_Interval = 1000 * Heat_Cycle_Interval * (100-Percent_Heat)/100;
     Serial.print("  Heat_On_Interval: ");
     Serial.println(Heat_On_Interval);
     Serial.print("  Heat_Off_Interval: ");
     Serial.println(Heat_Off_Interval);
  }

  void mister_on() {
    char *seconds;
    seconds = Olab_Cmd.next();
    if (seconds != NULL)
    {
      Mist_On_Interval = 1000 * atoi(seconds);
    }
    else {
      Serial.println("  No length given");
    }

     Serial.print("  Misting for: ");
     Serial.print(seconds);
     Serial.println(" seconds.");
     
     Mist_Next_Time = 0;
  }

  void unrecognized_params() {
    // This gets set as the default handler, and gets called when no other command matches.
    Serial.println("What?");
  }

void heat_subroutine() {
  if (Current_Time > Heat_Next_Time) {
    if (!digitalRead(Heater_Pin)) { // Heater is on, turn off
      digitalWrite(Heater_Pin, HIGH);
      Heat_Next_Time = Current_Time + Heat_Off_Interval;
    }
    else {
      if (Heat_On_Interval != 0) {
        digitalWrite(Heater_Pin, LOW);
        Heat_Next_Time = Current_Time + Heat_On_Interval;
      }
    }
  }
}

void mist_subroutine() {
  if (Current_Time > Mist_Next_Time) {
    if (!digitalRead(Mist_Pin)) {
        digitalWrite(Mist_Pin, HIGH); // Turn off mister
        Mist_Next_Time = Enormous;
    }
    else {
      if (Mist_On_Interval != 0) {
        digitalWrite(Mist_Pin, LOW);
        Mist_Next_Time = Current_Time + Mist_On_Interval;
      }
    }
  }
}

void LED_subroutine() {
  if ((Current_Time % LED_Div) == 0) {
    if (LED_Current_Val < LED_Target_Val) {
      if ((LED_Target_Val - LED_Current_Val) < LED_Delta) {
        analogWrite(LED_Pin, LED_Target_Val);
        LED_Current_Val = LED_Target_Val;
      }
      else {
        analogWrite(LED_Pin, LED_Current_Val + LED_Delta);
        LED_Current_Val = LED_Current_Val + LED_Delta;
      }
    }
    if (LED_Current_Val > LED_Target_Val) {
      if ((LED_Current_Val - LED_Target_Val) < LED_Delta) {
        analogWrite(LED_Pin, LED_Target_Val);
        LED_Current_Val = LED_Target_Val;
      }
      else {
        analogWrite(LED_Pin, LED_Current_Val - LED_Delta);
        LED_Current_Val = LED_Current_Val - LED_Delta;
      }
    }
  }
}

void data_output() {
  sensors.requestTemperatures();
  
  float h = dht.readHumidity();
  float t = dht.readTemperature();
    
  Serial.print(" ");
  Serial.print(sensors.getTempC(Ext_Temp_Addr));
  Serial.print(" ");
  Serial.print(sensors.getTempC(Bottom_Chamber_Temp_Addr));
  Serial.print(" ");
  Serial.print(sensors.getTempC(Top_Chamber_Temp_Addr));
  Serial.print(" ");
  Serial.print(sensors.getTempC(Heater_Temp_Addr));
  Serial.print(" ");
  Serial.print(h = dht.readTemperature());
  Serial.print(" ");
  Serial.println(h = dht.readHumidity());
}
