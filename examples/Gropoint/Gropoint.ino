#include "Gropoint.h"
#include "EnableInterrupt.h"

#define ENABLE_GROPOINT_DEBUG 1
#define debugSerial Serial

#define PIN_SDI12 5

SDI12 sdi(PIN_SDI12);
Gropoint gropoint(sdi);

void setup()
{
  // put your setup code here, to run once:
  debugSerial.begin(19200);

  // If SDI pin hasn't dedicated interrupt as D2 on Arduino UNO, we should use any other digital pin.
  // Then, we need to set an event on PIN_CHANGE_INTERRUPT on this pin.
  // If SDI pin has own interrupt, we must use it.
  enableInterrupt(PIN_SDI12, SDI12::handleInterrupt, CHANGE);

#if ENABLE_GROPOINT_DEBUG == 1
  debugSerial.println("Debug active");
  gropoint.setDebugSerial(debugSerial);
#endif

  debugSerial.println("START");

  char address;

  do
  {
    address = gropoint.findAddress();

    debugSerial.print(F("Address: "));

    if (address == '?')
    {
      debugSerial.println(F("Unable to get"));
      delay(10000);
    }
    else
    {
      debugSerial.println(address);
    }
  } while (address == '?');

#if ENABLE_GROPOINT_DEBUG == 1
  char infoBuffer[50] = "";
  gropoint.getInfo(infoBuffer, sizeof(infoBuffer));
  debugSerial.print("Info: ");
  debugSerial.println(infoBuffer);
#endif
  delay(1000);
}

void loop()
{
  // put your main code here, to run repeatedly:
  float moisture[6]= {0,0,0,0,0,0};
  float temperature[7];

  debugSerial.println(F("Moisture: "));
  gropoint.readMoisture(moisture, sizeof(moisture)/sizeof(moisture[0]));
  printValues(moisture, sizeof(moisture)/sizeof(moisture[0]));

  debugSerial.println(F("Temperature: "));
  gropoint.readTemperature(temperature, sizeof(temperature)/sizeof(temperature[0]));
  printValues(temperature, sizeof(temperature)/sizeof(temperature[0]));

  delay(10000);
}

void printValues(float *values, size_t values_length)
{
  debugSerial.println(F("Values: "));

  for (size_t i = 0; i < values_length; ++i)
  {
    debugSerial.println(values[i]);
  }
}
