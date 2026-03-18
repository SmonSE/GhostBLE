#include "GATTServiceRegistry.h"
#include "../logger/logger.h"

std::vector<GATTServiceEntry>& GATTServiceRegistry::registry()
{
    static std::vector<GATTServiceEntry> reg;
    return reg;
}

void GATTServiceRegistry::registerService(
    const std::string& uuid,
    const std::string& label,
    std::function<String(NimBLEClient*)> handler)
{
    registry().push_back({uuid, label, handler});
}

const std::vector<GATTServiceEntry>& GATTServiceRegistry::entries()
{
    return registry();
}

String GATTServiceRegistry::runDiscoveredHandlers(NimBLEClient* pClient)
{
    if (!pClient || !pClient->isConnected()) return "";

    String combined;

    // Collect discovered service UUIDs
    auto services = pClient->getServices();

    for (auto& entry : registry()) {
        // Check if this service UUID is present on the device
        NimBLERemoteService* svc = pClient->getService(entry.uuid.c_str());
        if (!svc) continue;

        String result = entry.handler(pClient);
        if (!result.isEmpty()) {
            if (!combined.isEmpty()) combined += "\n";
            combined += result;
        }
    }

    return combined;
}
