#pragma once
#include <NimBLEClient.h>
#include <Arduino.h>
#include <string>

class UnknownGATTHandler {
public:
    static String dumpService(NimBLEClient* pClient, const std::string& uuid);

private:
    static String getUtf8Text(const std::string& data);
    static bool isLikelyUtf8Text(const std::string& data);
};
