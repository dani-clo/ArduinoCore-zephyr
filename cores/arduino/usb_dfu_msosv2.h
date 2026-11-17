/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ARDUINO_USB_DFU_MSOSV2_H_
#define ARDUINO_USB_DFU_MSOSV2_H_

#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/bos.h>
#include <zephyr/usb/msos_desc.h>
#include <zephyr/usb/usbd.h>

/*
 * Header-only on purpose: a descriptor node can be linked to a single USB context
 * only, and the loader uses two (DFU mode and sketch). Each including source file
 * gets its own nodes and state.
 */
#define ARDUINO_DFU_MSOS_VENDOR_CODE 0x02U
#define ARDUINO_DFU_MSOS_WINDOWS_VERSION 0x06030000UL

/* ASCII character as a UTF-16LE code unit. */
#define MSOS_U16(c) (c), 0

struct arduino_dfu_msos_features {
	struct msosv2_compatible_id compatible_id;
	struct msosv2_guids_property guids;
} __packed;

static const struct arduino_dfu_msos_features arduino_dfu_msos_features = {
	.compatible_id = {
		.wLength = sys_cpu_to_le16(sizeof(struct msosv2_compatible_id)),
		.wDescriptorType = sys_cpu_to_le16(MS_OS_20_FEATURE_COMPATIBLE_ID),
		.CompatibleID = {'W', 'I', 'N', 'U', 'S', 'B', 0, 0},
	},
	.guids = {
		.wLength = sys_cpu_to_le16(sizeof(struct msosv2_guids_property)),
		.wDescriptorType = sys_cpu_to_le16(MS_OS_20_FEATURE_REG_PROPERTY),
		.wPropertyDataType = sys_cpu_to_le16(MS_OS_20_PROPERTY_DATA_REG_MULTI_SZ),
		.wPropertyNameLength = sys_cpu_to_le16(SIZEOF_FIELD(struct msosv2_guids_property, PropertyName)),
		.PropertyName = {DEVICE_INTERFACE_GUIDS_PROPERTY_NAME},
		.wPropertyDataLength = sys_cpu_to_le16(SIZEOF_FIELD(struct msosv2_guids_property, bPropertyData)),
		/* Stable DFU interface GUID: {B26A9C77-62D3-4DA3-9D91-728C6B7ABF81}. */
		.bPropertyData = {
			MSOS_U16('{'), MSOS_U16('B'), MSOS_U16('2'), MSOS_U16('6'), MSOS_U16('A'),
			MSOS_U16('9'), MSOS_U16('C'), MSOS_U16('7'), MSOS_U16('7'), MSOS_U16('-'),
			MSOS_U16('6'), MSOS_U16('2'), MSOS_U16('D'), MSOS_U16('3'), MSOS_U16('-'),
			MSOS_U16('4'), MSOS_U16('D'), MSOS_U16('A'), MSOS_U16('3'), MSOS_U16('-'),
			MSOS_U16('9'), MSOS_U16('D'), MSOS_U16('9'), MSOS_U16('1'), MSOS_U16('-'),
			MSOS_U16('7'), MSOS_U16('2'), MSOS_U16('8'), MSOS_U16('C'), MSOS_U16('6'),
			MSOS_U16('B'), MSOS_U16('7'), MSOS_U16('A'), MSOS_U16('B'), MSOS_U16('F'),
			MSOS_U16('8'), MSOS_U16('1'), MSOS_U16('}'),
			/* String terminator, then list terminator (REG_MULTI_SZ). */
			0, 0, 0, 0,
		},
	},
};

static bool arduino_dfu_msos_composite;

static size_t arduino_dfu_msos_length(void)
{
	return sizeof(struct msosv2_descriptor_set_header) + sizeof(arduino_dfu_msos_features) +
	       (arduino_dfu_msos_composite ? sizeof(struct msosv2_function_subset_header) : 0U);
}

/* Interface numbers are assigned by usbd_init(), independently per speed. */
static int arduino_dfu_msos_dfu_interface(const struct usbd_context *const ctx)
{
	const sys_slist_t *configs = usbd_bus_speed(ctx) == USBD_SPEED_HS ?
		&ctx->hs_configs : &ctx->fs_configs;
	struct usbd_config_node *cfg;
	struct usbd_class_node *cls;

	SYS_SLIST_FOR_EACH_CONTAINER(configs, cfg, node) {
		SYS_SLIST_FOR_EACH_CONTAINER(&cfg->class_list, cls, node) {
			if (strcmp(cls->c_data->name, "dfu_runtime") == 0 && cls->iface_bm != 0U) {
				return find_lsb_set(cls->iface_bm) - 1;
			}
		}
	}

	return -ENODEV;
}

