/*
 * STEPPER MOTOR DRIVER FOR THE MECHANICAL TESTER
   This firmware is writen based on the ARDUINO CNC Shield V3, where as the X and Y 
   Stepper motor connections have been used to drive a pair of Stepper Motors for the Z
   motion of the Mechanical Tester Unit.

NOTE:
   We need 'SoftwareSerial.h' header even if we're not using a SoftwareSerial objects         
   BUT incase you are using the Arduino DUE you will have to comment this header and then follow further instruction in SerialCommand.h for usage of HardwareSerial Only. 
 
   Codes writen by: Eng. SANGA, Valerian Linus
                    Based on the original Codes written by Robb
   For: Mechanica Tester
   Date: Sept 2017, UK & TZ
   Licence: OPEN GL V3
     
 */
 
//#include <SoftwareSerial.h>
#include <Arduino.h>
#include <SerialCommand.h> 
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Servo.h>
#include <DHT.h>
#include "A4988.h"


// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define Stepper1_MOTOR_STEPS 200
#define Stepper2_MOTOR_STEPS 200

//STEPPER MOTORS DEFINING
//** Stepper1
#define Stepper1_DIR  5
#define Stepper1_STEP 2

//** Stepper2
#define Stepper2_DIR  6
#define Stepper2_STEP 3

//** Stepper Enable pin
#define Stepper_ENBL  8

//** Variable that keeps the motor stepps
char *motorSteps;

SerialCommand myCMD; 

unsigned long Enormous = 0xFFFFFFFF;
unsigned long Current_Time;                                         // Overrun at 25 days

#define Heater_Pin 10                                               // Relays active low
unsigned long Heat_Cycle_Interval = 1000;                           // Cycle time (seconds)
unsigned long Percent_Heat = 0;                                     // Percentage duty cycle
unsigned long Heat_On_Interval, Heat_Off_Interval, Heat_Next_Time;

#define Mist_Pin 11                                                 // Relays active low
unsigned long Mist_On_Interval = 0;
unsigned long Mist_Next_Time = Enormous;

Servo flap;
const int flap_open   = 150;   // Value to open flap
const int flap_closed = 55;    // Value to close flap

#define Top_Fan_Pin    22
#define Bottom_Fan_Pin 24
#define LED_Pin        26

int LED_Target_Val = 0;
int LED_Current_Val = 0;
int LED_Delta = 1;
int LED_Div = 10;

#define DHT_Pin 9

DHT dht(DHT_Pin, DHT22);

#define Temp_Pin 32                      // DS1820B Pin
OneWire Temp_Wire(Temp_Pin);
DallasTemperature sensors(&Temp_Wire);

A4988 Stepper1(Stepper1_MOTOR_STEPS, Stepper1_DIR, Stepper1_STEP, Stepper_ENBL);
A4988 Stepper2(Stepper2_MOTOR_STEPS, Stepper2_DIR, Stepper2_STEP, Stepper_ENBL);

//DS temperature sensor address
/*
 * NOTE:
 * The DS18B20 temperature sensors have different addresses, thus before using the codes for your design
   make sure you run the DS_Scanner codes/sketch so as to identify the respective addresses for your sensors.
   Paste the addresses as obtained from the results.
 */
DeviceAddress Ext_Temp_Addr             = { 0x28, 0xD0, 0x38, 0x23, 0x09, 0x00, 0x00, 0x16 };
DeviceAddress Bottom_Chamber_Temp_Addr  = { 0x28, 0xC8, 0xBC, 0x22, 0x09, 0x00, 0x00, 0xB7 };
DeviceAddress Top_Chamber_Temp_Addr     = { 0x28, 0xA3, 0xD4, 0x22, 0x09, 0x00, 0x00, 0xA3 };
DeviceAddress Heater_Temp_Addr          = { 0x28, 0x5F, 0x6D, 0x23, 0x09, 0x00, 0x00, 0x9F };
unsigned long Temp_Cycle_Interval       = Enormous;                 // temperature cycle in seconds
unsigned long Temp_Interval, Temp_Next_Time;

