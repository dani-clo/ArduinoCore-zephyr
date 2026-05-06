/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "RTC.h"

Rtc rtc;

int year, month, day, hour, minute, second;
int previousSecond = 0;
char printBuffer[30];               // Allocate large enough buffer to hold the 28 characters of the output format "Time is: 2025-09-25 11:49:26"

void setup() {
    Serial.begin(115200);
    while (!Serial) {
        ; // Wait for Serial to be ready
    }
    if (!rtc.begin()) {
        Serial.println("RTC not ready\n");
        return;
    }
    if (!rtc.isRunning()) {
        Serial.println("RTC is not running, setting initial time to 2025-10-21 12:00:00\n");
        rtc.setTime(2025, 10, 21, 12, 0, 0);
    }
}

void loop() {
    rtc.getTime(year, month, day, hour, minute, second);    // Read back time from hardware
    sprintf(printBuffer, "Time is: %04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    Serial.println(printBuffer);
    delay(1000);
}