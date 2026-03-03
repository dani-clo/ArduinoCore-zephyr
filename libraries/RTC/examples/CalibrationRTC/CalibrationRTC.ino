/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: MPL-2.0
 */

#include "RTC.h"
#include <stdio.h>

Rtc rtc;

void setup() {
    char printBuffer[64];
    delay(1000);
    Serial.begin(115200);
    if (!rtc.begin()) {
        printf("RTC not ready\n");
        return;
    }

    int calib = 0;
    if (rtc.getCalibration(calib) == 0) {
        snprintf(printBuffer, sizeof(printBuffer), "Current calibration: %d\n", calib);
        Serial.println(printBuffer);
    } else {
        Serial.println("Failed to get calibration");
    }

    // Apply a small positive calibration (e.g., +1)
    // This value is hardware-dependent, in a real application you should check the microcontroller's datasheet for the correct amount.
    int32_t new_calib = calib + 1;

    if (rtc.setCalibration(new_calib) == 0) {
        snprintf(printBuffer, sizeof(printBuffer), "Calibration updated to: %d\n", new_calib);
        Serial.println(printBuffer);
    } else {
        Serial.println("Failed to set calibration");
    }
}

void loop() {
    delay(5000);
}
