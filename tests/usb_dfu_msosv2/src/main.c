/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include "../../../cores/arduino/usb_dfu_msosv2.h"

USBD_DEVICE_DEFINE(test_usbd, DEVICE_DT_GET(DT_NODELABEL(zephyr_udc0)), 0x2341, 0x007e);
USBD_CONFIGURATION_DEFINE(test_fs_config, 0, 100, NULL);
USBD_CONFIGURATION_DEFINE(test_hs_config, 0, 100, NULL);

static struct usbd_class_data runtime_class = {.name = "dfu_runtime"};
static struct usbd_class_node runtime_fs = {.c_data = &runtime_class, .iface_bm = BIT(2)};
static struct usbd_class_node runtime_hs = {.c_data = &runtime_class, .iface_bm = BIT(4)};

static struct usb_setup_packet request = {
	.bmRequestType = 0xC0,
	.bRequest = ARDUINO_DFU_MSOS_VENDOR_CODE,
	.wIndex = MS_OS_20_DESCRIPTOR_INDEX,
	.wLength = UINT16_MAX,
};

static void *setup(void)
{
	zassert_ok(usbd_add_configuration(&test_usbd, USBD_SPEED_FS, &test_fs_config));
	zassert_ok(usbd_add_configuration(&test_usbd, USBD_SPEED_HS, &test_hs_config));
	sys_slist_append(&test_fs_config.class_list, &runtime_fs.node);
	sys_slist_append(&test_hs_config.class_list, &runtime_hs.node);
	zassert_ok(arduino_dfu_msos_register(&test_usbd, true));
	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	arduino_dfu_msos_composite = true;
	test_usbd.status.speed = USBD_SPEED_FS;
	runtime_fs.iface_bm = BIT(2);
	request.wIndex = MS_OS_20_DESCRIPTOR_INDEX;
	request.wLength = UINT16_MAX;
}

static void check_features(const uint8_t *data)
{
	static const char guid[] = "{B26A9C77-62D3-4DA3-9D91-728C6B7ABF81}";

	zassert_equal(sys_get_le16(data), 20);
	zassert_equal(sys_get_le16(data + 2), MS_OS_20_FEATURE_COMPATIBLE_ID);
	zassert_mem_equal(data + 4, "WINUSB\0\0", 8);
	zassert_mem_equal(data + 12, (uint8_t[8]) {0}, 8);
	data += 20;
	zassert_equal(sys_get_le16(data), 132);
	zassert_equal(sys_get_le16(data + 2), MS_OS_20_FEATURE_REG_PROPERTY);
	zassert_equal(sys_get_le16(data + 4), MS_OS_20_PROPERTY_DATA_REG_MULTI_SZ);
	zassert_equal(sys_get_le16(data + 6), 42);
	for (size_t i = 0; i < sizeof("DeviceInterfaceGUIDs"); i++) {
		zassert_equal(sys_get_le16(data + 8 + 2 * i), "DeviceInterfaceGUIDs"[i]);
	}
	zassert_equal(sys_get_le16(data + 50), 80);
	for (size_t i = 0; i < sizeof(guid); i++) {
		zassert_equal(sys_get_le16(data + 52 + 2 * i), guid[i]);
	}
	zassert_equal(sys_get_le16(data + 130), 0);
}

ZTEST(dfu_msosv2, test_registration)
{
	const struct usb_device_descriptor *fs = test_usbd.fs_desc;
	const struct usb_device_descriptor *hs = test_usbd.hs_desc;

	zassert_equal(sys_le16_to_cpu(fs->bcdUSB), 0x0201);
	zassert_equal(sys_le16_to_cpu(hs->bcdUSB), 0x0201);
	zassert_equal(arduino_dfu_msos_bos.platform.bLength, 28);
	zassert_equal(sys_le16_to_cpu(arduino_dfu_msos_bos.cap.wMSOSDescriptorSetTotalLength),
		      170);
	zassert_equal(sys_le32_to_cpu(arduino_dfu_msos_bos.cap.dwWindowsVersion), 0x06030000);
	zassert_equal(arduino_dfu_msos_bos.cap.bAltEnumCode, 0);
}

ZTEST(dfu_msosv2, test_composite_descriptor)
{
	for (enum usbd_speed speed = USBD_SPEED_FS; speed <= USBD_SPEED_HS; speed++) {
		struct net_buf *buf;

		test_usbd.status.speed = speed;
		buf = arduino_dfu_msos_to_host(&test_usbd, &request);
		zassert_not_null(buf);
		zassert_equal(buf->len, 170);
		zassert_equal(sys_get_le16(buf->data), 10);
		zassert_equal(sys_get_le16(buf->data + 2), MS_OS_20_SET_HEADER_DESCRIPTOR);
		zassert_equal(sys_get_le32(buf->data + 4), 0x06030000);
		zassert_equal(sys_get_le16(buf->data + 8), 170);
		zassert_equal(sys_get_le16(buf->data + 10), 8);
		zassert_equal(sys_get_le16(buf->data + 12), MS_OS_20_SUBSET_HEADER_FUNCTION);
		zassert_equal(buf->data[14], speed == USBD_SPEED_FS ? 2 : 4);
		zassert_equal(buf->data[15], 0);
		zassert_equal(sys_get_le16(buf->data + 16), 160);
		check_features(buf->data + 18);
		net_buf_unref(buf);
	}
}

ZTEST(dfu_msosv2, test_standalone_descriptor)
{
	struct net_buf *buf;

	arduino_dfu_msos_composite = false;
	buf = arduino_dfu_msos_to_host(&test_usbd, &request);
	zassert_not_null(buf);
	zassert_equal(buf->len, 162);
	zassert_equal(sys_get_le16(buf->data + 8), 162);
	check_features(buf->data + 10);
	net_buf_unref(buf);
}

ZTEST(dfu_msosv2, test_short_requests)
{
	for (uint16_t len = 0; len <= 170; len++) {
		struct net_buf *buf;

		request.wLength = len;
		buf = arduino_dfu_msos_to_host(&test_usbd, &request);
		zassert_not_null(buf);
		zassert_equal(buf->len, len);
		net_buf_unref(buf);
	}
}

ZTEST(dfu_msosv2, test_invalid_requests)
{
	request.wIndex = 8;
	zassert_is_null(arduino_dfu_msos_to_host(&test_usbd, &request));
}

ZTEST(dfu_msosv2, test_missing_dfu_interface)
{
	runtime_fs.iface_bm = 0;
	zassert_is_null(arduino_dfu_msos_to_host(&test_usbd, &request));
}

ZTEST_SUITE(dfu_msosv2, NULL, setup, before, NULL, NULL);
