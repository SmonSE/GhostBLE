#include "GATTServiceInit.h"
#include "GATTServiceRegistry.h"
#include "genericAccessService.h"
#include "deviceInfoService.h"
#include "batteryLevelService.h"
#include "heartRateService.h"
#include "temperatureService.h"
#include "currentTimeService.h"
#include "txPowerService.h"
#include "immediateAlertService.h"
#include "linkLossService.h"

void registerGATTServiceHandlers()
{
    GATTServiceRegistry::registerService(
        "1800", "Generic Access",
        [](NimBLEClient* c) { return GenericAccessServiceHandler::readGenericAccessInfo(c); });

    GATTServiceRegistry::registerService(
        "180a", "Device Information",
        [](NimBLEClient* c) { return DeviceInfoServiceHandler::readDeviceInfo(c); });

    GATTServiceRegistry::registerService(
        "180f", "Battery",
        [](NimBLEClient* c) { return BatteryServiceHandler::readBatteryLevel(c); });

    GATTServiceRegistry::registerService(
        "180d", "Heart Rate",
        [](NimBLEClient* c) { return HeartRateServiceHandler::readHeartRate(c); });

    GATTServiceRegistry::registerService(
        "1809", "Health Thermometer",
        [](NimBLEClient* c) { return TemperatureServiceHandler::readTemperature(c); });

    GATTServiceRegistry::registerService(
        "1805", "Current Time",
        [](NimBLEClient* c) { return CurrentTimeServiceHandler::readCurrentTime(c); });

    GATTServiceRegistry::registerService(
        "1804", "TX Power",
        [](NimBLEClient* c) { return TxPowerServiceHandler::readTxPowerLevel(c); });

    GATTServiceRegistry::registerService(
        "1802", "Immediate Alert",
        [](NimBLEClient* c) { return ImmediateAlertServiceHandler::readImmediateAlert(c); });

    GATTServiceRegistry::registerService(
        "1803", "Link Loss",
        [](NimBLEClient* c) { return LinkLossServiceHandler::readLinkLoss(c); });
}