/* Zephyr already dispatches by vendor code and direction; only the index is checked. */
static struct net_buf *arduino_dfu_msos_to_host(const struct usbd_context *const ctx,
					     const struct usb_setup_packet *const setup)
{
	uint8_t data[sizeof(struct msosv2_descriptor_set_header) +
		     sizeof(struct msosv2_function_subset_header) +
		     sizeof(struct arduino_dfu_msos_features)];
	const size_t total = arduino_dfu_msos_length();
	const struct msosv2_descriptor_set_header header = {
		.wLength = sys_cpu_to_le16(sizeof(header)),
		.wDescriptorType = sys_cpu_to_le16(MS_OS_20_SET_HEADER_DESCRIPTOR),
		.dwWindowsVersion = sys_cpu_to_le32(ARDUINO_DFU_MSOS_WINDOWS_VERSION),
		.wTotalLength = sys_cpu_to_le16(total),
	};
	size_t offset = sizeof(header);
	struct net_buf *buf;

	if (setup->wIndex != MS_OS_20_DESCRIPTOR_INDEX) {
		return NULL;
	}

	memcpy(data, &header, sizeof(header));
	if (arduino_dfu_msos_composite) {
		const int interface = arduino_dfu_msos_dfu_interface(ctx);

		if (interface < 0) {
			return NULL;
		}

		const struct msosv2_function_subset_header function = {
			.wLength = sys_cpu_to_le16(sizeof(function)),
			.wDescriptorType = sys_cpu_to_le16(MS_OS_20_SUBSET_HEADER_FUNCTION),
			.bFirstInterface = interface,
			.wSubsetLength = sys_cpu_to_le16(sizeof(function) +
							 sizeof(arduino_dfu_msos_features)),
		};

		memcpy(data + offset, &function, sizeof(function));
		offset += sizeof(function);
	}
	memcpy(data + offset, &arduino_dfu_msos_features, sizeof(arduino_dfu_msos_features));

	const size_t len = MIN(setup->wLength, total);

	buf = usbd_ep_ctrl_data_in_alloc(ctx, len);
	if (buf != NULL) {
		net_buf_add_mem(buf, data, len);
	}
	return buf;
}

static struct {
	struct usb_bos_platform_descriptor platform;
	struct usb_bos_capability_msos cap;
} __packed arduino_dfu_msos_bos = {
	.platform = {
		.bLength = sizeof(struct usb_bos_platform_descriptor) +
			   sizeof(struct usb_bos_capability_msos),
		.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
		.bDevCapabilityType = USB_BOS_CAPABILITY_PLATFORM,
		.PlatformCapabilityUUID = {
			0xDF, 0x60, 0xDD, 0xD8, 0x89, 0x45, 0xC7, 0x4C,
			0x9C, 0xD2, 0x65, 0x9D, 0x9E, 0x64, 0x8A, 0x9F,
		},
	},
	.cap = {
		.dwWindowsVersion = sys_cpu_to_le32(ARDUINO_DFU_MSOS_WINDOWS_VERSION),
		.bMS_VendorCode = ARDUINO_DFU_MSOS_VENDOR_CODE,
	},
};

USBD_DESC_BOS_VREQ_DEFINE(arduino_dfu_msos_bos_node, sizeof(arduino_dfu_msos_bos),
			 &arduino_dfu_msos_bos, ARDUINO_DFU_MSOS_VENDOR_CODE,
			 arduino_dfu_msos_to_host, NULL);

static const struct usb_bos_capability_lpm arduino_dfu_usbext = {
	.bLength = sizeof(struct usb_bos_capability_lpm),
	.bDescriptorType = USB_DESC_DEVICE_CAPABILITY,
	.bDevCapabilityType = USB_BOS_CAPABILITY_EXTENSION,
};

USBD_DESC_BOS_DEFINE(arduino_dfu_usbext_node, sizeof(arduino_dfu_usbext), &arduino_dfu_usbext);

/* One configuration per speed; a function subset is needed only for composite runtime. */
static int arduino_dfu_msos_register(struct usbd_context *ctx, bool composite)
{
	int err;

	arduino_dfu_msos_composite = composite;
	arduino_dfu_msos_bos.cap.wMSOSDescriptorSetTotalLength =
		sys_cpu_to_le16(arduino_dfu_msos_length());
	err = usbd_device_set_bcd_usb(ctx, USBD_SPEED_FS, 0x0201);
	if (err != 0) {
		return err;
	}
	if (usbd_caps_speed(ctx) == USBD_SPEED_HS) {
		err = usbd_device_set_bcd_usb(ctx, USBD_SPEED_HS, 0x0201);
		if (err != 0) {
			return err;
		}
	}
	err = usbd_add_descriptor(ctx, &arduino_dfu_usbext_node);
	if (err != 0) {
		return err;
	}
	return usbd_add_descriptor(ctx, &arduino_dfu_msos_bos_node);
}

#endif /* ARDUINO_USB_DFU_MSOSV2_H_ */
