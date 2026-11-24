// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <chiaki/remoteinput.h>
#include <chiaki/seqnum.h>

static uint16_t read_u16_le(const uint8_t *p)
{
	return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t read_u32_le(const uint8_t *p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static int16_t read_i16_le(const uint8_t *p)
{
	return (int16_t)read_u16_le(p);
}

CHIAKI_EXPORT ChiakiErrorCode chiaki_remote_input_parse(const uint8_t *buf, size_t len, ChiakiRemoteInputPacket *out)
{
	if(!buf || !out || len != CHIAKI_REMOTE_INPUT_PACKET_SIZE)
		return CHIAKI_ERR_INVALID_DATA;
	if(buf[0] != CHIAKI_REMOTE_INPUT_VERSION)
		return CHIAKI_ERR_VERSION_MISMATCH;

	out->flags = buf[1];
	out->seq = read_u16_le(buf + 2);
	out->buttons = read_u32_le(buf + 4);
	out->l2 = buf[8];
	out->r2 = buf[9];
	out->lx = read_i16_le(buf + 10);
	out->ly = read_i16_le(buf + 12);
	out->rx = read_i16_le(buf + 14);
	out->ry = read_i16_le(buf + 16);
	return CHIAKI_ERR_SUCCESS;
}

CHIAKI_EXPORT bool chiaki_remote_input_accept_seq(bool has_last, uint16_t last, uint16_t seq)
{
	if(!has_last)
		return true;
	return chiaki_seq_num_16_gt(seq, last);
}

CHIAKI_EXPORT void chiaki_remote_input_to_state(const ChiakiRemoteInputPacket *pkt, ChiakiControllerState *state)
{
	chiaki_controller_state_set_idle(state);
	uint32_t buttons = pkt->buttons & CHIAKI_REMOTE_INPUT_VALID_BUTTONS;
	if(!(pkt->flags & CHIAKI_REMOTE_INPUT_FLAG_ADMIN))
		buttons &= ~(uint32_t)CHIAKI_REMOTE_INPUT_BLOCKED_BUTTONS;
	state->buttons = buttons;
	state->l2_state = pkt->l2;
	state->r2_state = pkt->r2;
	state->left_x = pkt->lx;
	state->left_y = pkt->ly;
	state->right_x = pkt->rx;
	state->right_y = pkt->ry;
}
