/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: MPL-2.0
 */

/**
 * @file RTC.h
 * @brief RTC library header for Zephyr-based Arduino boards.
 *
 * This class provides a unified interface for accessing and configuring
 * real-time clock (RTC) peripherals across multiple Arduino platforms,
 * such as the Portenta H7, GIGA R1, Opta, Nano 33 BLE, and Nicla Sense ME.
 *
 * It allows for reading and writing time, managing alarms, applying calibration,
 * and registering update callbacks.
 *
 * Supported APIs adapt automatically to the underlying hardware.
 */

#pragma once

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/rtc.h>
#include <time.h>

/**
 * @typedef RtcAlarmCallback
 * @brief Function pointer type for RTC alarm callbacks.
 *
 * @param user_data User-defined data passed to the callback when the alarm triggers.
 */
typedef void (*RtcAlarmCallback)(void *user_data);

/**
 * @typedef RtcUpdateCallback
 * @brief Function pointer type for RTC update event callbacks.
 *
 * @param user_data User-defined data passed to the callback when the RTC updates.
 */
typedef void (*RtcUpdateCallback)(void *user_data);

/**
 * @class Rtc
 * @brief Cross-platform RTC abstraction for Arduino boards running Zephyr.
 *
 * The Rtc class provides an object-oriented interface for interacting with
 * the hardware real-time clock. It supports time management, alarm scheduling,
 * calibration, and update event callbacks.
 *
 * Depending on the board, it interfaces with either the Zephyr RTC driver
 * or the Zephyr counter driver for boards without a dedicated RTC peripheral.
 *
 */
class Rtc {
public:
	/**
	 * @brief Constructs a new Rtc object.
	 *
	 * Initializes internal pointers and prepares the instance
	 * for communication with the underlying RTC or counter driver.
	 */
	Rtc();

	/**
	 * @brief Initializes the RTC hardware.
	 *
	 * Must be called before using other methods.
	 *
	 * @return true if the RTC device was successfully initialized, false otherwise.
	 */
	bool begin();

	/**
	 * @brief Destroys the Rtc object.
	 */
	~Rtc();

	/**
	 * @brief Sets the current day of the month in the RTC.
	 *
	 * Updates only the day field of the current date and keeps other
	 * components (year, month, time) intact.
	 *
	 * @param day Day value to be set (1–31).
	 * @return true if the operation succeeds, false otherwise.
	 */
	bool setDayOfMonth(int day);

	/**
	 * @brief Sets the current month in the RTC.
	 *
	 * Updates only the month field while keeping all other time components
	 * unchanged.
	 *
	 * @param month Month value to be set (1–12).
	 * @return true if the operation succeeds, false otherwise.
	 */
	bool setMonthOfYear(int month);

	/**
	 * @brief Sets the current year in the RTC.
	 *
	 * Retrieves the current RTC time, updates only the year field, and writes the
	 * new time back to the RTC while preserving other date and time components.
	 *
	 * @param year Year value to be set (e.g., 2025).
	 * @return true if the operation succeeds, false otherwise.
	 */
	bool setYear(int year);

	/**
	 * @brief Sets the current hour in the RTC.
	 *
	 * Updates the hour component of the RTC without modifying date, minutes, or seconds.
	 *
	 * @param hour Hour value to be set (0–23).
	 * @return true if the operation succeeds, false otherwise.
	 */
	bool setHour(int hour);

	/**
	 * @brief Sets the current minute in the RTC.
	 *
	 * Updates the minute field of the current time, keeping all other
	 * components unchanged.
	 *
	 * @param minute Minute value to be set (0–59).
	 * @return true if the operation succeeds, false otherwise.
	 */
	bool setMinute(int minute);

	/**
	 * @brief Sets the current second in the RTC.
	 *
	 * Updates only the seconds field while preserving other components.
	 *
	 * @param second Second value to be set (0–59).
	 * @return true if the operation succeeds, false otherwise.
	 */
	bool setSecond(int second);

	/**
	 * @brief Retrieves the current day of the month from the RTC.
	 *
	 * Reads the RTC date and returns the day field.
	 *
	 * @return The current day of the month (1–31).
	 */
	int getDayOfMonth();

	/**
	 * @brief Retrieves the current month from the RTC.
	 *
	 * Reads the current date and returns the month component.
	 *
	 * @return The current month as an integer (1–12).
	 */
	int getMonth();

