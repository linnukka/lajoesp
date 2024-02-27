#include <StatusUpdater.h>

StatusUpdater::StatusUpdater() {}

StatusUpdater::~StatusUpdater() {}

void StatusUpdater::init(StatusReportingObject *sros[]){
    // _sros.resize(sizeof(&sros));
    // for(int i=0;i<sizeof(&sros);i++){
    //     _sros[i] = *sros[i];
    // }
}

void StatusUpdater::setInterval(int intervalms){
    _interval = intervalms;
}
int StatusUpdater::getInterval(){
    return _interval;
}

void StatusUpdater::update(){
    // int size = _sros.size(); 
    // if(size>0){
    //     for(int i=0;i<size;i++){
    //         _sros[i].update();
    //     }
    // }
}