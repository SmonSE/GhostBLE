#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>
#include <vector>
#include <string>
#include <functional>

// A registered GATT service handler: UUID -> reader function
struct GATTServiceEntry {
    std::string uuid;          // e.g. "1800", "180f"
    std::string label;         // human-readable name for logging
    std::function<String(NimBLEClient*)> handler;
};

// Central registry for GATT service handlers.
// Services are matched against discovered UUIDs during connection;
// unmatched services fall through to the generic characteristic reader.
class GATTServiceRegistry {
public:
    // Register a service handler
    static void registerService(const std::string& uuid,
                                const std::string& label,
                                std::function<String(NimBLEClient*)> handler);

    // Run all registered handlers whose UUID was discovered on the client.
    // Returns combined log output from all matched handlers.
    static String runDiscoveredHandlers(NimBLEClient* pClient);

    // Access the registry (e.g. for testing)
    static const std::vector<GATTServiceEntry>& entries();

private:
    static std::vector<GATTServiceEntry>& registry();
};
