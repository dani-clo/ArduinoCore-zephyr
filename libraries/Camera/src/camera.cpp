/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * Camera driver.
 */
#include "Arduino.h"
#include "camera.h"

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree/port-endpoint.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyrClockInit.hpp>
#include <zephyrPinctrl.h>

#if !DT_HAS_CHOSEN(zephyr_camera)
#error "zephyr,camera node is not defined in devicetree"
#endif

#define CLOCK_NODE DT_CHOSEN(arduino_camera_clock)
static const struct pwm_dt_spec CLOCK_PWM = PWM_DT_SPEC_GET(CLOCK_NODE);

#define CAMERA_NODE          DT_CHOSEN(zephyr_camera)
#define CAMERA_PORT_NODE     DT_CHILD(CAMERA_NODE, port)
#define CAMERA_ENDPOINT_NODE DT_CHILD(CAMERA_PORT_NODE, endpoint)
#define CAMERA_SENSOR_NODE   DT_NODE_REMOTE_DEVICE(CAMERA_ENDPOINT_NODE)

#ifdef CONFIG_USER_MANAGES_VIDEO_BUFFER_POOL
K_HEAP_DEFINE(video_buffer_pool,
		CONFIG_VIDEO_BUFFER_POOL_SZ_MAX * 2);
#define VIDEO_COMMON_HEAP_ALLOC(align, size, timeout)                                              \
	k_heap_aligned_alloc(&video_buffer_pool, align, size, timeout);
#define VIDEO_COMMON_FREE(block) k_heap_free(&video_buffer_pool, block)

struct mem_block {
	void *data;
};

static struct video_buffer video_buf[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];
static struct mem_block video_block[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];


struct user_managed_video_buffer {
	struct video_buffer vbuf;
	void *data;
};

static struct user_managed_video_buffer user_video_buffers[CONFIG_VIDEO_BUFFER_POOL_NUM_MAX];

struct video_buffer *user_video_buffer_aligned_alloc(size_t size, size_t align,
							 k_timeout_t timeout)
{
	struct video_buffer *vbuf = NULL;
	struct mem_block *block;
	int i;

	/* find available video buffer */
	for (i = 0; i < ARRAY_SIZE(video_buf); i++) {
		if (video_buf[i].buffer == NULL) {
			vbuf = &video_buf[i];
			block = &video_block[i];
			break;
		}
	}

	if (vbuf == NULL) {
		printk("No available video buffer\n");
		return NULL;
	}

	/* Alloc buffer memory */
	block->data = VIDEO_COMMON_HEAP_ALLOC(align, size, timeout);
	if (block->data == NULL) {
		printk("Failed to allocate memory for video buffer\n");
		return NULL;
	}

	vbuf->buffer = static_cast<uint8_t *>(block->data);
	vbuf->size = size;
	vbuf->bytesused = 0;

	printk("Allocated video buffer: %p, size: %zu, align: %zu\n", vbuf->buffer, size, align);
	
	return vbuf;
}

void user_video_buffer_release(struct video_buffer *vbuf)
{
	struct mem_block *block = NULL;
	int i;

	if (vbuf == NULL) {
		printk("Attempted to release a NULL video buffer\n");
		return;
	}

	/* vbuf to block */
	for (i = 0; i < ARRAY_SIZE(video_block); i++) {
		if (video_block[i].data == vbuf->buffer) {
			block = &video_block[i];
			break;
		}
	}

	vbuf->buffer = NULL;
	if (block) {
		VIDEO_COMMON_FREE(block->data);
	}
}
#endif

FrameBuffer::FrameBuffer() : vbuf(NULL) {
}

uint32_t FrameBuffer::getBufferSize() {
	if (this->vbuf) {
		return this->vbuf->bytesused;
	}

	return 0;
}

uint8_t *FrameBuffer::getBuffer() {
	if (this->vbuf) {
		return this->vbuf->buffer;
	}

	return NULL;
}

Camera::Camera() : byte_swap(false), yuv_to_gray(false), vdev(NULL) {
	for (size_t i = 0; i < ARRAY_SIZE(this->vbuf); i++) {
		this->vbuf[i] = NULL;
	}
}

