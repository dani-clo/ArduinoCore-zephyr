/*
 * Copyright (c) 2026 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#if defined(CONFIG_BOARD_ARDUINO_UNO_Q) && defined(CONFIG_USBD_GS_USB)

#include <cannectivity/usb/class/gs_usb.h>
#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usbd.h>

LOG_MODULE_REGISTER(uno_q_cannectivity);

struct usbd_context *usbd_init_device(usbd_msg_cb_t msg_cb);

static const struct device *const timestamp_counter =
	DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(cannectivity), timestamp_counter));

static int timestamp_get(const struct device *dev, uint32_t *timestamp, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	return counter_get_value(timestamp_counter, timestamp);
}

static const struct gs_usb_ops can_ops = {
	.timestamp = timestamp_get,
};

static void usb_message(struct usbd_context *const ctx, const struct usbd_msg *msg)
{
	if (usbd_can_detect_vbus(ctx) && msg->type == USBD_MSG_VBUS_READY) {
		int err = usbd_enable(ctx);

		if (err != 0) {
			LOG_ERR("Failed to enable CAN USB interface (%d)", err);
		}
	}
}

static int uno_q_cannectivity_init(void)
{
	const struct device *gs_usb = DEVICE_DT_GET(DT_NODELABEL(gs_usb0));
	const struct device *channels[] = {
		DEVICE_DT_GET(DT_PHANDLE(DT_PATH(cannectivity, channel0), can_controller)),
	};
	struct usbd_context *ctx;
	int err;

	if (!device_is_ready(timestamp_counter) || !device_is_ready(gs_usb) ||
	    !device_is_ready(channels[0])) {
		return -ENODEV;
	}

	if (counter_get_frequency(timestamp_counter) != MHZ(1) ||
	    counter_get_max_top_value(timestamp_counter) != UINT32_MAX) {
		return -EINVAL;
	}

	err = counter_start(timestamp_counter);
	if (err != 0) {
		return err;
	}

	err = gs_usb_register(gs_usb, channels, ARRAY_SIZE(channels), &can_ops, NULL);
	if (err != 0) {
		return err;
	}

	ctx = usbd_init_device(usb_message);
	if (ctx == NULL) {
		return -EIO;
	}

	if (!usbd_can_detect_vbus(ctx)) {
		return usbd_enable(ctx);
	}

	return 0;
}

SYS_INIT(uno_q_cannectivity_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

#endif
