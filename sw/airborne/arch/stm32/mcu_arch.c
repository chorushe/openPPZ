/*
 * Copyright (C) 2010-2012 The Paparazzi team
 *
 * This file is part of Paparazzi.
 *
 * Paparazzi is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * Paparazzi is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Paparazzi; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/**
 * @file arch/stm32/mcu_arch.c
 * @brief stm32 arch dependant microcontroller initialisation functions.
 * @ingroup stm32_arch
 */
/*
 stm32的一些初始化：设定中断向量表的地址和外部时钟
 */

#include "mcu.h"

#include BOARD_CONFIG

#include <inttypes.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/rcc.h>
#include <libopencm3/stm32/f1/flash.h>
#include <libopencm3/cm3/scb.h>

#include "std.h"

void mcu_arch_init(void) {
#if LUFTBOOT
PRINT_CONFIG_MSG("We are running luftboot, the interrupt vector is being relocated.")
  SCB_VTOR = 0x00002000;
#endif
#if EXT_CLK == 8000000
PRINT_CONFIG_MSG("Using 8MHz external clock to PLL it to 72MHz.")
  rcc_clock_setup_in_hse_8mhz_out_72mhz();
#elif EXT_CLK == 12000000
PRINT_CONFIG_MSG("Using 12MHz external clock to PLL it to 72MHz.")
  rcc_clock_setup_in_hse_12mhz_out_72mhz();
#else
#error EXT_CLK is either set to an unsupported frequency or not defined at all. Please check!
#endif
}

