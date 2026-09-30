#pragma once

#include <Arduino.h>
#include <string> 

namespace StringUtils {

String indentFromTag(const String& devTag);

bool isLikelyJson(const std::string& value);
bool isPrintableText(const std::string& s);

}
