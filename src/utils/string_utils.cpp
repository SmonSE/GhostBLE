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

}
