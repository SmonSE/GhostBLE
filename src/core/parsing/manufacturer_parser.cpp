#include "manufacturer_parser.h"

// ============================================================
//  Company Identifiers
//  Source: Bluetooth Assigned Numbers, Section 7
//  Version: 2026-04-30
//  https://www.bluetooth.com/specifications/assigned-numbers/
//
//  Sorted by ID ascending.
//  Custom / unofficial entries marked with // (unofficial)
// ============================================================

String getManufacturerName(uint16_t id) {
    switch (id) {

    // ── 0x0000 – 0x00FF ──────────────────────────────────────
    case 0x0001: return "Ericsson Technology Licensing";
    case 0x0002: return "Intel Corp.";
    case 0x0003: return "IBM Corp.";
    case 0x0004: return "Toshiba Corp.";
    case 0x0005: return "3Com";
    case 0x0006: return "Microsoft Corporation";
    case 0x000F: return "Broadcom Corporation";
    case 0x0012: return "Matsushita Electric Industrial Co.";
    case 0x001D: return "Motorola";
    case 0x0025: return "Nokia Mobile Phones";
    case 0x002D: return "Hewlett-Packard Company";
    case 0x0030: return "Symbol Technologies";
    case 0x003D: return "Hitachi, Ltd";
    case 0x0040: return "Epson America, Inc.";
    case 0x0057: return "Harman International Industries, Inc.";
    case 0x0065: return "HP, Inc.";
    case 0x006B: return "Polar Electro Oy";
    case 0x0075: return "Samsung Electronics Co. Ltd.";
    case 0x0079: return "LG Electronics";
    case 0x007F: return "Lenovo Group Ltd.";
    case 0x0087: return "Garmin International, Inc.";
    case 0x00A0: return "Qualcomm Inc.";
    case 0x00AF: return "Plantronics (Poly)";
    case 0x00C3: return "Bose Corporation";
    case 0x00C7: return "Garmin International, Inc.";
    case 0x00D2: return "GoPro, Inc.";
    case 0x00E0: return "Nintendo Co., Ltd.";
    case 0x00E1: return "Bosch Sensortec GmbH";
    case 0x00E7: return "Polar Electro Oy";
    case 0x00EC: return "Sony Corporation";

    // ── 0x0100 – 0x01FF ──────────────────────────────────────
    case 0x0106: return "Tile, Inc.";
    case 0x0110: return "Fitbit, Inc.";
    case 0x0131: return "Google LLC";
    case 0x012D: return "Signify Netherlands B.V. (Philips Lighting)";
    case 0x0133: return "Facebook Technologies, LLC";
    case 0x01A6: return "Realme Mobile Telecommunications Co., Ltd.";
    case 0x01AB: return "Meta Platforms, Inc.";
    case 0x01D6: return "Anker Innovations Limited";
    case 0x01DC: return "Wyze Labs, Inc.";
    case 0x01E3: return "Amazfit (Huami Information Technology)";

    // ── 0x0200 – 0x02FF ──────────────────────────────────────
    case 0x02E5: return "Espressif Systems (M5Stack)";

    // ── 0x0300 – 0x03FF ──────────────────────────────────────
    case 0x03FF: return "Withings";  // unofficial

    // ── 0x004C (Apple — special, must match) ─────────────────
    case 0x004C: return "Apple, Inc.";

    // ── 0x05A7 ───────────────────────────────────────────────
    case 0x05A7: return "Samsung Electronics Co. Ltd.";

    // ── 0x0BA3 ───────────────────────────────────────────────
    case 0x0BA3: return "Sonova Consumer Hearing GmbH (Sennheiser)";

    // ── 0x096 ────────────────────────────────────────────────
    case 0x0969: return "Woan Technology (Shenzhen) Co., Ltd.";

    // ── Larger IDs ────────────────────────────────────────────
    case 0x0171: return "OPPO Mobile Telecommunications Corp., Ltd.";
    case 0x017C: return "Huawei Technologies Co., Ltd.";
    case 0x018B: return "vivo Mobile Communication Co., Ltd.";
    case 0x0195: return "OnePlus Electronics Corp. (Pvt. Ltd.)";
    case 0x0165: return "Xiaomi Inc.";
    case 0x0ECB: return "Zhong Shan City Richsound Electronic Industrial Ltd. (JBL)";

    default: return "";  // empty = Unknown, caller decides label
    }
}

