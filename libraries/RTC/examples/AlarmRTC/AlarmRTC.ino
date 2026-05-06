/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: MPL-2.0
 */

/*
* This sketch sets an alarm 10 seconds in the future and handles it via a callback.
*/

#include "RTC.h"
#include <stdio.h>

Rtc rtc;

char printBuffer[30]; // declare a buffer of large enough size for the message we want to display
int year, month, day, hour, minute, second;

void onAlarm(void *user_data) {
    char printBuffer[40];
    sprintf(printBuffer, "Alarm went off! Message: %s\n", (char *)user_data);
    Serial.println(printBuffer);
}

void setup() {
    int ret = 0xDEADBEEFu; // Starting with a custom value for the return which will definitely lead to failure if not changed to zero (i.e. success) by the functions below
    char printBuffer[60];
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial to be ready
    }

    if (!rtc.begin()) {
        Serial.println("RTC not ready\n");
        return;
    }

    int year, month, day, hour, minute, second;
    ret = rtc.getTime(year, month, day, hour, minute, second);
    if(ret != 0)
    {
        rtc.setTime(2025, 10, 21, 12, 0, 0);
    }

    sprintf(printBuffer, "Current Time: %04d-%02d-%02d %02d:%02d:%02d\n", year, month, day, hour, minute, second);
    Serial.println(printBuffer);

    // Set alarm 10 seconds into the future
    second += 5;
    // Correct for minute rollover if necessary
    if (second >= 60) {
        second -= 60;
        minute += 1;
    }

    // The method also allows for registering a function callback which will be called when the alarm sets off and a display message which will be shown in the console
    ret = rtc.setAlarm(year, month, day, hour, minute, second, onAlarm, (void *)"Wake up!!!");
    if (ret == 0) {
        sprintf(printBuffer, "Alarm set for: %02d:%02d:%02d\n", hour, minute, second);
        Serial.println(printBuffer);
    } else {
        sprintf(printBuffer, "Failed to set alarm (%d)\n", ret);
        Serial.println(printBuffer);
    }
}

void loop() {
    char printBuffer[30]; // declare a buffer of large enough size for the message we want to display
    int y, m, d, h, min, s;
    rtc.getTime(y, m, d, h, min, s);
    // Because the print() and println() functions do not support formatted output directly, we can use this trick to prepare a buffer with the string we want to show
    sprintf(printBuffer, "Time is: %04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, min, s);
    Serial.println(printBuffer);
    delay(1000);
}