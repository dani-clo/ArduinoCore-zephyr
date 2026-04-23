/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "RTC.h"
#include <errno.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/counter.h>
#include <stdio.h>

Rtc::~Rtc() {
}

bool Rtc::setYear(int year) {
	if (year < 1970 || year > 2100) {
		return false;
	}
	int currentYear, month, day, hour, minute, second;
	int retVal = getTime(currentYear, month, day, hour, minute, second);
	if (retVal == 0) {
		retVal = setTime(year, month, day, hour, minute, second);
		if (retVal == 0u) {
			return true;
		}
	}

	return false;
}

bool Rtc::setMonthOfYear(int month) {
	if (month < 1 || month > 12) {
		return false;
	}
	int year, currentMonth, day, hour, minute, second;
	int retVal = getTime(year, currentMonth, day, hour, minute, second);
	if (retVal == 0) {
		retVal = setTime(year, month, day, hour, minute, second);
		if (retVal == 0u) {
			return true;
		}
	}
	return false;
}

bool Rtc::setDayOfMonth(int day) {
	if (day < 1 || day > 31) {
		return false;
	}
	int year, month, currentDay, hour, minute, second;
	int retVal = getTime(year, month, currentDay, hour, minute, second);
	if (retVal == 0) {
		retVal = setTime(year, month, day, hour, minute, second);
		if (retVal == 0u) {
			return true;
		}
	}
	return false;
}

bool Rtc::setHour(int hour) {
	if (hour < 0 || hour > 23) {
		return false;
	}
	int year, month, day, currentHour, minute, second;
	int retVal = getTime(year, month, day, currentHour, minute, second);
	if (retVal == 0) {
		retVal = setTime(year, month, day, hour, minute, second);
		if (retVal == 0u) {
			return true;
		}
	}
	return false;
}

bool Rtc::setMinute(int minute) {
	if (minute < 0 || minute > 59) {
		return false;
	}
	int year, month, day, hour, currentMinute, second;
	int retVal = getTime(year, month, day, hour, currentMinute, second);
	if (retVal == 0) {
		retVal = setTime(year, month, day, hour, minute, second);
		if (retVal == 0u) {
			return true;
		}
	}
	return false;
}

bool Rtc::setSecond(int second) {
	if (second < 0 || second > 59) {
		return false;
	}
	int year, month, day, hour, minute, currentSecond;
	int retVal = getTime(year, month, day, hour, minute, currentSecond);
	if (retVal == 0) {
		retVal = setTime(year, month, day, hour, minute, second);
		if (retVal == 0u) {
			return true;
		}
	}
	return false;
}

int Rtc::getYear() {
	int year, month, day, hour, minute, second;
	int retVal = getTime(year, month, day, hour, minute, second);
	if (retVal == 0) {
		return year;
	}
	return -1;
}

int Rtc::getMonth() {
	int year, month, day, hour, minute, second;
	int retVal = getTime(year, month, day, hour, minute, second);
	if (retVal == 0) {
		return month;
	}
	return -1;
}

int Rtc::getDayOfMonth() {
	int year, month, day, hour, minute, second;
	int retVal = getTime(year, month, day, hour, minute, second);
	if (retVal == 0) {
		return day;
	}
	return -1;
}

int Rtc::getHour() {
	int year, month, day, hour, minute, second;
	int retVal = getTime(year, month, day, hour, minute, second);
	if (retVal == 0) {
		return hour;
	}
	return -1;
}

int Rtc::getMinutes() {
	int year, month, day, hour, minute, second;
	int retVal = getTime(year, month, day, hour, minute, second);
	if (retVal == 0) {
		return minute;
	}
	return -1;
}

int Rtc::getSeconds() {
	int year, month, day, hour, minute, second;
	int retVal = getTime(year, month, day, hour, minute, second);
	if (retVal == 0) {
		return second;
	}
	return -1;
}

#if !defined(CONFIG_COUNTER_NRF_RTC) // For boards with a dedicated RTC peripheral/driver, we use
									 // the Zephyr RTC

