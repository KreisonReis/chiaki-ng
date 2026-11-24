// SPDX-License-Identifier: LicenseRef-AGPL-3.0-only-OpenSSL

#include <munit.h>

#include <string.h>

#include <chiaki/remoteinput.h>

// Reference vector shared with fila-gta/tools/test_remote_input.py
static const uint8_t golden[CHIAKI_REMOTE_INPUT_PACKET_SIZE] = {
	0x01, 0x00,             // version, flags
	0x02, 0x01,             // seq = 0x0102
	0x01, 0x02, 0x00, 0x00, // buttons = CROSS | R1
	0x00, 0xFF,             // l2, r2
	0x00, 0x00,             // lx = 0
	0x00, 0x80,             // ly = -32768
	0xB0, 0x04,             // rx = 1200
	0xFF, 0xFF              // ry = -1
};

static MunitResult test_parse_golden(const MunitParameter params[], void *user)
{
	ChiakiRemoteInputPacket pkt;
	munit_assert_int(chiaki_remote_input_parse(golden, sizeof(golden), &pkt), ==, CHIAKI_ERR_SUCCESS);
	munit_assert_uint8(pkt.flags, ==, 0);
	munit_assert_uint16(pkt.seq, ==, 0x0102);
	munit_assert_uint32(pkt.buttons, ==, CHIAKI_CONTROLLER_BUTTON_CROSS | CHIAKI_CONTROLLER_BUTTON_R1);
	munit_assert_uint32(pkt.buttons, ==, 0x201);
	munit_assert_uint8(pkt.l2, ==, 0);
	munit_assert_uint8(pkt.r2, ==, 255);
	munit_assert_int16(pkt.lx, ==, 0);
	munit_assert_int16(pkt.ly, ==, -32768);
	munit_assert_int16(pkt.rx, ==, 1200);
	munit_assert_int16(pkt.ry, ==, -1);
	return MUNIT_OK;
}

static MunitResult test_parse_wrong_length(const MunitParameter params[], void *user)
{
	uint8_t buf[CHIAKI_REMOTE_INPUT_PACKET_SIZE + 1];
	memcpy(buf, golden, sizeof(golden));
	buf[CHIAKI_REMOTE_INPUT_PACKET_SIZE] = 0;
	ChiakiRemoteInputPacket pkt;
	munit_assert_int(chiaki_remote_input_parse(buf, CHIAKI_REMOTE_INPUT_PACKET_SIZE - 1, &pkt), ==, CHIAKI_ERR_INVALID_DATA);
	munit_assert_int(chiaki_remote_input_parse(buf, CHIAKI_REMOTE_INPUT_PACKET_SIZE + 1, &pkt), ==, CHIAKI_ERR_INVALID_DATA);
	return MUNIT_OK;
}

static MunitResult test_parse_wrong_version(const MunitParameter params[], void *user)
{
	uint8_t buf[CHIAKI_REMOTE_INPUT_PACKET_SIZE];
	memcpy(buf, golden, sizeof(golden));
	buf[0] = 0x02;
	ChiakiRemoteInputPacket pkt;
	munit_assert_int(chiaki_remote_input_parse(buf, sizeof(buf), &pkt), ==, CHIAKI_ERR_VERSION_MISMATCH);
	return MUNIT_OK;
}

static ChiakiRemoteInputPacket packet_with_buttons(uint32_t buttons, uint8_t flags)
{
	ChiakiRemoteInputPacket pkt;
	memset(&pkt, 0, sizeof(pkt));
	pkt.buttons = buttons;
	pkt.flags = flags;
	return pkt;
}

static MunitResult test_to_state_masks_blocked(const MunitParameter params[], void *user)
{
	ChiakiRemoteInputPacket pkt = packet_with_buttons(
		CHIAKI_CONTROLLER_BUTTON_PS | CHIAKI_CONTROLLER_BUTTON_OPTIONS | CHIAKI_CONTROLLER_BUTTON_SHARE | CHIAKI_CONTROLLER_BUTTON_CROSS, 0);
	ChiakiControllerState state;
	chiaki_remote_input_to_state(&pkt, &state);
	munit_assert_uint32(state.buttons, ==, CHIAKI_CONTROLLER_BUTTON_CROSS);
	return MUNIT_OK;
}