bool Camera::begin(uint32_t width, uint32_t height, uint32_t pixformat, bool byte_swap) {
	if (zephyr::arduino::init_pwm_ref_clock(DEVICE_DT_GET(CLOCK_NODE), CLOCK_PWM) != 0) {
		return false;
	}

	// init camera sensor first
	const struct device *sensor = DEVICE_DT_GET(CAMERA_SENSOR_NODE);
	if (zephyr::arduino::init_dev_apply_pinctrl(sensor) < 0) {
		return false;
	}

	this->vdev = DEVICE_DT_GET(CAMERA_NODE);

	if (zephyr::arduino::init_dev_apply_pinctrl(this->vdev) < 0) {
		return false;
	}

	switch (pixformat) {
	case CAMERA_RGB565:
		this->byte_swap = byte_swap;
		pixformat = VIDEO_PIX_FMT_RGB565;
		break;
	case CAMERA_GRAYSCALE:
		// There's no support for mono sensors.
		this->yuv_to_gray = true;
		pixformat = VIDEO_PIX_FMT_YUYV;
		break;
	default:
		break;
	}

	// Get capabilities
	struct video_caps caps;
	if (video_get_caps(this->vdev, &caps)) {
		return false;
	}

	for (size_t i = 0; caps.format_caps[i].pixelformat != 0; i++) {
		const struct video_format_cap *fcap = &caps.format_caps[i];
		if (fcap->width_min == width && fcap->height_min == height &&
			fcap->pixelformat == pixformat) {
			break;
		}
		if (caps.format_caps[i + 1].pixelformat == 0) {
			Serial.println("The specified format is not supported");
			return false;
		}
	}

	// Set format.
	static struct video_format fmt = {
		.pixelformat = pixformat,
		.width = width,
		.height = height,
		.pitch = width * 2,
	};

	if (video_set_format(this->vdev, &fmt)) {
		Serial.println("Failed to set video format");
		return false;
	}

	// Allocate video buffers.
	for (size_t i = 0; i < ARRAY_SIZE(this->vbuf); i++) {
		this->vbuf[i] = video_buffer_aligned_alloc(fmt.pitch * fmt.height,
												   CONFIG_VIDEO_BUFFER_POOL_ALIGN, K_MSEC(100));
		if (this->vbuf[i] == NULL) {
			Serial.println("Failed to allocate video buffers");
			return false;
		}
		video_enqueue(this->vdev, this->vbuf[i]);
	}

	// Start video capture
	if (video_stream_start(this->vdev, VIDEO_BUF_TYPE_OUTPUT)) {
		Serial.println("Failed to start capture");
		return false;
	}

	return true;
}

bool Camera::grabFrame(FrameBuffer &fb, uint32_t timeout) {
	if (this->vdev == NULL) {
		return false;
	}

	if (video_dequeue(this->vdev, &fb.vbuf, K_MSEC(timeout))) {
		return false;
	}

	if (this->byte_swap) {
		uint16_t *pixels = (uint16_t *)fb.vbuf->buffer;
		for (size_t i = 0; i < fb.vbuf->bytesused / 2; i++) {
			pixels[i] = __REVSH(pixels[i]);
		}
	}

	if (this->yuv_to_gray) {
		uint8_t *pixels = (uint8_t *)fb.vbuf->buffer;
		for (size_t i = 0; i < fb.vbuf->bytesused / 2; i++) {
			pixels[i] = pixels[i * 2];
		}
		fb.vbuf->bytesused /= 2;
	}

	return true;
}

bool Camera::releaseFrame(FrameBuffer &fb) {
	if (this->vdev == NULL) {
		return false;
	}

	if (video_enqueue(this->vdev, fb.vbuf)) {
		return false;
	}

	return true;
}

bool Camera::setVerticalFlip(bool flip_enable) {
	struct video_control ctrl = {.id = VIDEO_CID_VFLIP, .val = flip_enable};
	return video_set_ctrl(this->vdev, &ctrl) == 0;
}

bool Camera::setHorizontalMirror(bool mirror_enable) {
	struct video_control ctrl = {.id = VIDEO_CID_HFLIP, .val = mirror_enable};
	return video_set_ctrl(this->vdev, &ctrl) == 0;
}
