/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/* RS485 configuration for ArduinoModbus/ArduinoRS485 libraries on USART3 (Serial1) */
#define RS485_SERIAL_PORT           Serial1

#define RS485_DEFAULT_TX_PIN        -1             // TX managed by UART hardware
#define RS485_DEFAULT_DE_PIN        21             // D21 = gpiob 14 (Driver Enable)
#define RS485_DEFAULT_RE_PIN        22             // D22 = gpiob 13 (Receiver Enable)
