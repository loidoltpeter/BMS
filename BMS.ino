// Arduino powered BMS
// simulates BYD HV battery for Sungrow inverter
// kind of working with SH10RT but not safe! (no single cell monitoring yet)!
// for test purposes only!

#include <SPI.h>
#include <mcp2515.h>
#include "lcd.h"

//pin definition
#define powerSwitch 2
#define voltagePin  A2
#define currentPin  A0

//battery data
#define maxCurrent  20
#define minVoltage  290
#define maxVoltage  380

//tolerance before battery bms turns of the battery
#define cTol  5
#define vTol  5

//variables for calculation and display
float voltage;
float current;
float power;
float totalEnergy = 100.0;
float energy = 50.0;
int   soc;
int handshakeNr = 0;

//variables for CAN communication
int bmsVoltage;
int bmsCurrent;
int bmsEnergy;
int bmsTotalEnergy;
int bmsSoc;

const int bmsMaxVoltage = maxVoltage * 10;
const int bmsMinVoltage = minVoltage * 10;
const int bmsMaxCurrent = maxCurrent * 10;

//software interrupt variables
unsigned long currentMillis;
unsigned long previousMillis = 0;
const long interval = 1000;
unsigned long previousMillis1 = 0;
const long interval1 = 2000;
unsigned long previousMillis2 = 0;
const long interval2 = 10000;
unsigned long previousMillis3 = 0;
const long interval3 = 60000;

struct can_frame canMsg;
struct can_frame canMsg1;
struct can_frame canMsg2;
struct can_frame canMsg3;
struct can_frame canMsg4;
struct can_frame canMsg5;
struct can_frame canMsg6;
struct can_frame canMsg7;
struct can_frame canMsg8;
struct can_frame canMsg9;
struct can_frame canMsg10;
struct can_frame canMsg11;
struct can_frame canMsg12;

MCP2515 mcp2515(10);

void setup() { 
  pinMode(powerSwitch, OUTPUT);
  pinMode(voltagePin, INPUT);
  pinMode(currentPin, INPUT);
  
  digitalWrite(powerSwitch, HIGH);
  delay(100);

  Serial.begin(9600);

  Serial.println("------- TESLA BMS ----------");
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS);
  mcp2515.setNormalMode();

  lcd_init(LCD_DISP_ON);
  lcd_clrscr();
  lcd_set_contrast(0xFF);

  measure();
}

