#pragma once

#include <Arduino.h>
#include <string>

class SDLogger;

void decodeBLEData(const std::string& uuid, uint8_t* data, size_t length, SDLogger& sdLogger);