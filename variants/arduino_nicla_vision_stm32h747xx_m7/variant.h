/*
 * Copyright (c) Arduino s.r.l. and/or its affiliated companies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pure_analog_pins.h"

#define LEDR (4u) /* D4 - Red LED PE_3 */
#define LEDG (5u) /* D5 - Green LED PC_13 */
#define LEDB (6u) /* D6 - Blue LED PF_4 */

// SPI
// ---
#define SPI_HOWMANY (2)

/* SPI0 --> Nicla Vision SPI4 */
#define PIN_SPI_MISO (13u) /* D13 - PE_13 */
#define PIN_SPI_MOSI (14u) /* D14 - PE_14 */
#define PIN_SPI_SCK  (12u) /* D12 - PE_12 */
#define PIN_SPI_SS   (11u) /* D11 - PE_11 */

static const uint8_t SS = PIN_SPI_SS;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK = PIN_SPI_SCK;

// SPI1 --> Nicla Vision SPI5 (used for LSM6DSOX IMU)
#define LSM6DS_DEFAULT_SPI SPI1
#define LSM6DS_INT         (1u)

#define PIN_SPI_MISO1 (9u)  /* D9  - PF_8  */
#define PIN_SPI_SS1   (7u)  /* D7  - PF_6  */
#define PIN_SPI_MOSI1 (10u) /* D10 - PF_11 */
#define PIN_SPI_SCK1  (8u)  /* D8  - PF_7  */

#define SDA 0
#define SCL 0
