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
 * @file dw3000_phy.h
 * @author UWB Core <uwbcore@gmail.com>
 * @date 2018
 * @brief physical layer
 *
 * @details This is the phy base class which utilises the functions to set the clocks,initializes phy layer and configures the required
 * parameters.
 */

#ifndef _DW3000_PHY_H_
#define _DW3000_PHY_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <hal/hal_spi.h>
#include <dw3000-c0/dw3000_dev.h>
#include <dw3000-c0/dw3000_mac.h>
#include <dw3000-c0/dw3000_gpio.h>
#include <dw3000-c0/dw3000_otp.h>

#define PEAK_MULTPLIER  (0x60) //!<  Peak Multiplier
#define N_STD_FACTOR    (13)   //!< Number of standard deviation factor
#define LDE_PARAM1      (PEAK_MULTPLIER | N_STD_FACTOR) //!< LDE masking for 8-bit configuration
#define LDE_PARAM3_16 (0x1607)   //!<  LDE masking for 16-bit configuration
#define LDE_PARAM3_64 (0x0607)   //!< LDE masking for 64-bit configuration
#define MIXER_GAIN_STEP (0.5)    //!< TODO
#define DA_ATTN_STEP    (2.5)    //!< TODO

//! Enum of txrf config parameters
typedef enum {
    DW3000_txrf_config_18db = 0,   //!< txrf_config_18db for 18db power gain
    DW3000_txrf_config_15db,       //!< txrf_config_15db for 15db power gain
    DW3000_txrf_config_12db,       //!< txrf_config_12db for 12db power gain
    DW3000_txrf_config_9db,        //!< txrf_config_9db for 9db power gain
    DW3000_txrf_config_6db,        //!< txrf_config_6db for 6db power gain
    DW3000_txrf_config_3db,        //!< txrf_config_3db for 3db power gain
    DW3000_txrf_config_0db,        //!< txrf_config_0db for 0db power gain
    DW3000_txrf_config_off         //!< txrf_config_off
}coarse_power_levels_t;

#define dw3000_power_value(COARSE,FINE) ((COARSE<<5) + (0xFFFFF & (uint16_t)(FINE * 2)))    //!< To configure power values
#define dw3000_power_valuef(COARSE,FINE) ((COARSE<<5) + (0xFFFFF & (uint16_t)(DPL_FLOAT32_INT(DPL_FLOAT32_MUL(fine, DPL_FLOAT32_INIT(2.0f))))))

struct uwb_dev_status dw3000_phy_init(struct _dw3000_dev_instance_t * inst, struct uwb_dev_txrf_config * txrf_config);
void dw3000_phy_sysclk_XTAL(struct _dw3000_dev_instance_t * inst);
void dw3000_phy_sysclk_PLL(struct _dw3000_dev_instance_t * inst);
void dw3000_phy_sysclk_SEQ(struct _dw3000_dev_instance_t * inst);
void dw3000_phy_sysclk_ACC(struct _dw3000_dev_instance_t * inst, uint8_t mode);
void dw3000_phy_disable_sequencing(struct _dw3000_dev_instance_t * inst);
void dw3000_phy_config_lde(struct _dw3000_dev_instance_t * inst, int prfIndex);
void dw3000_phy_config_txrf(struct _dw3000_dev_instance_t * inst, struct uwb_dev_txrf_config * config);
void dw3000_phy_rx_reset(struct _dw3000_dev_instance_t * inst);
void dw3000_phy_forcetrxoff(struct _dw3000_dev_instance_t * inst);
void dw3000_phy_interrupt_mask(struct _dw3000_dev_instance_t * inst, uint32_t bitmask, uint8_t enable);

#define dw3000_phy_set_rx_antennadelay(inst, rxDelay) dw3000_write_reg(inst, LDE_IF_ID, LDE_RXANTD_OFFSET, rxDelay, sizeof(uint16_t)) //!< Set the RX antenna delay for auto TX timestamp adjustment
#define dw3000_phy_set_tx_antennadelay(inst, txDelay) dw3000_write_reg(inst, TX_ANTD_ID, TX_ANTD_OFFSET, txDelay, sizeof(uint16_t)) //!< Set the TX antenna delay for auto TX timestamp adjustment
#define dw3000_phy_read_wakeuptemp(inst) ((uint8_t) dw3000_read_reg(inst, TX_CAL_ID, TC_SARL_SAR_LTEMP_OFFSET, sizeof(uint8_t))) //!< Read the temperature level of the DW3000 that was sampled on waking from Sleep/Deepsleep
#define dw3000_phy_read_wakeupvbat(inst) ((uint8_t) dw3000_read_reg(inst, TX_CAL_ID, TC_SARL_SAR_LVBAT_OFFSET, sizeof(uint8_t))) //!< Read the battery voltage of the DW3000 that was sampled on waking from Sleep/Deepsleep


float dw3000_phy_read_wakeuptemp_SI(struct _dw3000_dev_instance_t * inst);
float dw3000_phy_read_read_wakeupvbat_SI(struct _dw3000_dev_instance_t * inst);
void dw3000_phy_external_sync(struct _dw3000_dev_instance_t * inst, uint8_t delay, bool enable);

void dw3000_phy_repeated_frames(struct _dw3000_dev_instance_t * inst, uint64_t rate);

uint16_t dw3000_phy_SHR_duration(struct uwb_phy_attributes * attrib);
uint16_t dw3000_phy_data_duration(struct uwb_phy_attributes * attrib, uint16_t nlen);
uint16_t dw3000_phy_frame_duration(struct uwb_phy_attributes * attrib, uint16_t nlen);

void dw3000_phy_enable_ext_pa(struct _dw3000_dev_instance_t* inst, bool enable);
void dw3000_phy_enable_ext_lna(struct _dw3000_dev_instance_t* inst, bool enable);

bool dw3000_phy_txrf_power_value(struct _dw3000_dev_instance_t * inst, uint8_t *reg, dpl_float32_t coarse, dpl_float32_t fine);

struct uwb_dev_status dw3000_phy_event_cnt_ctrl(struct _dw3000_dev_instance_t *inst, bool enable, bool reset);
struct uwb_dev_status dw3000_phy_event_cnt_read(struct _dw3000_dev_instance_t *inst, struct uwb_dev_evcnt *res);

#ifdef __cplusplus
}
#endif

#endif /* _DW3000_PHY_H_ */