static MunitResult test_to_state_admin_keeps_all(const MunitParameter params[], void *user)
{
	uint32_t all = CHIAKI_CONTROLLER_BUTTON_PS | CHIAKI_CONTROLLER_BUTTON_OPTIONS | CHIAKI_CONTROLLER_BUTTON_SHARE | CHIAKI_CONTROLLER_BUTTON_CROSS;
	ChiakiRemoteInputPacket pkt = packet_with_buttons(all, CHIAKI_REMOTE_INPUT_FLAG_ADMIN);
	ChiakiControllerState state;
	chiaki_remote_input_to_state(&pkt, &state);
	munit_assert_uint32(state.buttons, ==, all);
	return MUNIT_OK;
}

static MunitResult test_to_state_drops_high_bits(const MunitParameter params[], void *user)
{
	ChiakiRemoteInputPacket pkt = packet_with_buttons(0xFFFF0000u | CHIAKI_CONTROLLER_BUTTON_CROSS, 0);
	ChiakiControllerState state;
	chiaki_remote_input_to_state(&pkt, &state);
	munit_assert_uint32(state.buttons, ==, CHIAKI_CONTROLLER_BUTTON_CROSS);
	return MUNIT_OK;
}

static MunitResult test_to_state_copies_axes(const MunitParameter params[], void *user)
{
	ChiakiRemoteInputPacket pkt;
	munit_assert_int(chiaki_remote_input_parse(golden, sizeof(golden), &pkt), ==, CHIAKI_ERR_SUCCESS);
	ChiakiControllerState state;
	memset(&state, 0x5A, sizeof(state)); // garbage that must be overwritten
	chiaki_remote_input_to_state(&pkt, &state);
	munit_assert_uint8(state.l2_state, ==, 0);
	munit_assert_uint8(state.r2_state, ==, 255);
	munit_assert_int16(state.left_x, ==, 0);
	munit_assert_int16(state.left_y, ==, -32768);
	munit_assert_int16(state.right_x, ==, 1200);
	munit_assert_int16(state.right_y, ==, -1);

	ChiakiControllerState idle;
	chiaki_controller_state_set_idle(&idle);
	munit_assert_uint8(state.touch_id_next, ==, idle.touch_id_next);
	for(size_t i = 0; i < CHIAKI_CONTROLLER_TOUCHES_MAX; i++)
	{
		munit_assert_int8(state.touches[i].id, ==, idle.touches[i].id);
		munit_assert_uint16(state.touches[i].x, ==, idle.touches[i].x);
		munit_assert_uint16(state.touches[i].y, ==, idle.touches[i].y);
	}
	return MUNIT_OK;
}

static MunitResult test_accept_seq_no_last(const MunitParameter params[], void *user)
{
	munit_assert_true(chiaki_remote_input_accept_seq(false, 500, 0));
	return MUNIT_OK;
}

static MunitResult test_accept_seq_newer(const MunitParameter params[], void *user)
{
	munit_assert_true(chiaki_remote_input_accept_seq(true, 10, 11));
	return MUNIT_OK;
}

static MunitResult test_accept_seq_equal_or_older(const MunitParameter params[], void *user)
{
	munit_assert_false(chiaki_remote_input_accept_seq(true, 10, 10));
	munit_assert_false(chiaki_remote_input_accept_seq(true, 10, 5));
	return MUNIT_OK;
}

static MunitResult test_accept_seq_wraparound(const MunitParameter params[], void *user)
{
	munit_assert_true(chiaki_remote_input_accept_seq(true, 65535, 0));
	return MUNIT_OK;
}

#define REMOTE_INPUT_TEST(name, fn) { name, fn, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }

MunitTest tests_remote_input[] = {
	REMOTE_INPUT_TEST("/parse_golden", test_parse_golden),
	REMOTE_INPUT_TEST("/parse_wrong_length", test_parse_wrong_length),
	REMOTE_INPUT_TEST("/parse_wrong_version", test_parse_wrong_version),
	REMOTE_INPUT_TEST("/to_state_masks_blocked", test_to_state_masks_blocked),
	REMOTE_INPUT_TEST("/to_state_admin_keeps_all", test_to_state_admin_keeps_all),
	REMOTE_INPUT_TEST("/to_state_drops_high_bits", test_to_state_drops_high_bits),
	REMOTE_INPUT_TEST("/to_state_copies_axes", test_to_state_copies_axes),
	REMOTE_INPUT_TEST("/accept_seq_no_last", test_accept_seq_no_last),
	REMOTE_INPUT_TEST("/accept_seq_newer", test_accept_seq_newer),
	REMOTE_INPUT_TEST("/accept_seq_equal_or_older", test_accept_seq_equal_or_older),
	REMOTE_INPUT_TEST("/accept_seq_wraparound", test_accept_seq_wraparound),
	{ NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};