#if DT_NODE_HAS_STATUS(DT_NODELABEL(rtc), okay)
#define RTC_NODE DT_NODELABEL(rtc)
#else
#define RTC_NODE DT_INVALID_NODE
#warning "RTC node not found or not enabled in devicetree"
#endif

/**
 * @brief Rtc library constructor
 *
 * Constructor used with STM32 and RP2040-based boards using Zephyr RTC drivers
 *
 */
Rtc::Rtc() {
	rtc_dev = DEVICE_DT_GET_OR_NULL(RTC_NODE);
	if (!rtc_dev) {
		printk("RTC device not found in device tree\n");
	}
}

/**
 * @brief Rtc library classic begin() function
 *
 * Takes care of the underlying STM32 RTC HAL driver initalization
 *
 * @return bool operation result
 */
bool Rtc::begin() {
	if (!device_is_ready(rtc_dev)) {
		printk("RTC:: not ready\n");
		return false;
	}

	// Register the alarm callback (even if not active yet)
	rtc_alarm_set_callback(rtc_dev, alarmId, Rtc::alarmCallbackWrapper, this);

	return true;
}

/**
 * @brief Rtc library setter function
 *
 * Used to set the time and data with a single function call
 *
 * @param int year to be set
 * @param int month to be set
 * @param int day to be set
 * @param int hour to be set
 * @param int minute to be set
 * @param int second to be set
 * @return int operation result
 */
int Rtc::setTime(int year, int month, int day, int hour, int minute, int second) {
	struct rtc_time time = {
		.tm_sec = second,
		.tm_min = minute,
		.tm_hour = hour,
		.tm_mday = day,
		.tm_mon = month - 1,
		.tm_year = year - 1900,
		.tm_wday = 0,
		.tm_yday = 0,
		.tm_isdst = -1, /**< Daylight saving time flag (Unknown = -1) */
		.tm_nsec = 0    /* Nanoseconds (Unknown = 0) */
	};

	return rtc_set_time(rtc_dev, &time);
}

/**
 * @brief Rtc library getter
 *
 * Used to get the time and date with a single function call
 *
 * @param int reference to currently set year
 * @param int reference to currently set month
 * @param int reference to currently set day of month
 * @param int reference to currently set hour
 * @param int reference to currently set minute
 * @param int reference to currently set second
 */
int Rtc::getTime(int &year, int &month, int &day, int &hour, int &minute, int &second) {
	struct rtc_time time;
	int ret = rtc_get_time(rtc_dev, &time);
	if (ret < 0) {
		return ret;
	}

	year = time.tm_year + 1900;
	month = time.tm_mon + 1;
	day = time.tm_mday;
	hour = time.tm_hour;
	minute = time.tm_min;
	second = time.tm_sec;

	return 0;
}

/**
 * @brief Rtc library setter
 *
 * Used to set the alarm time through a single function call
 *
 * @param int year to be set
 * @param int month to be set
 * @param int day to be set
 * @param int hour to be set
 * @param int minute to be set
 * @param int second to be set
 * @return int operation result
 */
