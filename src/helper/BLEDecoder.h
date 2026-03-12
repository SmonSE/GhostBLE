#pragma once

#include <Arduino.h>
#include <string> 

void decodeBLEData(const std::string& uuid, uint8_t* data, size_t length);