// ============================================================
//  Member Service UUIDs (Section 3.11)
//  Maps a 16-bit service UUID to the owning company name.
//  Useful for identifying who advertises an unknown service.
// ============================================================
String getMemberServiceOwner(uint16_t uuid) {
    switch (uuid) {

    // Apple
    case 0xFC94: return "Apple Inc.";
    case 0xFCA0: return "Apple Inc.";
    case 0xFCB2: return "Apple Inc.";
    case 0xFD43: return "Apple Inc.";
    case 0xFD44: return "Apple Inc.";
    case 0xFD6F: return "Apple Inc.";  // Exposure Notification
    case 0xFE13: return "Apple Inc.";
    case 0xFE25: return "Apple Inc.";
    case 0xFE8A: return "Apple Inc.";
    case 0xFE8B: return "Apple Inc.";
    case 0xFEC7: return "Apple Inc.";
    case 0xFEC8: return "Apple Inc.";
    case 0xFEC9: return "Apple Inc.";
    case 0xFECA: return "Apple Inc.";
    case 0xFECB: return "Apple Inc.";
    case 0xFECC: return "Apple Inc.";
    case 0xFECD: return "Apple Inc.";
    case 0xFECE: return "Apple Inc.";
    case 0xFECF: return "Apple Inc.";
    case 0xFED0: return "Apple Inc.";
    case 0xFED1: return "Apple Inc.";
    case 0xFED2: return "Apple Inc.";
    case 0xFED3: return "Apple Inc.";
    case 0xFED4: return "Apple Inc.";

    // Google
    case 0xFC3E: return "Google LLC";
    case 0xFC56: return "Google LLC";
    case 0xFC73: return "Google LLC";
    case 0xFCB1: return "Google LLC";
    case 0xFCCF: return "Google LLC";
    case 0xFCF1: return "Google LLC";
    case 0xFD36: return "Google LLC";
    case 0xFD62: return "Google LLC";
    case 0xFD63: return "Google LLC";
    case 0xFD87: return "Google LLC";
    case 0xFD8C: return "Google LLC";
    case 0xFD96: return "Google LLC";
    case 0xFDE2: return "Google LLC";
    case 0xFDF0: return "Google LLC";
    case 0xFEAA: return "Google LLC";  // Eddystone
    case 0xFED8: return "Google LLC";
    case 0xFEF3: return "Google LLC";
    case 0xFEF4: return "Google LLC";
    case 0xFE19: return "Google LLC";
    case 0xFE26: return "Google LLC";
    case 0xFE27: return "Google LLC";
    case 0xFE2C: return "Google LLC";
    case 0xFE50: return "Google LLC";
    case 0xFE55: return "Google LLC";
    case 0xFE56: return "Google LLC";
    case 0xFE9F: return "Google LLC";
    case 0xFEA0: return "Google LLC";

    // Samsung
    case 0xFC91: return "Samsung Electronics Co., Ltd.";
    case 0xFD1D: return "Samsung Electronics Co., Ltd.";
    case 0xFD4B: return "Samsung Electronics Co., Ltd.";
    case 0xFD59: return "Samsung Electronics Co., Ltd.";
    case 0xFD5A: return "Samsung Electronics Co., Ltd.";
    case 0xFD69: return "Samsung Electronics Co., Ltd.";
    case 0xFD6C: return "Samsung Electronics Co., Ltd.";
    case 0xFD7E: return "Samsung Electronics Co., Ltd.";
    case 0xFDDB: return "Samsung Electronics Co., Ltd.";

    // Tesla
    case 0xFE96: return "Tesla Motors Inc.";
    case 0xFE97: return "Tesla Motors Inc.";

    // Xiaomi
    case 0xFC46: return "Xiaomi Inc.";
    case 0xFC66: return "Xiaomi Inc.";
    case 0xFC75: return "Xiaomi Inc.";
    case 0xFCC0: return "Xiaomi Inc.";
    case 0xFD2D: return "Xiaomi Inc.";
    case 0xFDAA: return "Xiaomi Inc.";
    case 0xFDAB: return "Xiaomi Inc.";
    case 0xFE95: return "Xiaomi Inc.";

    // Huawei
    case 0xFCF7: return "Honor Device Co., Ltd.";
    case 0xFCF8: return "Honor Device Co., Ltd.";
    case 0xFD21: return "Huawei Technologies Co., Ltd.";
    case 0xFD22: return "Huawei Technologies Co., Ltd.";
    case 0xFD9A: return "Huawei Technologies Co., Ltd.";
    case 0xFD9B: return "Huawei Technologies Co., Ltd.";
    case 0xFD9C: return "Huawei Technologies Co., Ltd.";
    case 0xFDD0: return "Huawei Technologies Co., Ltd.";
    case 0xFDD1: return "Huawei Technologies Co., Ltd.";
    case 0xFDEE: return "Huawei Technologies Co., Ltd.";
    case 0xFE35: return "HUAWEI Technologies Co., Ltd.";
    case 0xFE36: return "HUAWEI Technologies Co., Ltd.";
    case 0xFE86: return "HUAWEI Technologies Co., Ltd.";

    // Garmin
    case 0xFE1F: return "Garmin International, Inc.";

    // Sonos
    case 0xFC6A: return "Sonos Inc.";
    case 0xFC6B: return "Sonos Inc.";
    case 0xFE07: return "Sonos, Inc.";

    // Tile
    case 0xFD84: return "Tile, Inc.";
    case 0xFEEC: return "Tile, Inc.";
    case 0xFEED: return "Tile, Inc.";

    // Polar
    case 0xFEEE: return "Polar Electro Oy";
    case 0xFEEF: return "Polar Electro Oy";

    // Amazon
    case 0xFCDC: return "Amazon.com Services, LLC";
    case 0xFD41: return "Amazon Lab126";
    case 0xFE00: return "Amazon.com Services, Inc.";
    case 0xFE03: return "Amazon.com Services, Inc.";
    case 0xFE15: return "Amazon.com Services, Inc.";

    // Bose
    case 0xFC8F: return "Bose Corporation";
    case 0xFDD2: return "Bose Corporation";
    case 0xFEBE: return "Bose Corporation";
    case 0xFE21: return "Bose Corporation";

    // Woan Technology (from your log)
    case 0xFD3D: return "Woan Technology (Shenzhen) Co., Ltd.";

    // Nordic Semiconductor (common in BLE dev boards)
    case 0xFE58: return "Nordic Semiconductor ASA";
    case 0xFE59: return "Nordic Semiconductor ASA";
    case 0xFEE4: return "Nordic Semiconductor ASA";
    case 0xFEE5: return "Nordic Semiconductor ASA";

    // Microsoft
    case 0xFE08: return "Microsoft";
    case 0xFEB2: return "Microsoft Corporation";

    // Meta
    case 0xFEB7: return "Meta Platforms, Inc.";
    case 0xFEB8: return "Meta Platforms, Inc.";
    case 0xFD5F: return "Meta Platforms Technologies, LLC";

    // Fitbit / Google Health
    case 0xFEE0: return "Anhui Huami Information Technology (Amazfit)";
    case 0xFEE1: return "Anhui Huami Information Technology (Amazfit)";

    // Withings
    case 0xFD77: return "Withings";
    case 0xFD78: return "Withings";
    case 0xFD79: return "Withings";
    case 0xFD7A: return "Withings";

    // Oura
    case 0xFDB0: return "Oura Health Ltd.";
    case 0xFDB1: return "Oura Health Ltd.";

    // Harman / JBL
    case 0xFC39: return "Harman International";
    case 0xFC69: return "Harman International";
    case 0xFC7E: return "Harman International";
    case 0xFDDF: return "Harman International";

    // Sennheiser / Sonova
    case 0xFCFE: return "Sonova Consumer Hearing GmbH (Sennheiser)";
    case 0xFDCE: return "SENNHEISER electronic GmbH & Co. KG";

    // Medtronic
    case 0xFCA8: return "Medtronic Inc.";
    case 0xFCA9: return "Medtronic Inc.";
    case 0xFE81: return "Medtronic Inc.";
    case 0xFE82: return "Medtronic Inc.";

    // Abbott (medical)
    case 0xFD86: return "Abbott";
    case 0xFDE3: return "Abbott Diabetes Care";
    case 0xFE72: return "Abbott (formerly St. Jude Medical)";
    case 0xFE73: return "Abbott (formerly St. Jude Medical)";

    // Dexcom (CGM)
    case 0xFEBC: return "Dexcom Inc.";

    // GN Hearing
    case 0xFD20: return "GN Hearing A/S";
    case 0xFD71: return "GN Hearing A/S";
    case 0xFEFE: return "GN Hearing A/S";
    case 0xFEFF: return "GN Netcom";

    default: return "";
    }
}