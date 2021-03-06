/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * @file dw3000_gpio.c
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief General Purpose Input Output
 *
 * @details This is the gpio base class which utilises functions to enable/disable all the configurations related to GPIO.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <dpl/dpl.h>
#include <dpl/dpl_cputime.h>
#include <hal/hal_spi.h>
#include <hal/hal_gpio.h>
#include <dw3000-c0/dw3000_gpio.h>


/**
 * Set the mode on a gpio
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param gpioNum       This is the GPIO to configure (0-8)
 * @param mode          See manual for available modes for different pins
 *
 * @return void
 */
void
dw3000_gpio_set_mode(struct _dw3000_dev_instance_t * inst, uint8_t gpioNum, uint8_t mode)
{
    uint32_t reg;
    reg = (uint32_t) dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, sizeof(uint32_t));
    reg &= ~(0x3UL << (6+gpioNum*2));
    reg |= (mode << (6+gpioNum*2));
    dw3000_write_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, reg, sizeof(uint32_t));
}

/**
 * Reads the current mode on a gpio
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param gpioNum       This is the GPIO to configure (0-8)
 * @param mode          See manual for available modes for different pins
 *
 * @return void
 */
uint8_t
dw3000_gpio_get_mode(struct _dw3000_dev_instance_t * inst, uint8_t gpioNum)
{
    uint32_t reg;
    reg = (uint32_t) dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, sizeof(uint32_t));
    reg &= (0x3UL << (6+gpioNum*2));
    reg >>= (6+gpioNum*2);
    return (uint8_t)(reg&0x3);
}

/**
 * API to set up gpio 4/5/6 as pa/lna mode
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 *
 * @return void
 */
void dw3000_gpio4_config_ext_pa(struct _dw3000_dev_instance_t * inst)
{
    uint32_t reg;

        reg = (uint32_t) dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, sizeof(uint32_t));
        reg &= ~GPIO_MSGP4_MASK;
        reg |= GPIO_PIN4_EXTPA;
        dw3000_write_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, reg, sizeof(uint32_t));
}

void dw3000_gpio5_config_ext_txe(struct _dw3000_dev_instance_t * inst)
{
    uint32_t reg;

        reg = (uint32_t) dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, sizeof(uint32_t));
        reg &= ~GPIO_MSGP5_MASK;
        reg |= GPIO_PIN5_EXTTXE;
        dw3000_write_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, reg, sizeof(uint32_t));
}

void dw3000_gpio6_config_ext_rxe(struct _dw3000_dev_instance_t * inst)
{
    uint32_t reg;

        reg = (uint32_t) dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, sizeof(uint32_t));
        reg &= ~GPIO_MSGP6_MASK;
        reg |= GPIO_PIN6_EXTRXE;
        dw3000_write_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, reg, sizeof(uint32_t));
}

/**
 * API to set up Tx/Rx GPIOs which could be used to control LEDs.
 * Note: not completely IC dependent, also needs board with LEDS fitted on right I/O lines
 *       this function enables GPIOs 2 and 3 which are connected to LED3 and LED4 on EVB1000.
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param mode  This is a bit field interpreted as follows:
 *          - bit 0: 1 to enable LEDs, 0 to disable them
 *          - bit 1: 1 to make LEDs blink once on init. Only valid if bit 0 is set (enable LEDs)
 *          - bit 2 to 7: reserved
 *
 * @return void
 */
