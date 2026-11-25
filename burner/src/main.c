/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 * SPDX-License-Identifier: Apache-2.0
 *
 * Bootloader updater: uploaded in place of the loader, it rewrites the boot
 * partition with the embedded bootloader if it differs from the flash content.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/storage/flash_map.h>
#ifdef CONFIG_CPU_CORTEX_M_HAS_DWT
#include <cmsis_core.h>
#endif
#ifdef CONFIG_RETENTION_BOOT_MODE
#include <zephyr/retention/bootmode.h>
#endif
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CK_SG_GC)
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/flash/mchp_nvmctrl_g3.h>
#include <soc.h>
#endif
#ifdef CONFIG_BURNER_USB_TOUCH
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/usb/usbd.h>
#endif

static const uint8_t bootloader[] = {
#include "bootloader.bin.inc"
};

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

#if defined(CONFIG_SOC_FLASH_STM32) && defined(CONFIG_SOC_SERIES_STM32H7X)
/* Clear stale error flags left by the bootloader, as the loader does */
static int internal_flash_clean_regs(void) {
	FLASH->CCR1 = FLASH_FLAG_ALL_BANK1;
#ifdef DUAL_BANK
	FLASH->CCR2 = FLASH_FLAG_ALL_BANK2;
#endif
	return 0;
}

