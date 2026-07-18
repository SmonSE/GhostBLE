// ui/finder/approach_view.h
#pragma once
#include <Arduino.h>
namespace ApproachView {
    void open(const char* mac, const char* name);
    void close();
    bool isOpen();
    void update();   // regelmäßig aus loop() aufrufen — macht kurzen gezielten Scan + redraw
}
