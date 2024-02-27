#ifndef STATUSUPDATER_H
#define STATUSUPDATER_H

#pragma once

#include <StatusReportingObject.h>
#include <vector>

using namespace std;

class StatusUpdater {
public:
    StatusUpdater();
    ~StatusUpdater();

    void init(StatusReportingObject *sros[]);

    void update();

    void setInterval(int intervalms);
    int getInterval();

private:
    int _interval;
    // vector<StatusReportingObject> _sros;
};

#endif