void loop() {

  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print("  "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print("  ");
    
    for (int i = 0; i<canMsg.can_dlc; i++)  {  // print the data
      Serial.print(canMsg.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
         
    if(canMsg.can_id == 0x105) {
      if(canMsg.data[0] == 1 && handshakeNr < 1) {
        canHandshake();
        handshakeNr ++;
      }
    }    
  }
  
  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    measure();
    checkBatt();
    updateCanMsg();
    show();
  }
  //send CAN
  if (currentMillis - previousMillis1 >= interval1) {
    previousMillis1 = currentMillis;
    mcp2515.sendMessage(&canMsg8);
  }
  if (currentMillis - previousMillis2 >= interval2) {
    previousMillis2 = currentMillis;
    mcp2515.sendMessage(&canMsg9);
    mcp2515.sendMessage(&canMsg10);
    mcp2515.sendMessage(&canMsg11);
  }
  if (currentMillis - previousMillis3 >= interval3) {
    previousMillis3 = currentMillis;
    mcp2515.sendMessage(&canMsg12);
  }
}

void measure() {
  long v = 0;
  long c = 0;
  
  for (int i = 0; i < 10; i++) {
    v += analogRead(voltagePin);
    c += analogRead(currentPin);
  }
  voltage = (v/10) * 0.483;
  current = ((c/10) - 782) * 0.193;
  
  v = (long) (voltage * 10L);
  voltage = (float) v / 10.0;
  c = (long) (current * 10L);
  current = (float) c / 10.0;
  
  power = (voltage * current) / 1000;
  energy += power / 3600;
  soc = (energy * 100) / totalEnergy;

  if(soc > 100) {
    soc = 100;
    handshakeNr = 0;
  }
  if(soc < 0) {
    soc = 0;
    handshakeNr = 0;
  }

  bmsVoltage = voltage * 10;
  bmsCurrent = current * 10;
  bmsEnergy = energy * 10;
  bmsTotalEnergy = totalEnergy * 10;
  bmsSoc = soc * 100;

  Serial.print(String(voltage) +"V  ");
  Serial.print(String(current) +"A  ");
  Serial.print(String(soc) +"%  ");
  Serial.print(String(power) +"kW  ");
  Serial.println(String(energy) +"kWh");
}

void checkBatt() {
  if (voltage <= minVoltage) {
    energy = 0;
  }
  if (voltage >= maxVoltage) {
    totalEnergy = energy;
  }
  if (voltage < minVoltage - vTol) {
    digitalWrite(powerSwitch, LOW);
    Serial.println("UNDERVOLTAGE");
    lcd_clrscr();
    lcd_charMode(DOUBLESIZE);
    lcd_gotoxy(0,0);
    lcd_puts("  UNDER \r\n VOLTAGE");
    lcd_display();
    while(1) {
    }
  }
  if (voltage > maxVoltage + vTol) {
    digitalWrite(powerSwitch, LOW);
    Serial.println("OVERVOLTAGE");
    lcd_clrscr();
    lcd_charMode(DOUBLESIZE);
    lcd_gotoxy(0,0);
    lcd_puts("  OVER \r\n VOLTAGE");
    lcd_display();
    while(1) {  
    }
  }
  if (current > maxCurrent + cTol || current < -maxCurrent - cTol) {
    digitalWrite(powerSwitch, LOW);
    Serial.println("OVERCURRENT");
    lcd_clrscr();
    lcd_charMode(DOUBLESIZE);
    lcd_gotoxy(0,0);
    lcd_puts("  OVER \r\n CURRENT");
    lcd_display();
    while(1) {  
    }
  }
}

void show() {
   
    char voltageArray[String(voltage).length()];
    String(voltage).toCharArray(voltageArray,String(voltage).length());

    char currentArray[String(current).length()];
    String(current).toCharArray(currentArray,String(current).length());

    char powerArray[String(power).length()];
    String(power).toCharArray(powerArray,String(power).length());

    char energyArray[String(energy).length()];
    String(energy).toCharArray(energyArray,String(energy).length());
    
    lcd_charMode(NORMALSIZE);
    lcd_gotoxy(0,0);
    lcd_puts(voltageArray);
    lcd_puts("V  ");
    lcd_gotoxy(0,1);
    lcd_puts(currentArray);
    lcd_puts("A  ");
    lcd_gotoxy(0,3);
    lcd_puts(powerArray);
    lcd_puts("kW  ");
    lcd_charMode(DOUBLESIZE);
    lcd_gotoxy(9,0);
    lcd_puts(energyArray);
    lcd_gotoxy(10,2);
    lcd_puts("kWh");

    //battery icon
    int fill = map(soc,0,100,29,4);    
    lcd_drawLine(44,0,44,31,WHITE);
    lcd_drawRect(115,2,127,31,WHITE);
    lcd_drawRect(118,0,124,2,WHITE);
    lcd_fillRect(117,fill,125,29,WHITE);
    lcd_display();
}

void canHandshake() {
  //Handshake messages
  canMsg1.can_id  = 0x250;
  canMsg1.can_dlc = 8;
  canMsg1.data[7] = 0x03;
  canMsg1.data[6] = 0x16;
  canMsg1.data[5] = 0x00;
  canMsg1.data[4] = 0x66;
  canMsg1.data[3] = 0x00;
  canMsg1.data[2] = 0x33;
  canMsg1.data[1] = 0x02;
  canMsg1.data[0] = 0x09;

  canMsg2.can_id  = 0x290;
  canMsg2.can_dlc = 8;
  canMsg2.data[7] = 0x06;
  canMsg2.data[6] = 0x37;
  canMsg2.data[5] = 0x10;
  canMsg2.data[4] = 0xD9;
  canMsg2.data[3] = 0x00;
  canMsg2.data[2] = 0x00;
  canMsg2.data[1] = 0x00;
  canMsg2.data[0] = 0x00;

  canMsg3.can_id  = 0x2D0;
  canMsg3.can_dlc = 8;
  canMsg3.data[7] = 0x00;
  canMsg3.data[6] = 0x42;
  canMsg3.data[5] = 0x59;
  canMsg3.data[4] = 0x44;
  canMsg3.data[3] = 0x00;
  canMsg3.data[2] = 0x00;
  canMsg3.data[1] = 0x00;
  canMsg3.data[0] = 0x00;

  canMsg4.can_id  = 0x3D0;
  canMsg4.can_dlc = 8;
  canMsg4.data[7] = 0x00;
  canMsg4.data[6] = 0x42;
  canMsg4.data[5] = 0x61;
  canMsg4.data[4] = 0x74;
  canMsg4.data[3] = 0x74;
  canMsg4.data[2] = 0x65;
  canMsg4.data[1] = 0x72;
  canMsg4.data[0] = 0x79;

  canMsg5.can_id  = 0x3D0;
  canMsg5.can_dlc = 8;
  canMsg5.data[7] = 0x01;
  canMsg5.data[6] = 0x2D;
  canMsg5.data[5] = 0x42;
  canMsg5.data[4] = 0x6F;
  canMsg5.data[3] = 0x78;
  canMsg5.data[2] = 0x20;
  canMsg5.data[1] = 0x50;
  canMsg5.data[0] = 0x72;

  canMsg6.can_id  = 0x3D0;
  canMsg6.can_dlc = 8;
  canMsg6.data[7] = 0x02;
  canMsg6.data[6] = 0x65;
  canMsg6.data[5] = 0x6D;
  canMsg6.data[4] = 0x69;
  canMsg6.data[3] = 0x75;
  canMsg6.data[2] = 0x6D;
  canMsg6.data[1] = 0x20;
  canMsg6.data[0] = 0x48;

  canMsg7.can_id  = 0x3D0;
  canMsg7.can_dlc = 8;
  canMsg7.data[7] = 0x03;
  canMsg7.data[6] = 0x56;
  canMsg7.data[5] = 0x4D;
  canMsg7.data[4] = 0x00;
  canMsg7.data[3] = 0x00;
  canMsg7.data[2] = 0x00;
  canMsg7.data[1] = 0x00;
  canMsg7.data[0] = 0x00;

  mcp2515.sendMessage(&canMsg1);
  mcp2515.sendMessage(&canMsg2);
  mcp2515.sendMessage(&canMsg3);
  mcp2515.sendMessage(&canMsg4);
  mcp2515.sendMessage(&canMsg5);
  mcp2515.sendMessage(&canMsg6);
  mcp2515.sendMessage(&canMsg7);
}

void updateCanMsg() {
  //2sec. Interval
  canMsg8.can_id  = 0x110; 
  canMsg8.can_dlc = 8;
  canMsg8.data[0] = highByte(bmsMaxVoltage); //max. Voltage *10
  canMsg8.data[1] = lowByte(bmsMaxVoltage);
  canMsg8.data[2] = highByte(bmsMinVoltage); //min. Voltage *10
  canMsg8.data[3] = lowByte(bmsMinVoltage);
  canMsg8.data[4] = highByte(bmsMaxCurrent); //max. discharge Current *10
  canMsg8.data[5] = lowByte(bmsMaxCurrent);
  canMsg8.data[6] = highByte(bmsMaxCurrent); //max. charge Current *10
  canMsg8.data[7] = lowByte(bmsMaxCurrent);;

  //10sec. Interval
  canMsg9.can_id  = 0x1D0;
  canMsg9.can_dlc = 8;
  canMsg9.data[0] = highByte(bmsVoltage); //Voltage *10
  canMsg9.data[1] = lowByte(bmsVoltage);
  canMsg9.data[2] = highByte(bmsCurrent); //Current *10
  canMsg9.data[3] = lowByte(bmsCurrent);
  canMsg9.data[4] = 0x00; //Temperature
  canMsg9.data[5] = 0xB4;
  canMsg9.data[6] = 0x03; //??
  canMsg9.data[7] = 0x08;

  canMsg10.can_id  = 0x210;
  canMsg10.can_dlc = 8;
  canMsg10.data[0] = 0x00; //max. Cell temp. *10
  canMsg10.data[1] = 0xBE;
  canMsg10.data[2] = 0x00; //min. Cell temp. *10
  canMsg10.data[3] = 0xB4;
  canMsg10.data[4] = 0x00;
  canMsg10.data[5] = 0x00;
  canMsg10.data[6] = 0x00;
  canMsg10.data[7] = 0x00;

  canMsg11.can_id  = 0x150;
  canMsg11.can_dlc = 8;
  canMsg11.data[0] = highByte(bmsSoc); //SOC % *100
  canMsg11.data[1] = lowByte(bmsSoc);
  canMsg11.data[2] = 0x27; //SOH % *100
  canMsg11.data[3] = 0x10;
  canMsg11.data[4] = highByte(bmsEnergy); //actual Ah?
  canMsg11.data[5] = lowByte(bmsEnergy);
  canMsg11.data[6] = highByte(bmsTotalEnergy); //Ah?
  canMsg11.data[7] = lowByte(bmsTotalEnergy);

  //60sec. Interval
  canMsg12.can_id  = 0x190;
  canMsg12.can_dlc = 8;
  canMsg12.data[0] = 0x00;
  canMsg12.data[1] = 0x00;
  canMsg12.data[2] = 0x00;
  canMsg12.data[3] = 0x04; //?
  canMsg12.data[4] = 0x00;
  canMsg12.data[5] = 0x00;
  canMsg12.data[6] = 0x00;
  canMsg12.data[7] = 0x00;
}