	/**
	 * @brief Retrieves the current year from the RTC.
	 *
	 * Reads the current date and time and returns the year component.
	 *
	 * @return The current year as an integer (e.g. 2025).
	 */
	int getYear();

	/**
	 * @brief Retrieves the current hour from the RTC.
	 *
	 * Reads the RTC time and returns the hour component.
	 *
	 * @return The current hour (0–23).
	 */
	int getHour();

	/**
	 * @brief Retrieves the current minute from the RTC.
	 *
	 * Reads the RTC time and returns the minute component.
	 *
	 * @return The current minute (0–59).
	 */
	int getMinutes();

	/**
	 * @brief Retrieves the current second from the RTC.
	 *
	 * Reads the RTC time and returns the second component.
	 *
	 * @return The current second (0–59).
	 */
	int getSeconds();

	/**
	 * @brief Sets the complete date and time.
	 *
	 * @param year Year (e.g., 2025)
	 * @param month Month (1–12)
	 * @param day Day of the month (1–31)
	 * @param hour Hour (0–23)
	 * @param minute Minute (0–59)
	 * @param second Second (0–59)
	 * @return 0 on success, negative error code otherwise.
	 */
	int setTime(int year, int month, int day, int hour, int minute, int second);

	/**
	 * @brief Retrieves the current date and time.
	 *
	 * @param year Reference to store the current year.
	 * @param month Reference to store the current month.
	 * @param day Reference to store the current day.
	 * @param hour Reference to store the current hour.
	 * @param minute Reference to store the current minute.
	 * @param second Reference to store the current second.
	 * @return 0 on success, negative error code otherwise.
	 */
	int getTime(int &year, int &month, int &day, int &hour, int &minute, int &second);

#if defined(CONFIG_RTC_STM32) || defined(CONFIG_RTC_RPI_PICO) || defined(CONFIG_RTC_RENESAS_RA)
	/**
	 * @brief Schedules an alarm for a specific time.
	 *
	 * @param year Target year.
	 * @param month Target month.
	 * @param day Target day of the month.
	 * @param hour Target hour.
	 * @param minute Target minute.
	 * @param second Target second.
	 * @param cb Optional user callback triggered when the alarm fires.
	 * @param user_data Optional user-defined data passed to the callback.
	 * @return 0 on success, negative error code otherwise.
	 */
	int setAlarm(int year, int month, int day, int hour, int minute, int second,
				 RtcAlarmCallback cb = nullptr, void *user_data = nullptr);

	/**
	 * @brief Cancels the currently active alarm.
	 * @return 0 on success, negative error code otherwise.
	 */
	int cancelAlarm();

#else
	/**
	 * @brief Sets an RTC alarm for boards using the Zephyr counter driver.
	 *
	 * @param year Target year.
	 * @param month Target month.
	 * @param day Target day.
	 * @param hour Target hour.
	 * @param minute Target minute.
	 * @param second Target second.
	 * @param callback Function pointer for the alarm event.
	 * @param cb_user_data Pointer to user data passed to the callback.
	 * @return 0 on success, negative error code otherwise.
	 */
	int setAlarm(int year, int month, int day, int hour, int minute, int second,
				 void (*callback)(const struct device *dev, uint8_t chan_id, uint32_t ticks,
								  void *user_data),
				 void *cb_user_data);

	/**
	 * @brief Cancels an active alarm.
	 *
	 * @return 0 on success, negative error code otherwise.
	 */
	int cancelAlarm();

	/**
	 * @brief Retrieves the currently configured alarm time.
	 *
	 * @return -1 to indicate not supported.
	 */
	int getAlarm([[maybe_unused]] int &year, [[maybe_unused]] int &month, [[maybe_unused]] int &day,
				 [[maybe_unused]] int &hour, [[maybe_unused]] int &minute,
				 [[maybe_unused]] int &second);

	/**
	 * @brief Checks whether an alarm is currently pending.
	 * This function is not supported and will return -1 to indicate this.
	 *
	 * @return false to indicate not supported.
	 */
	bool isAlarmPending();

	/**
	 * @brief Registers an update callback function.
	 * This function is not supported and will return -1 to indicate this.
	 *
	 * @return -1 to indicate not supported.
	 */
	int setUpdateCallback([[maybe_unused]] RtcUpdateCallback cb, [[maybe_unused]] void *user_data);

