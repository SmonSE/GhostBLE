#include "ManufacturerHelper.h"

String getManufacturerName(uint16_t manufacturerId) {
    if (manufacturerId == 0x02E5) return "Espressif Systems (M5Stack)";
    if (manufacturerId == 0x004C) return "Apple, Inc.";
    if (manufacturerId == 0x0006) return "Microsoft Corporation";
    if (manufacturerId == 0x000F) return "Broadcom Corporation";
    if (manufacturerId == 0x0131) return "Google";
    if (manufacturerId == 0x0075) return "Samsung Electronics Co.";
    if (manufacturerId == 0x00E0) return "Nintendo Co., Ltd.";
    if (manufacturerId == 0x0001) return "Ericsson Technology Licensing";
    if (manufacturerId == 0x0002) return "Intel Corp.";
    if (manufacturerId == 0x0003) return "IBM Corp.";
    if (manufacturerId == 0x0004) return "Toshiba Corp.";
    if (manufacturerId == 0x0005) return "3Com";
    if (manufacturerId == 0x0012) return "Matsushita Electric Industrial Co.";
    if (manufacturerId == 0x001D) return "Motorola";
    if (manufacturerId == 0x0025) return "Nokia Mobile Phones";
    if (manufacturerId == 0x003D) return "Hitachi, Ltd";
    if (manufacturerId == 0x0065) return "Sony Ericsson Mobile Communications";
    if (manufacturerId == 0x0079) return "LG Electronics";
    if (manufacturerId == 0x00A0) return "Qualcomm Inc.";
    if (manufacturerId == 0x00C7) return "Garmin International, Inc.";
    if (manufacturerId == 0x00D2) return "GoPro, Inc.";
    if (manufacturerId == 0x00E1) return "Bosch Sensortec GmbH";
    if (manufacturerId == 0x00EC) return "Sony Corporation";
    if (manufacturerId == 0x0106) return "Tile, Inc.";
    if (manufacturerId == 0x0110) return "Fitbit, Inc.";
    if (manufacturerId == 0x012D) return "Signify (Philips Lighting B.V.)";
    if (manufacturerId == 0x0133) return "Facebook Technologies, LLC";
    if (manufacturerId == 0x0165) return "Xiaomi Inc.";
    if (manufacturerId == 0x0171) return "OPPO Mobile Telecommunications Corp.";
    if (manufacturerId == 0x017C) return "Huawei Technologies Co., Ltd.";
    if (manufacturerId == 0x018B) return "vivo Mobile Communication Co., Ltd.";
    if (manufacturerId == 0x0195) return "OnePlus Electronics Corp.";
    if (manufacturerId == 0x0065) return "Garmin International, Inc.";
    
    return "Unknown Manufacturer";
}
  