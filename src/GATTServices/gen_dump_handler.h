#pragma once
#include <NimBLEClient.h>
#include <Arduino.h>
#include <string>

class GenericDumpHandler {
public:
    static String dumpService(NimBLEClient* pClient, const std::string& uuid);
};