int Rtc::setAlarm(int year, int month, int day, int hour, int minute, int second,
				  RtcAlarmCallback cb, void *user_data) {
	struct rtc_time alarm_time = {
		.tm_sec = second,
		.tm_min = minute,
		.tm_hour = hour,
		.tm_mday = day,
		.tm_mon = month - 1,
		.tm_year = year - 1900,
		.tm_wday = -1,  /* unknown */
		.tm_yday = -1,  /* unknown */
		.tm_isdst = -1, /* unknown */
		.tm_nsec = 0    /* unknown */
	};

	uint16_t supported_mask = 0;
	int ret = rtc_alarm_get_supported_fields(rtc_dev, alarmId, &supported_mask);
	if (ret < 0) {
		return ret;
	}

	// Build the actual mask based on what we intend to set
	uint16_t mask = 0;
	if (supported_mask & RTC_ALARM_TIME_MASK_SECOND) {
		mask |= RTC_ALARM_TIME_MASK_SECOND;
	}
	if (supported_mask & RTC_ALARM_TIME_MASK_MINUTE) {
		mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (supported_mask & RTC_ALARM_TIME_MASK_HOUR) {
		mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (supported_mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}
	if (supported_mask & RTC_ALARM_TIME_MASK_MONTH) {
		mask |= RTC_ALARM_TIME_MASK_MONTH;
	}
	if (supported_mask & RTC_ALARM_TIME_MASK_YEAR) {
		mask |= RTC_ALARM_TIME_MASK_YEAR;
	}

	// Save user callback
	userAlarmCallback = cb;
	userAlarmCallbackData = user_data;

	return rtc_alarm_set_time(rtc_dev, alarmId, mask, &alarm_time);
}

/**
 * @brief Rtc library getter
 *
 * Used to get the time and date of the currently set alarm
 *
 * @param int reference to currently set year
 * @param int reference to currently set month
 * @param int reference to currently set day of month
 * @param int reference to currently set hour
 * @param int reference to currently set minute
 * @param int reference to currently set second
 * @return int operation result
 */
int Rtc::getAlarm(int &year, int &month, int &day, int &hour, int &minute, int &second) {
	struct rtc_time alarm_time;
	uint16_t mask = 0;

	int ret = rtc_alarm_get_time(rtc_dev, alarmId, &mask, &alarm_time);
	if (ret < 0) {
		return ret;
	}

	year = alarm_time.tm_year + 1900;
	month = alarm_time.tm_mon + 1;
	day = alarm_time.tm_mday;
	hour = alarm_time.tm_hour;
	minute = alarm_time.tm_min;
	second = alarm_time.tm_sec;

	return 0;
}

/**
 * @brief Rtc utility function
 *
 * Used to cancel a previously set alarm
 *
 * @return int operation result
 */
int Rtc::cancelAlarm() {
	// Clear alarm by setting mask to 0
	struct rtc_time dummy = {};
	return rtc_alarm_set_time(rtc_dev, alarmId, 0, &dummy);
}

/**
 * @brief Rtc utility function
 *
 * Used to check if an alarm is currently set
 *
 * @return bool operation result, true for alarm pending, false otherwise
 */
bool Rtc::isAlarmPending() {
	int ret = rtc_alarm_is_pending(rtc_dev, alarmId);
	return ret > 0;
}

/**
 * @brief Rtc callback function
 *
 * This alarm callback wrapper is needed to make the connection between C-style low level driver
 * function callbacks which need to be static and the objects derived from the RTC class; You do not
 * need to call this directly in the app
 *
 * @param const struct device *dev is the hardware device (not used at this level)
 * @param uint16_t id is an event/alarm identifier
 * @param void *user_data is a void pointer the user can set when registering the callback
 */
void Rtc::alarmCallbackWrapper([[maybe_unused]] const struct device *dev,
							   [[maybe_unused]] uint16_t id, void *user_data) {
	Rtc *self = static_cast<Rtc *>(user_data);
	if (self && self->userAlarmCallback) {
		self->userAlarmCallback(self->userAlarmCallbackData);
	}
}

/**
 * @brief Rtc utility function
 *
 * This function allows for updating the user callback and the message passed to it
 *
 * @param RtcUpdateCallback cb is the new callback which will be called in the app when the event
 * fires later on
 * @param void *user_data is a void pointer the user can set when registering the callback
 */
int Rtc::setUpdateCallback(RtcUpdateCallback cb, void *user_data) {
#if defined(CONFIG_RTC_RPI_PICO)
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
#else
	userUpdateCallback = cb;
	userUpdateCallbackData = user_data;

	return rtc_update_set_callback(rtc_dev, Rtc::updateCallbackWrapper, this);
#endif
}

/**
 * @brief Rtc utility function
 *
 * This function is a wrapper which allows for connecting the low level driver and the registered
 * callback with the instance of the object we use in the application sketch
 *
 * @param const struct device *dev is the hardware device (not used at this level)
 * @param void *user_data is a void pointer the user can set when registering the callback
 */
void Rtc::updateCallbackWrapper([[maybe_unused]] const struct device *dev, void *user_data) {
	Rtc *self = static_cast<Rtc *>(user_data);
	if (self && self->userUpdateCallback) {
		self->userUpdateCallback(self->userUpdateCallbackData);
	}
}

/**
 * @brief Sets the Rtc calibration value.
 *
 * This function writes a calibration value to the Rtc hardware to adjust
 * timing accuracy. A positive or negative calibration factor may be applied,
 * depending on whether the clock is running fast or slow.
 *
 * @param calibration The calibration adjustment value in parts per million (ppm).
 * @return 0 if successful, or a negative error code on failure.
 */
int Rtc::setCalibration(int32_t calibration) {
#if defined(CONFIG_RTC_RPI_PICO)
	ARG_UNUSED(calibration);
	return -ENOTSUP;
#else
	return rtc_set_calibration(rtc_dev, calibration);
#endif
}

/**
 * @brief Retrieves the current Rtc calibration value.
 *
 * This function reads the current calibration adjustment applied to the Rtc.
 *
 * @param calibration Reference to an integer where the current calibration
 *        value (in parts per million) will be stored.
 * @return 0 if successful, or a negative error code on failure.
 */
int Rtc::getCalibration(int32_t &calibration) {
#if defined(CONFIG_RTC_RPI_PICO)
	ARG_UNUSED(calibration);
	return -ENOTSUP;
#else
	return rtc_get_calibration(rtc_dev, &calibration);
#endif
}

#else // For other platforms (i.e. Nordic), we must use the counter API to implement RTC
	  // functionality

#if DT_NODE_HAS_STATUS(DT_NODELABEL(rtc1), okay)
#define COUNTER_NODE DT_NODELABEL(rtc1)
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(rtc2), okay)
#define COUNTER_NODE DT_NODELABEL(rtc2)
#else
#warning "RTC node not found or not enabled in devicetree"
#define COUNTER_NODE DT_INVALID_NODE
#endif
LOG_MODULE_REGISTER(Rtc);

// Utility functions - local scope only
namespace {
static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static bool is_leap(int year) {
	return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}
} // namespace

/**
 * @brief Static alarm handler for the RTC driver.
 *
 * This function acts as a static wrapper between the hardware interrupt and
 * the user-defined callback. It retrieves the relevant `RTC` instance
 * from the provided `user_data` and invokes the user’s callback function.
 *
 * @param dev Pointer to the RTC device structure (hardware driver).
 * @param chan_id The alarm channel identifier.
 * @param ticks The current tick count at the time of the alarm.
 * @param user_data Pointer to user-defined data (the RTC instance).
 */
void Rtc::alarmHandler(const struct device *dev, uint8_t chan_id, uint32_t ticks, void *user_data) {
	Rtc *rtc = static_cast<Rtc *>(user_data);
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);

	rtc->alarmPending = false;
	if (rtc->userAlarmCallback) {
		rtc->userAlarmCallback(rtc->userAlarmCallbackData);
	} else {
		printk("Alarm triggered but no callback registered! Channel: %d, Ticks: %u\n", chan_id,
			   ticks);
	}
}

