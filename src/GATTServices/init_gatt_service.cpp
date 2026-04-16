#include "init_gatt_service.h"
#include "reg_gatt_service.h"
#include "gen_access_service.h"
#include "gen_attribute_service.h"
#include "device_info_service.h"
#include "battery_lvl_service.h"
#include "heart_rate_service.h"
#include "temperature_service.h"
#include "current_time_service.h"
#include "tx_power_service.h"
#include "immediate_alert_service.h"
#include "link_loss_service.h"
#include "hid_service.h"
#include "env_sensing_service.h"
#include "alert_notifi_service.h"
#include "phone_alert_status_service.h"
#include "gen_dump_handler.h"


void registerGATTServiceHandlers()
{
    GATTServiceRegistry::registerService(
        "1800", "Generic Access",
        [](NimBLEClient* c) { return GenericAccessServiceHandler::readGenericAccessInfo(c); });

    GATTServiceRegistry::registerService(
        "1801", "Generic Attribute",
        [](NimBLEClient* c) { return GenericAttributeServiceHandler::readGenericAttribute(c); });

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

    GATTServiceRegistry::registerService(
        "1812", "Human Interface Device",
        [](NimBLEClient* c) { return HIDServiceHandler::readHID(c); });

    GATTServiceRegistry::registerService(
        "181a", "Environmental Sensing",
        [](NimBLEClient* c) { return EnvironmentalSensingServiceHandler::readEnvironmentalSensing(c); });

    GATTServiceRegistry::registerService(
        "1811", "Alert Notification",
        [](NimBLEClient* c) { return AlertNotificationServiceHandler::readAlertNotification(c); });

    GATTServiceRegistry::registerService(
        "180e", "Phone Alert Status",
        [](NimBLEClient* c) { return PhoneAlertStatusServiceHandler::readPhoneAlertStatus(c); });

    // Fallback: dump characteristics for any unregistered service
    GATTServiceRegistry::registerFallback(
        [](NimBLEClient* c, std::string uuid) { return GenericDumpHandler::dumpService(c, uuid); });
}
