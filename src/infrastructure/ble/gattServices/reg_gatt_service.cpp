#include "reg_gatt_service.h"
#include "infrastructure/logging/logger.h"
#include <NimBLERemoteService.h>
#include <set>

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

std::map<std::string, String>& GATTServiceRegistry::results()
{
    static std::map<std::string, String> res;
    return res;
}

std::function<String(NimBLEClient*, std::string)>& GATTServiceRegistry::fallback()
{
    static std::function<String(NimBLEClient*, std::string)> fb;
    return fb;
}

void GATTServiceRegistry::registerFallback(
    std::function<String(NimBLEClient*, std::string)> handler)
{
    fallback() = handler;
}

String GATTServiceRegistry::getLastResult(const std::string& uuid)
{
    auto it = results().find(uuid);
    if (it != results().end()) return it->second;
    return "";
}

String GATTServiceRegistry::runDiscoveredHandlers(NimBLEClient* pClient)
{
    if (!pClient || !pClient->isConnected()) return "";

    results().clear();
    String combined;

    std::set<std::string> registeredUUIDs;
    for (auto& entry : registry()) {
        registeredUUIDs.insert(entry.uuid);
    }

    for (auto& entry : registry()) {
        NimBLERemoteService* svc = pClient->getService(entry.uuid.c_str());
        if (!svc) continue;

        String result = entry.handler(pClient);
        results()[entry.uuid] = result;
        if (!result.isEmpty()) {
            if (!combined.isEmpty()) combined += "\n";
            combined += result;
        }
    }

    // Run fallback handler for any unregistered services
    if (fallback()) {
        for (auto* svc : pClient->getServices()) {
            std::string uuid = svc->getUUID().toString();

            if (uuid.substr(0, 2) == "0x") {
                uuid = uuid.substr(2);
            }

            if (registeredUUIDs.count(uuid)) continue;

            std::string lower = uuid;
            for (auto& ch : lower) ch = tolower(ch);
            if (registeredUUIDs.count(lower)) continue;

            String result = fallback()(pClient, uuid);
            results()[uuid] = result;
            if (!result.isEmpty()) {
                if (!combined.isEmpty()) combined += "\n";
                combined += result;
            }
        }
    }

    return combined;
}