	/**
	 * @brief Sets the Rtc calibration value.
	 * This function is not supported and will return -1 to indicate this.
	 *
	 * @return -1 to indicate not supported.
	 */
	int setCalibration([[maybe_unused]] int32_t calibration);

	/**
	 * @brief Retrieves the current Rtc calibration value.
	 * This function is not supported and will return -1 to indicate this.
	 *
	 * @return -1 to indicate not supported.
	 */
	int getCalibration([[maybe_unused]] int32_t &calibration);
#endif

#if defined(CONFIG_RTC_STM32) || defined(CONFIG_RTC_RPI_PICO) || defined(CONFIG_RTC_RENESAS_RA)
	/**
	 * @brief Retrieves the currently configured alarm time.
	 *
	 * @param year Reference to store the alarm year.
	 * @param month Reference to store the alarm month.
	 * @param day Reference to store the alarm day.
	 * @param hour Reference to store the alarm hour.
	 * @param minute Reference to store the alarm minute.
	 * @param second Reference to store the alarm second.
	 * @return 0 on success, negative error code otherwise.
	 */
	int getAlarm(int &year, int &month, int &day, int &hour, int &minute, int &second);

	/**
	 * @brief Checks whether an alarm is currently pending.
	 *
	 * @return true if an alarm is pending, false otherwise.
	 */
	bool isAlarmPending();

	/**
	 * @brief Registers an update callback function.
	 *
	 * The callback is invoked when the RTC generates an update event.
	 *
	 * @param cb Function pointer for the update callback.
	 * @param user_data Optional user data passed to the callback.
	 * @return 0 on success, negative error code otherwise.
	 */
	int setUpdateCallback(RtcUpdateCallback cb, void *user_data);

	/**
	 * @brief Applies a calibration correction to the RTC.
	 *
	 * @param calibration Calibration offset in parts per million (ppm).
	 * @return 0 on success, negative error code otherwise.
	 */
	int setCalibration(int32_t calibration);

	/**
	 * @brief Reads the current RTC calibration setting.
	 *
	 * @param calibration Reference to store the current calibration value in ppm.
	 * @return 0 on success, negative error code otherwise.
	 */
	int getCalibration(int32_t &calibration);
#endif

private:
#if defined(CONFIG_RTC_STM32) || defined(CONFIG_RTC_RPI_PICO) || defined(CONFIG_RTC_RENESAS_RA)
	/** @brief Pointer to the Zephyr RTC device. */
	const struct device *rtc_dev;

	/** @brief Internal static wrapper for alarm callbacks. */
	static void alarmCallbackWrapper([[maybe_unused]] const struct device *dev,
									 [[maybe_unused]] uint16_t id, void *user_data);

	/** @brief Internal static wrapper for update callbacks. */
	static void updateCallbackWrapper([[maybe_unused]] const struct device *dev, void *user_data);

	RtcAlarmCallback userAlarmCallback = nullptr; /**< User-registered alarm callback. */
	void *userAlarmCallbackData = nullptr;        /**< User data for alarm callback. */

	RtcUpdateCallback userUpdateCallback = nullptr; /**< User-registered update callback. */
	void *userUpdateCallbackData = nullptr;         /**< User data for update callback. */

	uint16_t alarmId = 0; /**< Default alarm identifier. */

#else // For boards without a dedicated RTC, we use the counter API to implement RTC functionality
	/** @brief Pointer to the Zephyr counter device used as RTC backend. */
	const struct device *counter_dev;

	/** @brief Time offset between the system counter and epoch time. */
	time_t timeOffset;

	/** @brief Counter driver alarm configuration. */
	struct counter_alarm_cfg alarm_cfg;

	/** @brief User-registered callback for counter-based alarms. */
	void (*user_callback)(const struct device *dev, uint8_t chan_id, uint32_t ticks,
						  void *user_data);

	/** @brief User data associated with the alarm callback. */
	void *user_data;

	/** @brief Static handler for counter alarm interrupts. */
	static void alarmHandler(const struct device *dev, uint8_t chan_id, uint32_t ticks,
							 void *user_data);

	/** @brief Converts a date and time to a Unix epoch timestamp. */
	time_t datetimeToEpoch(int year, int month, int day, int hour, int minute, int second);

	/** @brief Converts a Unix epoch timestamp to a date and time. */
	void epochToDatetime(time_t t, int &year, int &month, int &day, int &hour, int &minute,
						 int &second);
#endif
};
