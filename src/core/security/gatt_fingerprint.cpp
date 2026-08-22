#include "gatt_fingerprint.h"


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

    result += "GATT Fingerprint:\n";
    result += "  Services: " + String(services) + "\n";
    result += "  Proprietary: " + String(proprietaryServices) + "\n";
    result += "  Standard: " + String(standardServices) + "\n";
    result += "  Characteristics: " + String(characteristics) + "\n";
    result += "  Read: " + String(read) + "\n";
    result += "  Write: " + String(write) + "\n";
    result += "  ReadWrite: " + String(readWrite) + "\n";
    result += "  Notify: " + String(notify) + "\n";
    result += "  Indicate: " + String(indicate);

    return result;
}
