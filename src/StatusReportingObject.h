#ifndef STATUSREPORTINGOBJECT_H
#define STATUSREPORTINGOBJECT_H

#pragma once

#include <Arduino.h>

class StatusReportingObject {
    public:
        virtual String getStatusString() = 0;
        virtual String getName() = 0;
        virtual void update() = 0;
    protected:
        unsigned long _lastUpdateTime = 0;
};

#endif