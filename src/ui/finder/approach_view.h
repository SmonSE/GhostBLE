// ui/finder/approach_view.h
#pragma once
#include <Arduino.h>


namespace ApproachView {
    enum class ReturnTarget { FinderList, SusList };
    void open(const char* mac, const char* name, ReturnTarget returnTo);
    void close();
    bool isOpen();
    void update();
}