void dw3000_gpio_config_leds(struct _dw3000_dev_instance_t * inst, dw3000_led_modes_t mode)
{
    uint32_t reg;

    if (mode & DWT_LEDS_ENABLE){
        // Set up MFIO for LED output.
        reg = (uint32_t) dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, sizeof(uint32_t));
        reg &= ~(GPIO_MSGP2_MASK | GPIO_MSGP3_MASK);
        reg |= (GPIO_PIN2_RXLED | GPIO_PIN3_TXLED);
        dw3000_write_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, reg, sizeof(uint32_t));

        // Enable LP Oscillator to run from counter and turn on de-bounce clock.
        reg = (uint32_t) dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint32_t));
        reg |= (PMSC_CTRL0_GPDCE | PMSC_CTRL0_KHZCLEN);
        dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, reg, sizeof(uint32_t));
        dw3000_write_reg(inst, PMSC_ID, PMSC_LEDC_OFFSET, PMSC_LEDC_BLNKEN | PMSC_LEDC_BLINK_TIME_DEF, sizeof(uint32_t));

        if (mode & DWT_LEDS_INIT_BLINK){
            // Single blink sign-of-life.
            reg = (uint32_t) dw3000_read_reg(inst, PMSC_ID, PMSC_LEDC_OFFSET, sizeof(uint32_t));
            reg |= PMSC_LEDC_BLINK_NOW_ALL;

            dw3000_write_reg(inst, PMSC_ID, PMSC_LEDC_OFFSET, reg, sizeof(uint32_t));
            reg &= ~PMSC_LEDC_BLINK_NOW_ALL;
            dpl_cputime_delay_usecs(10);
            dw3000_write_reg(inst, PMSC_ID, PMSC_LEDC_OFFSET, reg, sizeof(uint32_t));
        }
    }else{
        // Clear the GPIO bits that are used for LED control.
        reg = dw3000_read_reg(inst,GPIO_CTRL_ID, GPIO_MODE_OFFSET,sizeof(uint32_t));
        reg &= ~(GPIO_MSGP2_MASK | GPIO_MSGP3_MASK);
        dw3000_write_reg(inst, GPIO_CTRL_ID, GPIO_MODE_OFFSET, reg, sizeof(uint32_t));
    }
}

/**
 * API to set GPIO direction as an input (1) or output (0).
 *
 * @param gpioNum       This is the GPIO to configure (0-8)
 * @param direction     This sets the GPIO direction input (1) or output (0)
 *
 * @return void
 */
void
dw3000_gpio_set_direction(struct _dw3000_dev_instance_t * inst,
                          uint8_t gpioNum, uint8_t direction)
{
    uint32_t reg;
    uint8_t buf[GPIO_DIR_LEN];
    uint32_t command;

    assert(gpioNum < 9);

    /* Activate GPIO Clock if not already active */
    reg = dw3000_read_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET, sizeof(uint32_t));
    if ((reg&PMSC_CTRL0_GPCE) == 0 || (reg&PMSC_CTRL0_GPRN) == 0) {
        dw3000_write_reg(inst, PMSC_ID, PMSC_CTRL0_OFFSET,
                         reg|PMSC_CTRL0_GPCE|PMSC_CTRL0_GPRN,
                         sizeof(uint32_t));
    }

    /* See GxM1-8 and GxP1-8 in dw3000_regs.h. Mask | Value */
    if (gpioNum < 4) {
        command = (1 << (gpioNum+4)) | (direction << gpioNum);
    } else if (gpioNum<8) {
        command = (1 << (gpioNum+4+8)) | (direction << (gpioNum+8));
    } else {
        command = (1 << (gpioNum+4+12)) | (direction << (gpioNum+12));
    }

    buf[0] = command & 0xff;
    buf[1] = (command >> 8) & 0xff;
    buf[2] = (command >> 16) & 0xff;

    dw3000_write(inst, GPIO_CTRL_ID, GPIO_DIR_OFFSET, buf, GPIO_DIR_LEN);
}

/**
 * Get GPIO direction as an input (1) or output (0).
 *
 * @param gpioNum       This is the GPIO to configure (0-8)
 * @param direction     This sets the GPIO direction input (1) or output (0)
 *
 * @return uint8_t
 */
