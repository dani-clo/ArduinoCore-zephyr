/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include "usbd_dfu.h"
#include "../../cores/arduino/usb_dfu_msosv2.h"

/* The statically allocated context is available before MCUboot starts USB. */
static int boot_dfu_msosv2_init(void)
{
	return arduino_dfu_msos_register(boot_usb_dfu_get_context(), false);
}

SYS_INIT(boot_dfu_msosv2_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