/**
 * @brief Constructs an Rtc object.
 *
 * Initializes internal variables, sets default offsets, and retrieves
 * the hardware RTC counter device.
 */
Rtc::Rtc() {
	counter_dev = DEVICE_DT_GET_OR_NULL(COUNTER_NODE);
	timeOffset = 0;
}

/**
 * @brief Initializes the RTC hardware interface.
 *
 * This function checks if the RTC device is ready and starts the counter.
 *
 * @return true if the device is ready and successfully started, false otherwise.
 */
bool Rtc::begin() {
	if (!counter_dev) {
		printk("RTC counter device not found in device tree\n");
		return false;
	}
	if (!device_is_ready(counter_dev)) {
		printk("RTC counter device not ready\n");
		return false;
	}

	counter_start(counter_dev);
	return true;
}

/**
 * @brief Sets the current RTC time.
 *
 * This function converts a date and time into epoch format and adjusts the
 * internal time offset based on the current tick counter.
 *
 * @param year   The current year (e.g., 2025).
 * @param month  The current month (1–12).
 * @param day    The day of the month (1–31).
 * @param hour   The hour (0–23).
 * @param minute The minute (0–59).
 * @param second The second (0–59).
 * @return 0 if successful, or a negative error code if frequency or ticks are invalid.
 */