uint8_t
dw3000_gpio_get_direction(struct _dw3000_dev_instance_t * inst, uint8_t gpioNum)
{
    uint8_t res;
    uint32_t reg;

    assert(gpioNum < 9);

    reg = dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_DIR_OFFSET, sizeof(uint32_t));

    if (gpioNum < 4) {
        res = (uint8_t)(0x1 & (reg >> gpioNum));
    } else if (gpioNum<8) {
        res = (uint8_t)(0x1 & (reg >> (gpioNum+8)));
    } else {
        res = (uint8_t)(0x1 & (reg >> (gpioNum+12)));
    }
    return res;
}

/**
 * API to set GPIO value as (1) or (0) only applies if the GPIO is configured as output.
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param gpioNum       This is the GPIO to configure (0-8)
 * @param value         This sets the GPIO HIGH (1) or LOW (0)
 *
 * @return void
 */
void
dw3000_gpio_set_value(struct _dw3000_dev_instance_t * inst,
                      uint8_t gpioNum, uint8_t value)
{
    uint8_t buf[GPIO_DOUT_LEN];
    uint32_t command;

    assert(gpioNum < 9);

    /* See GxM1-8 and GxP1-8 in dw3000_regs.h. Mask | Value */
    if (gpioNum < 4) {
        command = (1 << (gpioNum+4)) | (value << gpioNum);
    } else if (gpioNum<8) {
        command = (1 << (gpioNum+4+8)) | (value << (gpioNum+8));
    } else {
        command = (1 << (gpioNum+4+12)) | (value << (gpioNum+12));
    }
    buf[0] = command & 0xff;
    buf[1] = (command >> 8) & 0xff;
    buf[2] = (command >> 16) & 0xff;

    dw3000_write(inst, GPIO_CTRL_ID, GPIO_DOUT_OFFSET, buf, GPIO_DOUT_LEN);
}

/**
 * API to get GPIO value from an GPIO pin configured as input
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 *
 * @return uint32_t     The raw state on the pins currently
 */
uint32_t
dw3000_gpio_get_values(struct _dw3000_dev_instance_t * inst)
{
    uint32_t reg;
    reg = (uint32_t) dw3000_read_reg(inst, GPIO_CTRL_ID, GPIO_RAW_OFFSET,
                                     sizeof(uint32_t));
    return (reg&GPIO_RAW_MASK);
}

/**
 * API that matches Mynewt's hal_gpio_init_out
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param gpioNum       This is the GPIO to configure (0-7)
 * @param val           This sets the GPIO HIGH (1) or LOW (0)
 *
 * @return void
 */
void
dw3000_gpio_init_out(struct _dw3000_dev_instance_t * inst, int gpioNum, int val)
{
    dw3000_gpio_set_direction(inst, gpioNum, 0);
    dw3000_gpio_set_value(inst, gpioNum, val);
}

/**
 * API that matches Mynewt's hal_gpio_init_in
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param gpioNum       This is the GPIO to configure (0-7)
 * @param val           This sets the GPIO HIGH (1) or LOW (0)
 *
 * @return void
 */
void
dw3000_gpio_init_in(struct _dw3000_dev_instance_t * inst, int gpioNum)
{
    dw3000_gpio_set_direction(inst, gpioNum, 1);
}

/**
 * API to get GPIO value from an GPIO pin configured as input
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param gpioNum       This is the GPIO to read from (0-8)
 *
 * @return int          The raw state on the pin currently
 */
int
dw3000_gpio_read(struct _dw3000_dev_instance_t * inst, uint8_t gpioNum)
{
    uint32_t reg = dw3000_gpio_get_values(inst);
    return (reg&(1<<gpioNum)) ? 1:0;
}

/**
 * API that matches Mynewt's hal_gpio_write
 *
 * @param inst  pointer to _dw3000_dev_instance_t.
 * @param gpioNum       This is the GPIO to configure (0-7)
 * @param val           This sets the GPIO HIGH (1) or LOW (0)
 *
 * @return void
 */
void
dw3000_gpio_write(struct _dw3000_dev_instance_t * inst, int gpioNum, int val)
{
    dw3000_gpio_set_value(inst, gpioNum, val);
}
