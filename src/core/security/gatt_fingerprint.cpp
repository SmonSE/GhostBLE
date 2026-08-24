#include "gatt_fingerprint.h"

#include "app/context/globals.h"
#include "utils/string_utils.h"


void GATTFingerprint::reset()
{
    services = 0;
    proprietaryServices = 0;
    standardServices = 0;
    characteristics = 0;

    read = 0;
    write = 0;
    readWrite = 0;

    notify = 0;
    indicate = 0;
}

String GATTFingerprint::toString() const
{
    String result;
    String indent = StringUtils::indentFromTag(devTag);

    result += devTag + "GATT Fingerprint:\n";
    result += indent + "- Services: " + String(services) + "\n";
    result += indent + "- Proprietary: " + String(proprietaryServices) + "\n";
    result += indent + "- Standard: " + String(standardServices) + "\n";
    result += indent + "- Characteristics: " + String(characteristics) + "\n";
    result += indent + "- Read: " + String(read) + "\n";
    result += indent + "- Write: " + String(write) + "\n";
    result += indent + "- ReadWrite: " + String(readWrite) + "\n";
    result += indent + "- Notify: " + String(notify) + "\n";
    result += indent + "- Indicate: " + String(indicate);

    return result;
}