int Rtc::setTime(int year, int month, int day, int hour, int minute, int second) {
	time_t target = datetimeToEpoch(year, month, day, hour, minute, second);

	uint32_t ticks;
	counter_get_value(counter_dev, &ticks);

	uint32_t freq = counter_get_frequency(counter_dev);
	if (freq == 0) {
		printk("Counter frequency is zero\n");
		return -1;
	}

	timeOffset = target - (ticks / freq);
	return 0;
}

/**
 * @brief Retrieves the current RTC time.
 *
 * This function reads the current counter value, applies the time offset,
 * and converts the epoch time to a human-readable date and time.
 *
 * @param year   Reference to store the current year.
 * @param month  Reference to store the current month.
 * @param day    Reference to store the current day.
 * @param hour   Reference to store the current hour.
 * @param minute Reference to store the current minute.
 * @param second Reference to store the current second.
 * @return 0 if successful.
 */
int Rtc::getTime(int &year, int &month, int &day, int &hour, int &minute, int &second) {
	uint32_t ticks;
	counter_get_value(counter_dev, &ticks);

	uint32_t freq = counter_get_frequency(counter_dev);
	time_t now = timeOffset + (ticks / freq);

	epochToDatetime(now, year, month, day, hour, minute, second);
	return 0;
}

/**
 * @brief Sets an RTC alarm.
 *
 * This function schedules an alarm for a specific date and time. When the
 * target time is reached, the registered user callback is invoked.
 *
 * @param year      Target alarm year.
 * @param month     Target alarm month.
 * @param day       Target alarm day.
 * @param hour      Target alarm hour.
 * @param minute    Target alarm minute.
 * @param second    Target alarm second.
 * @param cb        Function pointer to the user-defined callback.
 * @param user_data Pointer to user data passed to the callback when triggered.
 * @return 0 if successful, or a negative error code if the target time is invalid.
 */
int Rtc::setAlarm(int year, int month, int day, int hour, int minute, int second,
				  RtcAlarmCallback cb, void *user_data) {
	time_t target_epoch = datetimeToEpoch(year, month, day, hour, minute, second);
	uint32_t freq = counter_get_frequency(counter_dev);
	uint32_t current_ticks;

	if (counter_get_value(counter_dev, &current_ticks) != 0) {
		printk("Failed to get current counter value\n");
		return -1;
	}

	time_t current_epoch = timeOffset + (current_ticks / freq);
	time_t delta_seconds = target_epoch - current_epoch;

	if (delta_seconds <= 0) {
		return -1;
	}

	uint32_t alarm_ticks = current_ticks + delta_seconds * freq;

	// Save user callback
	userAlarmCallback = cb;
	userAlarmCallbackData = user_data;
	alarmEpoch = target_epoch;
	alarmPending = true;

	alarm_cfg.flags = 0; // Absolute alarm
	alarm_cfg.ticks = alarm_ticks;
	alarm_cfg.callback = Rtc::alarmHandler;
	alarm_cfg.user_data = this;

	int ret = counter_set_channel_alarm(counter_dev, 0, &alarm_cfg);
	if (ret != 0) {
		printk("Failed to set alarm: %d\n", ret);
		userAlarmCallback = nullptr;
		userAlarmCallbackData = nullptr;
		alarmEpoch = 0;
		alarmPending = false;
		return ret;
	}

	return 0;
}

/**
 * @brief Cancels the currently scheduled alarm.
 *
 * This function stops any active alarm and clears the registered callback
 * and user data.
 * @return 0 on success, negative error code otherwise.
 */
int Rtc::cancelAlarm() {
	int ret = counter_cancel_channel_alarm(counter_dev, 0);
	if (ret != 0) {
		printk("Failed to cancel alarm: %d\n", ret);
	}
	userAlarmCallback = nullptr;
	userAlarmCallbackData = nullptr;
	alarmEpoch = 0;
	alarmPending = false;
	return ret;
}

