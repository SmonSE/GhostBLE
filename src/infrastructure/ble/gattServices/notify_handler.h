#pragma once

#include <NimBLEClient.h>
#include <Arduino.h>
#include <vector>
#include <string>

// ============================================================
//  NotifyHandler
//
//  Subscribes to ALL notifiable / indicatable characteristics
//  found on the connected client — across all services.
//
//  Design:
//    - One subscribe() call per characteristic, with a 3-second
//      capture window after the last subscription.
//    - Each notification callback decodes the raw payload and
//      logs a human-readable result where the UUID is known,
//      or a hex dump for unknown characteristics.
//    - XP is awarded per notification received.
//    - No static flags — subscriptions are tracked per-call
//      via a local set, so each new device gets a clean slate.
//
//  Call from connectAndReadGATT() after attribute discovery,
//  before disconnecting.
// ============================================================

namespace NotifyHandler {

// Subscribe to all notifiable characteristics on pClient.
// Blocks for captureWindowMs after the last subscription to
// collect incoming notifications, then unsubscribes cleanly.
// Returns a log string summarising what was captured.
String subscribeAndCapture(NimBLEClient* pClient,
                           uint32_t      captureWindowMs = 3000);
            
int    lastNotifyCount();                       

} // namespace NotifyHandler
