/*
 * Copyright (c) 2019
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BITTERS__RPI__H
#define __BITTERS__RPI__H


/* Raspberry Pi BCM
 *
 * The header GPIO bank is named by the label its pinctrl driver gives the
 * chip, not by device number: the number is assigned at probe and is not
 * the same on every model or kernel -- the Pi 5 moved the header to the
 * RP1 southbridge -- while the label names the silicon. bitters tries the
 * alternatives in order (see ctrl_devname in <bitters/gpio.h>); the last
 * is the device name every earlier release used, so a chip none of the
 * labels match behaves as before. The line offsets below are the same on
 * all of them.
 *
 * Defined only if it is not already, so a build can say which chip it
 * means on the compiler command line, as a string literal:
 *
 *     -DBITTERS_RPI_BCM_GPIO_CHIP='"gpiochip4"'
 */
#define BITTERS_RPI_GPIO_LABEL_BCM2835	"pinctrl-bcm2835"	// Pi 1-3, Zero
#define BITTERS_RPI_GPIO_LABEL_BCM2711	"pinctrl-bcm2711"	// Pi 4
#define BITTERS_RPI_GPIO_LABEL_RP1	"pinctrl-rp1"		// Pi 5

#ifndef BITTERS_RPI_BCM_GPIO_CHIP
#define BITTERS_RPI_BCM_GPIO_CHIP					\
    BITTERS_RPI_GPIO_LABEL_RP1     "|"					\
    BITTERS_RPI_GPIO_LABEL_BCM2711 "|"					\
    BITTERS_RPI_GPIO_LABEL_BCM2835 "|"					\
    "gpiochip0"
#endif

#define BITTERS_RPI_BCM_GPIO_0		0
#define BITTERS_RPI_BCM_GPIO_1		1
#define BITTERS_RPI_BCM_GPIO_2		2
#define BITTERS_RPI_BCM_GPIO_3		3
#define BITTERS_RPI_BCM_GPIO_4		4
#define BITTERS_RPI_BCM_GPIO_5		5
#define BITTERS_RPI_BCM_GPIO_6		6
#define BITTERS_RPI_BCM_GPIO_7		7
#define BITTERS_RPI_BCM_GPIO_8		8
#define BITTERS_RPI_BCM_GPIO_9		9
#define BITTERS_RPI_BCM_GPIO_10		10
#define BITTERS_RPI_BCM_GPIO_11		11
#define BITTERS_RPI_BCM_GPIO_12		12
#define BITTERS_RPI_BCM_GPIO_13		13
#define BITTERS_RPI_BCM_GPIO_14		14
#define BITTERS_RPI_BCM_GPIO_15		15
#define BITTERS_RPI_BCM_GPIO_16		16
#define BITTERS_RPI_BCM_GPIO_17		17
#define BITTERS_RPI_BCM_GPIO_18		18
#define BITTERS_RPI_BCM_GPIO_19		19
#define BITTERS_RPI_BCM_GPIO_20		20
#define BITTERS_RPI_BCM_GPIO_21		21
#define BITTERS_RPI_BCM_GPIO_22		22
#define BITTERS_RPI_BCM_GPIO_23		23
#define BITTERS_RPI_BCM_GPIO_24		24
#define BITTERS_RPI_BCM_GPIO_25		25
#define BITTERS_RPI_BCM_GPIO_26		26
#define BITTERS_RPI_BCM_GPIO_27		27
#define BITTERS_RPI_BCM_GPIO_MAX	28



/* Peripheric mapping
 */
#define BITTERS_RPI_GPIO_CHIP		BITTERS_RPI_BCM_GPIO_CHIP

#define BITTERS_RPI_SPI0_MOSI		BITTERS_RPI_BCM_GPIO_10	// P1_19
#define BITTERS_RPI_SPI0_MISO		BITTERS_RPI_BCM_GPIO_9	// P1_21
#define BITTERS_RPI_SPI0_SCLK		BITTERS_RPI_BCM_GPIO_11	// P1_23
#define BITTERS_RPI_SPI0_CE0		BITTERS_RPI_BCM_GPIO_8	// P1_24
#define BITTERS_RPI_SPI0_CE1		BITTERS_RPI_BCM_GPIO_7	// P1_26
#define BITTERS_RPI_SPI0		0

#define BITTERS_RPI_SPI1_MOSI		BITTERS_RPI_BCM_GPIO_20	// P1_38
#define BITTERS_RPI_SPI1_MISO		BITTERS_RPI_BCM_GPIO_19	// P1_35
#define BITTERS_RPI_SPI1_SCLK		BITTERS_RPI_BCM_GPIO_21	// P1_40
#define BITTERS_RPI_SPI1		1