void setup() {
    Serial.begin(9600);
    Serial.println("System Booting...");
    delay(2000);

    //initiate dht sensor
    dht.begin();

    //Setup Stepper motors
    Stepper1.disable();
    Stepper2.disable();
    
    Stepper1.setRPM(100);
    Stepper2.setRPM(100);

    // Setup callbacks for SerialCommand commands
        // Motion controller
    myCMD.addCommand("UP", moving_up);             // Moving Z beam UP
    myCMD.addCommand("DOWN", moving_down);         // Moving Z beam UP
        // Functions controllers
    myCMD.addCommand("FANS", fan_update);          // Sets each fan on or off
    myCMD.addCommand("FLAP", flap_update);         // Flap position
    myCMD.addCommand("OPEN", close_flap);          // Flap position
    myCMD.addCommand("CLOSE",open_flap);           // Flap position
    myCMD.addCommand("LED",  LED_update);          // Flap position
    myCMD.addCommand("HEAT", heat_update_params);  // Heater, Cycle_Period, %On
    myCMD.addCommand("MIST", mister_on);           // Mister, Cycle_Period, %On
    myCMD.addCommand("DATA", data_output);
     
    myCMD.addDefaultHandler(unrecognized_cmd);     // For unrecognized commands
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

    // Flap servo init
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

    //Prints the comand lists over Serial terminal
    Serial.println();
    delay(500);
    Serial.println("Commands Lists:");
    Serial.println("UP+Amount of Steps  : For moving Z axis up.");
    Serial.println("DOWN+Amount of Steps: For moving Z axis down.");
    Serial.println("FANS                : For seting each fan on or off."); 
    Serial.println("FLAP                : For changing flap postion."); 
    Serial.println("OPEN                : For closing flap"); 
    Serial.println("CLOSE               : For opening flap"); 
    Serial.println("LED                 : For LED conrolling");
    Serial.println("HEAT                : For heaters operation"); 
    Serial.println("MIST                : For mister operations"); 
    Serial.println("DATA                : For sending sensor readouts over serial");
    Serial.println("Note:");
    Serial.println("1. Carriage return should be enabled in your terminal.");
    Serial.println("2. The plus sign (+) means SPACE, thus Don't put a plus (+).");
    Serial.println("3. It is case sensitive.");
    
}

void loop() {
   Current_Time = millis();
   
   myCMD.readSerial();                           // See if there are any incoming commands over serial

   heat_subroutine();
   LED_subroutine();
   mist_subroutine();
   
}


// SUBROUTINES
//fan subroutine
void fan_update() {
  char *arg;

  Serial.println("Adjusting Fans");
  arg = myCMD.next();
  if (arg != NULL) {
    digitalWrite(Top_Fan_Pin, atoi(arg));
  }
  arg = myCMD.next();
  if (arg != NULL) {
    digitalWrite(Bottom_Fan_Pin, atoi(arg));
  }
}

//flap update subroutine
void flap_update() {
  char *arg;

  Serial.println("Adjusting Flap");
  arg = myCMD.next();
  if (arg != NULL) {
    flap.write(atoi(arg));
  }
}

//flap open subroutine
void open_flap() {
  Serial.println("Opening Flap");
  flap.write(flap_open);
}

//flap close subroutine
void close_flap() {
  Serial.println("Closing Flap");
  flap.write(flap_closed);
}

//led subroutine
void LED_update() {
  Serial.println("LED Updating");
  char *arg;

  arg = myCMD.next();
  if (arg != NULL) {
    LED_Target_Val = atoi(arg);
  }
  arg = myCMD.next();
  if (arg != NULL) {
    LED_Div = atoi(arg);
  }
}

//heating updator params subroutine
void heat_update_params() {
  char *arg;

  Serial.println("Adjusting Heat Params");
  arg = myCMD.next();
  if (arg != NULL) {
    Heat_Cycle_Interval = atoi(arg);                // Converts a char string to an integer
  }
  else {
    Serial.println("  No arguments");
  }

  arg = myCMD.next();
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

//void mister subroutine
void mister_on() {
  Serial.println("Mister function");
  char *seconds;
  seconds = myCMD.next();
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

//hear subroutine
void heat_subroutine() {
  
  Serial.println("Heater Functions");
  if (Current_Time > Heat_Next_Time) {
    if (!digitalRead(Heater_Pin)) {                      // Heater is on, turn off
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

//mist subroutine
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

//led subroutine
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

//data output subroutine
void data_output() {
  sensors.requestTemperatures();
  
  Serial.print("Sensor Readings: ");
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

//Up motion
void moving_up(){
  
  Stepper1.enable();
  Stepper2.enable();
  motorSteps = myCMD.next();  // Captures the amount of steps

  if (motorSteps!=NULL){
    Serial.print("Moving Up: ");
    Serial.print(motorSteps );
    Serial.println(" Steps");
    for(int s=0; s<atoi(motorSteps); s++){
      Stepper1.move(1);
      Stepper2.move(1);
      }
    }else{
      Serial.println("ERROR! The Motor Stepps are not defined!");
      } 
}

//Down motion
void moving_down(){
  
  Stepper1.enable();
  Stepper2.enable();
  motorSteps = myCMD.next();

  if (motorSteps!=NULL){
    Serial.print("Moving Down: ");
    Serial.print(motorSteps );
    Serial.println(" Steps");
    //Sync the two steppers
   for(int s=0; s<atoi(motorSteps); s++){
      Stepper1.move(-1);    
      Stepper2.move(-1);
      }
     }else{
      Serial.println("ERROR! The Motor Stepps are not defined!");
      } 
}

//Unrecognized commands
void unrecognized_cmd(){
  Serial.println("SORRY!! Wrong command!!"); 
}