/**
 * @brief Retrieves the currently configured alarm time.
 *
 * @return 0 on success, or a negative error code if no alarm is active.
 */
int Rtc::getAlarm([[maybe_unused]] int &year, [[maybe_unused]] int &month,
				  [[maybe_unused]] int &day, [[maybe_unused]] int &hour,
				  [[maybe_unused]] int &minute, [[maybe_unused]] int &second) {
	if (!alarmPending) {
		return -ENOENT;
	}

	epochToDatetime(alarmEpoch, year, month, day, hour, minute, second);
	return 0;
}

/**
 * @brief Checks whether an alarm is currently pending.
 *
 * @return true if an alarm is pending, false otherwise.
 */
bool Rtc::isAlarmPending() {
	return alarmPending;
}

/**
 * @brief Registers an update callback function.
 * This function is not supported and will return -1 to indicate this.
 *
 * @return -1 to indicate not supported.
 */
int Rtc::setUpdateCallback([[maybe_unused]] RtcUpdateCallback cb,
						   [[maybe_unused]] void *user_data) {
	return -ENOTSUP;
}

/**
 * @brief Sets the Rtc calibration value.
 * This function is not supported and will return -1 to indicate this.
 *
 * @return -1 to indicate not supported.
 */
int Rtc::setCalibration([[maybe_unused]] int32_t calibration) {
	return -ENOTSUP;
}

/**
 * @brief Retrieves the current Rtc calibration value.
 * This function is not supported and will return -1 to indicate this.
 *
 * @return -1 to indicate not supported.
 */
int Rtc::getCalibration([[maybe_unused]] int32_t &calibration) {
	return -ENOTSUP;
}

/**
 * @brief Converts a date and time to Unix epoch seconds.
 *
 * This utility function calculates the number of seconds since
 * January 1, 1970 (the Unix epoch) for the provided date and time.
 *
 * @param year   Year value (e.g., 2025).
 * @param month  Month value (1–12).
 * @param day    Day value (1–31).
 * @param hour   Hour value (0–23).
 * @param minute Minute value (0–59).
 * @param second Second value (0–59).
 * @return Corresponding epoch time in seconds.
 */
time_t Rtc::datetimeToEpoch(int year, int month, int day, int hour, int minute, int second) {
	int y = year;
	int m = month;
	time_t days = 0;

	for (int i = 1970; i < y; i++) {
		days += is_leap(i) ? 366 : 365;
	}

	for (int i = 1; i < m; i++) {
		days += days_in_month[i - 1];
		if (i == 2 && is_leap(y)) {
			days += 1;
		}
	}

	days += (day - 1);

	return (((days * 24 + hour) * 60 + minute) * 60 + second);
}

/**
 * @brief Converts Unix epoch seconds to a date and time.
 *
 * This utility function breaks down a given epoch timestamp into a
 * human-readable calendar date and time.
 *
 * @param t       Epoch time in seconds.
 * @param year    Reference to store the year.
 * @param month   Reference to store the month.
 * @param day     Reference to store the day.
 * @param hour    Reference to store the hour.
 * @param minute  Reference to store the minute.
 * @param second  Reference to store the second.
 */
void Rtc::epochToDatetime(time_t t, int &year, int &month, int &day, int &hour, int &minute,
						  int &second) {
	time_t seconds_in_day = t % 86400;
	time_t days = t / 86400;

	hour = seconds_in_day / 3600;
	minute = (seconds_in_day % 3600) / 60;
	second = seconds_in_day % 60;

	year = 1970;
	while (true) {
		int days_in_year = is_leap(year) ? 366 : 365;
		if (days >= days_in_year) {
			days -= days_in_year;
			year++;
		} else {
			break;
		}
	}

	int m = 0;
	while (true) {
		int dim = days_in_month[m];
		if (m == 1 && is_leap(year)) {
			dim++;
		}

		if (days >= dim) {
			days -= dim;
			m++;
		} else {
			break;
		}
	}

	month = m + 1;
	day = days + 1;
}
#endif /* !defined(CONFIG_COUNTER) */
