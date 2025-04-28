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
    if (manufacturerId == 0x0065) return "Garmin International, Inc.";   // 16-bit balue
    if (manufacturerId == 0xECB) return "JBL by Harman International";   // 12-bit value
    // NEW
    if (manufacturerId == 0x007F) return "Lenovo";
    if (manufacturerId == 0x01A6) return "Realme Mobile Telecommunications";
    if (manufacturerId == 0x01D6) return "Anker Innovations Limited";
    if (manufacturerId == 0x01E3) return "Amazfit (Huami)";
    if (manufacturerId == 0x01DC) return "Wyze Labs, Inc.";
    if (manufacturerId == 0x00E7) return "Polar Electro Oy (Heart rate devices!)";
    if (manufacturerId == 0x00C3) return "Bose Corporation"; // 0x00C3 is Bose, not Plantronics
    if (manufacturerId == 0x00AF) return "Plantronics (now Poly)";
    if (manufacturerId == 0x0030) return "Symbol Technologies (barcode scanners)";
    
    return "Unknown Manufacturer";
}

  