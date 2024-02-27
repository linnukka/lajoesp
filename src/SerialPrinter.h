#ifndef SERIALPRINTER_H
#define SERIALPRINTER_H

#pragma once
#include <Arduino.h>

class SerialPrinter
{
public:
    SerialPrinter();
    ~SerialPrinter();
    void init(unsigned long usbBaudRate, unsigned long hwBaudRate);
    void sendDebug(String message);

private:
    int _usbBauds;
    int _hwBauds;

    // Serial _serial2;
    // Serial _serial3;
};

#endif