SYS_INIT(internal_flash_clean_regs, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
#endif

static bool bootloader_matches(const struct flash_area *fa) {
	static uint8_t buf[256];

	for (size_t off = 0; off < sizeof(bootloader); off += sizeof(buf)) {
		size_t len = MIN(sizeof(buf), sizeof(bootloader) - off);

		if (flash_area_read(fa, off, buf, len) != 0 || memcmp(buf, &bootloader[off], len) != 0) {
			return false;
		}
	}
	return true;
}

#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CK_SG_GC)
/*
 * PIC32CK: the bootloader lives in the Boot Flash Memory, write protected
 * region by region by the FCW. Lift the protection of both BFM banks while
 * rewriting it, and restore it afterwards, also on failure.
 */
static int bfm_write_protect(const struct device *dev, uint32_t lbwp, uint32_t ubwp, bool enable) {
	uint32_t pages[] = {lbwp & FCW_LBWP_LBWP_Msk, ubwp & FCW_UBWP_UBWP_Msk};
	int rc = 0;

	for (size_t bank = 0; bank < ARRAY_SIZE(pages); bank++) {
		struct bfm_exop wp = {
			.boot_bank = bank == 0 ? BOOT_FLASH_BANK_1 : BOOT_FLASH_BANK_2,
			.wp_page = enable ? pages[bank] : FCW_LBWP_LBWP_Msk,
		};
		int err = flash_ex_op(dev,
							  enable ? FLASH_EX_OP_BFM_WRITE_PROTECT_ENABLE :
									   FLASH_EX_OP_BFM_WRITE_PROTECT_DISABLE,
							  (uintptr_t)&wp, NULL);

		if (rc == 0) {
			rc = err;
		}
	}
	return rc;
}

/* restore the saved protection; returns rc, or else the restore error */
static int bfm_restore_write_protect(const struct device *dev, uint32_t lbwp, uint32_t ubwp,
									 int rc) {
	int err = bfm_write_protect(dev, lbwp, ubwp, true);

	if (err == 0 && (FCW_REGS->FCW_LBWP != lbwp || FCW_REGS->FCW_UBWP != ubwp)) {
		err = -EIO;
	}
	if (err) {
		printk("Failed to restore the boot flash write protection: %d\n", err);
	}
	return rc ? rc : err;
}
#endif

static int rewrite_bootloader(const struct flash_area *fa) {
	int rc;

#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CK_SG_GC)
	const uint32_t lbwp = FCW_REGS->FCW_LBWP;
	const uint32_t ubwp = FCW_REGS->FCW_UBWP;

	/* a locked protection cannot be lifted until reset: check before erasing */
	if ((lbwp & FCW_LBWP_LBWPLOCK_Msk) != 0 || (ubwp & FCW_UBWP_UBWPLOCK_Msk) != 0) {
		printk("Boot flash write protection locked\n");
		return -EACCES;
	}
	rc = bfm_write_protect(fa->fa_dev, lbwp, ubwp, false);
	if (rc == 0 && ((FCW_REGS->FCW_LBWP & FCW_LBWP_LBWP_Msk) != 0 ||
					(FCW_REGS->FCW_UBWP & FCW_UBWP_UBWP_Msk) != 0)) {
		rc = -EACCES;
	}
	if (rc) {
		printk("Failed to lift the boot flash write protection: %d\n", rc);
		goto out;
	}
#endif

	rc = flash_area_erase(fa, 0, fa->fa_size);
	if (rc) {
		printk("Erase failed: %d\n", rc);
		goto out;
	}

#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CK_SG_GC)
	/* row programming reads its source through the SRAM bus, not from flash */
	static uint8_t row[DT_PROP(DT_NODELABEL(flash0), write_block_size_row)] __aligned(4);

	for (size_t off = 0; off < sizeof(bootloader); off += sizeof(row)) {
		size_t len = MIN(sizeof(row), sizeof(bootloader) - off);

		memcpy(row, &bootloader[off], len);
		rc = flash_area_write(fa, off, row, len);
		if (rc) {
			break;
		}
	}
#else
	rc = flash_area_write(fa, 0, bootloader, sizeof(bootloader));
#endif
	if (rc) {
		printk("Write failed: %d\n", rc);
	}

out:
#if defined(CONFIG_SOC_FAMILY_MICROCHIP_PIC32CK_SG_GC)
	rc = bfm_restore_write_protect(fa->fa_dev, lbwp, ubwp, rc);
#endif
	return rc;
}

static int update_bootloader(void) {
	const struct flash_area *fa;
	int rc;

	rc = flash_area_open(PARTITION_ID(boot_partition), &fa);
	if (rc) {
		printk("Failed to open boot partition: %d\n", rc);
		return rc;
	}

	if (sizeof(bootloader) > fa->fa_size) {
		printk("Bootloader (%u bytes) larger than boot partition (%u bytes)\n",
			   (unsigned int)sizeof(bootloader), (unsigned int)fa->fa_size);
		rc = -EFBIG;
		goto out;
	}

	if (bootloader_matches(fa)) {
		printk("Bootloader already up to date\n");
		goto out;
	}

	printk("Updating bootloader (%u bytes)...\n", (unsigned int)sizeof(bootloader));
	rc = rewrite_bootloader(fa);
	if (rc) {
		goto out;
	}
	if (!bootloader_matches(fa)) {
		printk("Verify failed\n");
		rc = -EIO;
		goto out;
	}
	printk("Bootloader updated\n");

out:
	flash_area_close(fa);
	return rc;
}

#ifdef CONFIG_BURNER_USB_TOUCH
/* the CDC ACM port brought up at boot, see CDC_ACM_SERIAL_INITIALIZE_AT_BOOT */
static const struct device *const usb_uart = DEVICE_DT_GET(DT_INST(0, zephyr_cdc_acm_uart));

/* a 1200 bps touch resets the board, which then enters bootloader mode */
static void check_1200_bps_touch(void) {
	uint32_t baudrate = 0;

	if (uart_line_ctrl_get(usb_uart, UART_LINE_CTRL_BAUD_RATE, &baudrate) == 0 &&
		baudrate == 1200) {
		k_sleep(K_MSEC(100));
		/* detach from the host first, as the loader does; the USB device
		 * belongs to the CDC ACM serial backend, so walk the contexts
		 */
		STRUCT_SECTION_FOREACH (usbd_context, ctx) {
			(void)usbd_disable(ctx);
		}
		sys_reboot(SYS_REBOOT_COLD);
	}
}
#endif

#ifdef CONFIG_CPU_CORTEX_M_HAS_DWT
/*
 * DWT watchpoints survive a system reset. The loader's null pointer detection
 * watches 0x0-0x3ff, where some boards keep their bootloader: when the burner
 * reads it, a matching watchpoint halts the core if a debugger enabled halting
 * debug since power-up. Disable them before reading the bootloader.
 */
static void dwt_disable_watchpoints(void) {
	volatile uint32_t *function = &DWT->FUNCTION0;
	uint32_t num = (DWT->CTRL & DWT_CTRL_NUMCOMP_Msk) >> DWT_CTRL_NUMCOMP_Pos;

	/* COMPn, MASKn, FUNCTIONn, reserved: 4 words per comparator */
	for (uint32_t i = 0; i < num; i++) {
		function[i * 4] = 0;
	}
}
#endif

int main(void) {
#ifdef CONFIG_CPU_CORTEX_M_HAS_DWT
	dwt_disable_watchpoints();
#endif
	int rc = update_bootloader();

#ifdef CONFIG_RETENTION_BOOT_MODE
	/* the bootloader enters bootloader mode at the next reset */
	(void)bootmode_set(BOOT_MODE_TYPE_BOOTLOADER);
#ifdef CONFIG_BURNER_USB_TOUCH
	printk("Done; upload a sketch to install the loader\n");
#else
	printk("Done; press RESET and upload a sketch to install the loader\n");
#endif
#else
	printk("Done; double-tap RESET and upload a sketch to install the loader\n");
#endif

	/* slow blink: success, fast blink: failure */
	int period_ms = rc ? 100 : 1000;

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	while (true) {
		gpio_pin_toggle_dt(&led);
		for (int ms = 0; ms < period_ms; ms += 100) {
#ifdef CONFIG_BURNER_USB_TOUCH
			check_1200_bps_touch();
#endif
			k_msleep(100);
		}
	}
}
