#include <Arduino.h>
#include <stdio.h>
#include <SoftwareSerial.h>
#include <InterpolationLib.h>
#include <SPI.h>          //Library for using SPI Communication 
#include <mcp2515.h>      //Library for using CAN Communication (https://github.com/autowp/arduino-mcp2515/)

struct can_frame canMsg;
 
MCP2515 mcp2515(10);

#define ACT_LED 4
#define STAT_LED 13

#define NTC_CH1 0

//Offsets defines for the analog channels for NTC.
//Real Arduino: 212.008281 
//China Arduino: 204.391217
#define NTC_CH1_OFFSET 212.008281

unsigned long CyclickTask_counter10ms = 0u;
const long Counter10msDelayTime = 10u;
unsigned int CounterActLed = 0u;
unsigned int CounterStatLed = 0u;
unsigned int CounterDebugInfo = 0u;
const int CounterActLedRef = 100u;
const int CounterStatLedRef = 50u;
const int CounterDebugInfoRef = 100u;
const int maxIterationTemp = 3;

//NTC Raw Values from the ADC
unsigned int NtcCh1AdcRaw = 0u;

unsigned int tempout_NTC_1 = 0u;
unsigned int calcAdcRaw_NTC_1 = 0u;
unsigned int calcTempFullAdcRaw_NTC_1 = 0u;



//Interpolation points:
  const unsigned char numValues = 13;
  double xValues_Voltage[13] = { 0.11644697, 0.221562458, 
                                 0.401090967, 0.685617124,
                                 1.097381647,1.62022035,
                                 2.204293965, 2.786757329,
                                 3.316969617, 3.74925015, 
                                 4.085968783, 4.336513443, 
                                 4.517528009 }; // Points from the NTC curve
                                 
  double yValues_Celsius[13] = {-40, -30,
                                -20, -10,
                                  0, 10,
                                 20, 30, 
                                 40, 50,
                                 60, 70, 
                                 80 };//List for temperature range  
                                 

void setup()
{
  //init functions
  while (!Serial);
  Serial.begin(9600);
  SPI.begin();               //Begins SPI communication
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ); //Sets CAN at speed 500KBPS and Clock 8MHz
  mcp2515.setNormalMode();
  pinMode(ACT_LED, OUTPUT);
  digitalWrite(ACT_LED, HIGH);
  //pinMode(STAT_LED, OUTPUT);
  //digitalWrite(STAT_LED, HIGH);

  //Print the debug menu interface welcome message
  Serial.print("\n -------------------------------------------");
  Serial.print("\n|       Welcome to the Free Masonry|");
  Serial.print("\n -------------------------------------------");
}

void loop()
{
  if(millis() - CyclickTask_counter10ms >= Counter10msDelayTime)
  {
    CyclickTask_counter10ms = millis();
    //ActLedCyclic();
    AdcReadCyclic();
    NtcDataCyclic();
    //DebugDataCyclic();
    SendCANData();
    // Put here your Cyclick task on Counter10msDelayTime
  }
}

void ActLedCyclic()
{
 if(CounterStatLed >= CounterStatLedRef)
  {
    CounterStatLed = 0u;
    digitalWrite(STAT_LED, !digitalRead(STAT_LED));
  }
  else
  {
    CounterStatLed++;
  }

   if(CounterActLed >= CounterActLedRef)
  {
    CounterActLed = 0u;
    digitalWrite(ACT_LED, !digitalRead(ACT_LED));
  }
  else
  {
    CounterActLed++;
  }
}

void AdcReadCyclic()
{
  calcTempFullAdcRaw_NTC_1 = 0u;
  for (int i = 0; i < maxIterationTemp; i++) // Filtering of ADC raw
  {
    NtcCh1AdcRaw = analogRead(NTC_CH1);
    NtcCh1AdcRaw = (NtcCh1AdcRaw/(NTC_CH1_OFFSET));
    calcTempFullAdcRaw_NTC_1 =+ NtcCh1AdcRaw;
  }

  calcAdcRaw_NTC_1 = calcTempFullAdcRaw_NTC_1 / maxIterationTemp;
}

void NtcDataCyclic()
{
   //Interpolation
   tempout_NTC_1 = Interpolation::Linear(xValues_Voltage, yValues_Celsius, numValues, calcAdcRaw_NTC_1, true);
}

void DebugDataCyclic()
{
//DebugDataCyclic

 if(CounterDebugInfo >= CounterDebugInfoRef)
  {
    CounterDebugInfo = 0u;
    Serial.print("\n -------------------------------------------");
    Serial.print("\n|       DEBUG INFO                         |");
    Serial.print("\n -------------------------------------------");
    Serial.print("\n BLUETOOTH RECEIVED INFO:");
   
  }
  else
  {
    CounterDebugInfo++;
  }
}

void SendCANData()
{
  canMsg.can_id  = 0x036;            //CAN id as 0x036
  canMsg.can_dlc = 8;                //CAN data length as 8
  canMsg.data[0] = calcAdcRaw_NTC_1; //Update temperature value in [0]
  canMsg.data[1] = 0x00;             //Rest all with 0
  canMsg.data[2] = 0x00;
  canMsg.data[3] = 0x00;
  canMsg.data[4] = 0x00;
  canMsg.data[5] = 0x00;
  canMsg.data[6] = 0x00;
  canMsg.data[7] = 0x00;
 
  mcp2515.sendMessage(&canMsg);     //Sends the CAN message
}
