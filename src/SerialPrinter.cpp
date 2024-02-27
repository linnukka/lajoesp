#include "SerialPrinter.h"

SerialPrinter::SerialPrinter()
{

}

SerialPrinter::~SerialPrinter()
{

}

void SerialPrinter::init(unsigned long usbBaudRate, unsigned long hwBaudRate){
    Serial.begin(usbBaudRate);
    Serial2.begin(hwBaudRate);
}

void SerialPrinter::sendDebug(String message){
    Serial.println("DEBUG: " + message);
}