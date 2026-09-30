#include "string_utils.h"


namespace StringUtils {

String indentFromTag(const String& devTag)
{
    String indent;

    for (size_t i = 0; i < devTag.length(); ++i) {
        indent += ' ';
    }

    return indent;
}

bool isLikelyJson(const std::string& value) {
    if (value.empty()) return false;

    // Führende Whitespaces überspringen
    size_t start = 0;
    while (start < value.size() && isspace((unsigned char)value[start])) start++;
    if (start >= value.size()) return false;

    char first = value[start];
    if (first != '{' && first != '[') return false;

    // Trailing Whitespaces überspringen, letztes Zeichen prüfen
    size_t end = value.size() - 1;
    while (end > start && isspace((unsigned char)value[end])) end--;

    char last = value[end];
    return (first == '{' && last == '}') || (first == '[' && last == ']');
}

// ===========================================================================
//  Helper: check if a raw string is printable ASCII (32–126).
//  Used to filter binary garbage from GATT characteristic values.
// ===========================================================================
bool isPrintableText(const std::string& s)
{
    if (s.empty())
        return false;

    for (unsigned char c : s)
    {
        if (c < 32 || c > 126)
            return false;
    }

    return true;
}

}
