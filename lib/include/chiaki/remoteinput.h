// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#ifndef CHIAKI_REMOTEINPUT_H
#define CHIAKI_REMOTEINPUT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "common.h"
#include "controller.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Remote input: controller state received from an external process
 * (fila-gta home agent) over UDP on localhost.
 *
 * Packet layout, 18 bytes, little endian:
 *   u8 version (=1) | u8 flags | u16 seq | u32 buttons | u8 l2 | u8 r2 |
 *   i16 lx | i16 ly | i16 rx | i16 ry
 */
#define CHIAKI_REMOTE_INPUT_PACKET_SIZE 18
#define CHIAKI_REMOTE_INPUT_VERSION 1
#define CHIAKI_REMOTE_INPUT_FLAG_ADMIN 0x01
#define CHIAKI_REMOTE_INPUT_VALID_BUTTONS 0xFFFFu
#define CHIAKI_REMOTE_INPUT_BLOCKED_BUTTONS (CHIAKI_CONTROLLER_BUTTON_PS | CHIAKI_CONTROLLER_BUTTON_OPTIONS | CHIAKI_CONTROLLER_BUTTON_SHARE)

typedef struct chiaki_remote_input_packet_t
{
	uint8_t flags;
	uint16_t seq;
	uint32_t buttons;
	uint8_t l2, r2;
	int16_t lx, ly, rx, ry;
} ChiakiRemoteInputPacket;

/**
 * @return CHIAKI_ERR_INVALID_DATA if len is not exactly CHIAKI_REMOTE_INPUT_PACKET_SIZE,
 *         CHIAKI_ERR_VERSION_MISMATCH if the version byte is not CHIAKI_REMOTE_INPUT_VERSION
 */
CHIAKI_EXPORT ChiakiErrorCode chiaki_remote_input_parse(const uint8_t *buf, size_t len, ChiakiRemoteInputPacket *out);

/**
 * @param has_last false if no packet was accepted yet (or after the deadman timeout)
 * @return whether seq is newer than last (16 bit wraparound aware)
 */
CHIAKI_EXPORT bool chiaki_remote_input_accept_seq(bool has_last, uint16_t last, uint16_t seq);

/**
 * Converts a packet to a full controller state (touches idle).
 * Drops undefined button bits and, unless CHIAKI_REMOTE_INPUT_FLAG_ADMIN is set,
 * the buttons in CHIAKI_REMOTE_INPUT_BLOCKED_BUTTONS.
 */
CHIAKI_EXPORT void chiaki_remote_input_to_state(const ChiakiRemoteInputPacket *pkt, ChiakiControllerState *state);

#ifdef __cplusplus
}
#endif

#endif // CHIAKI_REMOTEINPUT_H