#define BITTERS_RPI_PWM0		BITTERS_RPI_BCM_GPIO_18	// P1_12
#define BITTERS_RPI_PWM1		BITTERS_RPI_BCM_GPIO_13	// P1_33

#define BITTERS_RPI_I2C1_SDA		BITTERS_RPI_BCM_GPIO_2	// P1_3
#define BITTERS_RPI_I2C1_SCL		BITTERS_RPI_BCM_GPIO_3	// P1_5
#define BITTERS_RPI_I2C1		1

#define BITTERS_RPI_UART0_TX		BITTERS_RPI_BCM_GPIO_14	// P1_8
#define BITTERS_RPI_UART0_RX		BITTERS_RPI_BCM_GPIO_15	// P1_10

#define BITTERS_RPI_ID_SD		BITTERS_RPI_BCM_GPIO_0	// P1_27
#define BITTERS_RPI_ID_SC		BITTERS_RPI_BCM_GPIO_1	// P1_28

#define BITTERS_RPI_GPCLK0		BITTERS_RPI_BCM_GPIO_4	// P1_7



/* Rasbperry Pi Pinout
 * See: https://pinout.xyz/#
 */
#undef  BITTERS_RPI_P1_1		// Power: 3.3v
#undef  BITTERS_RPI_P1_2		// Power: 5v
#define BITTERS_RPI_P1_3		BITTERS_RPI_BCM_GPIO_2
#undef  BITTERS_RPI_P1_4		// Power: 5v
#define BITTERS_RPI_P1_5		BITTERS_RPI_BCM_GPIO_3
#undef  BITTERS_RPI_P1_6		// Ground
#define BITTERS_RPI_P1_7		BITTERS_RPI_BCM_GPIO_4
#define BITTERS_RPI_P1_8		BITTERS_RPI_BCM_GPIO_14
#undef  BITTERS_RPI_P1_9		// Ground
#define BITTERS_RPI_P1_10		BITTERS_RPI_BCM_GPIO_15
#define BITTERS_RPI_P1_11		BITTERS_RPI_BCM_GPIO_17
#define BITTERS_RPI_P1_12		BITTERS_RPI_BCM_GPIO_18
#define BITTERS_RPI_P1_13		BITTERS_RPI_BCM_GPIO_27
#undef  BITTERS_RPI_P1_14		// Ground
#define BITTERS_RPI_P1_15		BITTERS_RPI_BCM_GPIO_22
#define BITTERS_RPI_P1_16		BITTERS_RPI_BCM_GPIO_23
#undef  BITTERS_RPI_P1_17		// Power: 3.3v
#define BITTERS_RPI_P1_18		BITTERS_RPI_BCM_GPIO_24
#define BITTERS_RPI_P1_19		BITTERS_RPI_BCM_GPIO_10
#undef  BITTERS_RPI_P1_20		// Ground
#define BITTERS_RPI_P1_21		BITTERS_RPI_BCM_GPIO_9
#define BITTERS_RPI_P1_22		BITTERS_RPI_BCM_GPIO_25
#define BITTERS_RPI_P1_23		BITTERS_RPI_BCM_GPIO_11
#define BITTERS_RPI_P1_24		BITTERS_RPI_BCM_GPIO_8
#undef  BITTERS_RPI_P1_25		// Ground
#define BITTERS_RPI_P1_26		BITTERS_RPI_BCM_GPIO_7
#define BITTERS_RPI_P1_27		BITTERS_RPI_BCM_GPIO_0
#define BITTERS_RPI_P1_28		BITTERS_RPI_BCM_GPIO_1
#define BITTERS_RPI_P1_29		BITTERS_RPI_BCM_GPIO_5
#undef  BITTERS_RPI_P1_30		// Ground
#define BITTERS_RPI_P1_31		BITTERS_RPI_BCM_GPIO_6
#define BITTERS_RPI_P1_32		BITTERS_RPI_BCM_GPIO_12
#define BITTERS_RPI_P1_33		BITTERS_RPI_BCM_GPIO_13
#undef  BITTERS_RPI_P1_34		// Ground
#define BITTERS_RPI_P1_35		BITTERS_RPI_BCM_GPIO_19
#define BITTERS_RPI_P1_36		BITTERS_RPI_BCM_GPIO_16
#define BITTERS_RPI_P1_37		BITTERS_RPI_BCM_GPIO_26
#define BITTERS_RPI_P1_38		BITTERS_RPI_BCM_GPIO_20
#undef  BITTERS_RPI_P1_39		// Ground
#define BITTERS_RPI_P1_40		BITTERS_RPI_BCM_GPIO_21


#endif
