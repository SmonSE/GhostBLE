// device_finder.cpp
#include "device_finder.h"
#include <NimBLEDevice.h>

#include "app/context/scan_context.h"
#include "ui/finder/finder_list_view.h"
#include "infrastructure/logging/logger.h"

namespace DeviceFinder {

static FoundDevice list_[MAX_FOUND];
static int count_ = 0;

void scan5s() {
    count_ = 0;
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->clearResults();
    pScan->setActiveScan(true);

    NimBLEScanResults results = pScan->getResults(10000);  // 10 Sekunden

    for (int i = 0; i < results.getCount() && count_ < MAX_FOUND; i++) {
        const NimBLEAdvertisedDevice* d = results.getDevice(i);
        if (!d) continue;

        FoundDevice& fd = list_[count_];
        String nm = d->haveName() ? d->getName().c_str() : "(unknown)";
        strncpy(fd.name, nm.c_str(), sizeof(fd.name) - 1);
        fd.name[sizeof(fd.name) - 1] = '\0';

        strncpy(fd.mac, d->getAddress().toString().c_str(), sizeof(fd.mac) - 1);
        fd.mac[sizeof(fd.mac) - 1] = '\0';

        fd.rssi = d->getRSSI();
        count_++;
    }
}

int count() { return count_; }
const FoundDevice& get(int index) { return list_[index]; }

void startFinderFlow() {
    bool wasScanning = ScanContext::bleScanEnabled.load();
    if (wasScanning) {
        LOG(LOG_CONTROL, "Find Device — stopping main scan");
        ScanContext::bleScanEnabled.store(false);
        vTaskDelay(pdMS_TO_TICKS(300));
    }

    LOG(LOG_CONTROL, "Find Device — scanning 10s...");
    scan5s();

    FinderListView::open();
}

} // namespace DeviceFinder
