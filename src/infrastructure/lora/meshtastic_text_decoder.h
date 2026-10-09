#pragma once

// Minimal Meshtastic channel decoder for GhostBLE (text messages only).
// Requires the ESP32 Arduino mbedTLS AES implementation.
// Place this header next to lora_scanner.cpp.

#include <Arduino.h>
#include <stddef.h>
#include <stdint.h>
#include <mbedtls/aes.h>

namespace MeshtasticTextDecoder {

static constexpr uint8_t DEFAULT_PSK[16] = {
    0xD4, 0xF1, 0xBB, 0x3A, 0x20, 0x29, 0x07, 0x59,
    0xF0, 0xBC, 0xFF, 0xAB, 0xCF, 0x4E, 0x69, 0x01
};

static constexpr uint32_t TEXT_MESSAGE_APP = 1;
static constexpr size_t MAX_PLAINTEXT = 256;

inline uint32_t readLE32(const uint8_t* p) {
    return (uint32_t)p[0]
        | ((uint32_t)p[1] << 8)
        | ((uint32_t)p[2] << 16)
        | ((uint32_t)p[3] << 24);
}

inline bool readVarint(const uint8_t* data, size_t len, size_t& pos, uint64_t& value) {
    value = 0;
    for (unsigned i = 0; i < 10 && pos < len; ++i) {
        const uint8_t b = data[pos++];
        if (i == 9 && (b & 0xFE) != 0) return false; // uint64 overflow
        value |= (uint64_t)(b & 0x7F) << (7 * i);
        if ((b & 0x80) == 0) return true;
    }
    return false;
}

inline bool skipField(const uint8_t* data, size_t len, size_t& pos, uint8_t wireType) {
    uint64_t n = 0;
    switch (wireType) {
        case 0:
            return readVarint(data, len, pos, n);
        case 1:
            if (len - pos < 8) return false;
            pos += 8;
            return true;
        case 2:
            if (!readVarint(data, len, pos, n) || n > len - pos) return false;
            pos += (size_t)n;
            return true;
        case 5:
            if (len - pos < 4) return false;
            pos += 4;
            return true;
        default:
            // Groups (wire types 3/4) are not expected in Meshtastic Data.
            return false;
    }
}

inline bool validUtf8Text(const uint8_t* s, size_t n) {
    if (n == 0) return false;
    size_t i = 0;
    while (i < n) {
        const uint8_t c = s[i];
        if (c < 0x80) {
            if (c == 0 || (c < 0x20 && c != '\n' && c != '\r' && c != '\t')) return false;
            ++i;
            continue;
        }

        uint32_t cp = 0;
        size_t extra = 0;
        if (c >= 0xC2 && c <= 0xDF) {
            cp = c & 0x1F; extra = 1;
        } else if (c >= 0xE0 && c <= 0xEF) {
            cp = c & 0x0F; extra = 2;
        } else if (c >= 0xF0 && c <= 0xF4) {
            cp = c & 0x07; extra = 3;
        } else {
            return false;
        }
        if (n - i <= extra) return false;
        for (size_t j = 1; j <= extra; ++j) {
            const uint8_t cc = s[i + j];
            if ((cc & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (cc & 0x3F);
        }
        if ((extra == 2 && cp < 0x800) ||
            (extra == 3 && cp < 0x10000) ||
            (cp >= 0xD800 && cp <= 0xDFFF) ||
            cp > 0x10FFFF) {
            return false;
        }
        i += extra + 1;
    }
    return true;
}

// Parse the decrypted protobuf Data wrapper and extract payload only when
// portnum == TEXT_MESSAGE_APP (1). Unknown protobuf fields are safely skipped.
inline bool parseTextData(const uint8_t* plain, size_t len, String& text) {
    size_t pos = 0;
    bool havePortnum = false;
    bool havePayload = false;
    uint64_t portnum = 0;
    const uint8_t* payload = nullptr;
    size_t payloadLen = 0;

    while (pos < len) {
        uint64_t tag = 0;
        if (!readVarint(plain, len, pos, tag) || tag == 0) return false;
        const uint32_t field = (uint32_t)(tag >> 3);
        const uint8_t wire = (uint8_t)(tag & 0x07);
        if (field == 0) return false;

        if (field == 1 && wire == 0) {
            if (!readVarint(plain, len, pos, portnum)) return false;
            havePortnum = true;
        } else if (field == 2 && wire == 2) {
            uint64_t n = 0;
            if (!readVarint(plain, len, pos, n) || n > len - pos) return false;
            payload = plain + pos;
            payloadLen = (size_t)n;
            pos += payloadLen;
            havePayload = true;
        } else {
            if (!skipField(plain, len, pos, wire)) return false;
        }
    }

    if (!havePortnum || !havePayload || portnum != TEXT_MESSAGE_APP) return false;
    if (payloadLen >= MAX_PLAINTEXT || !validUtf8Text(payload, payloadLen)) return false;

    char textBuffer[MAX_PLAINTEXT];
    for (size_t i = 0; i < payloadLen; ++i) textBuffer[i] = (char)payload[i];
    textBuffer[payloadLen] = '\0';
    text = String(textBuffer);
    return true;
}

// Decrypt a Meshtastic channel packet using the well-known default PSK ("AQ==").
// Nonce layout: packet_id (uint64 LE) + sender node ID (uint32 LE) + CTR block counter.
// This only decodes the default channel key; it does not decode PKI-encrypted DMs
// or packets encrypted with a different channel PSK.
inline bool decodeDefaultText(const uint8_t* ciphertext, size_t ciphertextLen,
                              uint32_t packetId, uint32_t fromNode, String& text) {
    if (!ciphertext || ciphertextLen == 0 || ciphertextLen > MAX_PLAINTEXT) return false;

    uint8_t nonceCounter[16] = {};
    // Packet ID is a uint32 on the over-the-air header, zero-extended to uint64.
    nonceCounter[0] = (uint8_t)(packetId);
    nonceCounter[1] = (uint8_t)(packetId >> 8);
    nonceCounter[2] = (uint8_t)(packetId >> 16);
    nonceCounter[3] = (uint8_t)(packetId >> 24);
    nonceCounter[8] = (uint8_t)(fromNode);
    nonceCounter[9] = (uint8_t)(fromNode >> 8);
    nonceCounter[10] = (uint8_t)(fromNode >> 16);
    nonceCounter[11] = (uint8_t)(fromNode >> 24);

    uint8_t streamBlock[16] = {};
    uint8_t plaintext[MAX_PLAINTEXT] = {};
    size_t ncOffset = 0;

    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    int rc = mbedtls_aes_setkey_enc(&aes, DEFAULT_PSK, 128);
    if (rc == 0) {
        rc = mbedtls_aes_crypt_ctr(&aes, ciphertextLen, &ncOffset,
                                   nonceCounter, streamBlock,
                                   ciphertext, plaintext);
    }
    mbedtls_aes_free(&aes);
    if (rc != 0) return false;

    return parseTextData(plaintext, ciphertextLen, text);
}

} // namespace MeshtasticTextDecoder
